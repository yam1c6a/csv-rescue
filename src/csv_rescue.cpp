#include "csv_rescue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace csvr {

const char* CsvRescue::kEmpty = "";

// 文字列複製（範囲）
static char* dup_range(const char* s, int len) {
    char* p = (char*)malloc((size_t)len + 1);
    if (!p) return 0;
    if (len > 0 && s) memcpy(p, s, (size_t)len);
    p[len] = '\0';
    return p;
}

// v0.2 ============================================================

// 左に padChar で width まで埋める
std::string PadLeft(
    const std::string& src,
    size_t width,
    char padChar
){
    if (src.length() >= width)
        return src;

    return std::string(width - src.length(), padChar) + src;
}

// 右に padChar で width まで埋める
std::string PadRight(
    const std::string& src,
    size_t width,
    char padChar
){
    if (src.length() >= width)
        return src;

    return src + std::string(width - src.length(), padChar);
}

// 数字以外をすべて除去する（" 01-23 " → "0123"）
std::string NormalizeNumber(const std::string& src)
{
    std::string out;
    for (size_t i = 0; i < src.length(); i++) {
        if (src[i] >= '0' && src[i] <= '9') {
			out += src[i];
        }
    }
    return out;
}

// 数値 + 左0埋め
std::string NormalizeNumberPad(
    const std::string& src,
    size_t width
){
    std::string n = NormalizeNumber(src);
    return PadLeft(n, width, '0');
}


CsvRescue::CsvRescue()
: m_rows(0), m_rowCount(0), m_rowCap(0), m_maxCols(0) {}

CsvRescue::~CsvRescue() {
    Clear();
}

void CsvRescue::Clear() {
    if (m_rows) {
        int i;
        for (i = 0; i < m_rowCount; ++i) {
            FreeRow(&m_rows[i]);
        }
        free(m_rows);
    }
    m_rows = 0;
    m_rowCount = 0;
    m_rowCap = 0;
    m_maxCols = 0;
}

bool CsvRescue::LoadFromFile(const char* path) {
    Clear();
    if (!path) return false;

    FILE* fp = fopen(path, "rb");
    if (!fp) return false;

    // サイズ取得
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz < 0) { fclose(fp); return false; }

    // 読み込み（空ファイルも許容するが、0.1では false 扱いにする）
    char* buf = (char*)malloc((size_t)sz);
    if (!buf) { fclose(fp); return false; }

    size_t rd = fread(buf, 1, (size_t)sz, fp);
    fclose(fp);

    bool ok = LoadFromMemory(buf, (int)rd);
    free(buf);
    return ok;
}

bool CsvRescue::LoadFromMemory(const char* data, int size) {
    Clear();
    if (!data || size <= 0) return false;
    return ParseCsv(data, size);
}

// 行の解放
void CsvRescue::FreeRow(Row* row) {
    if (!row) return;
    if (row->cells) {
        int i;
        for (i = 0; i < row->count; ++i) {
            if (row->cells[i]) free(row->cells[i]);
        }
        free(row->cells);
    }
    row->cells = 0;
    row->count = 0;
}

// 行配列の容量確保
bool CsvRescue::EnsureRowCapacity(int need) {
    if (need <= m_rowCap) return true;

    int newCap = (m_rowCap == 0) ? 64 : m_rowCap;
    while (newCap < need) newCap *= 2;

    Row* p = (Row*)realloc(m_rows, sizeof(Row) * (size_t)newCap);
    if (!p) return false;

    // 新規領域を初期化
    {
        int i;
        for (i = m_rowCap; i < newCap; ++i) {
            p[i].cells = 0;
            p[i].count = 0;
        }
    }

    m_rows = p;
    m_rowCap = newCap;
    return true;
}

// 行を追加（所有権を移す）
void CsvRescue::PushRow(Row* row) {
    if (!row) return;
    if (m_rowCount < m_rowCap) {
        m_rows[m_rowCount].cells = row->cells;
        m_rows[m_rowCount].count = row->count;
        if (row->count > m_maxCols) m_maxCols = row->count;
        ++m_rowCount;
        row->cells = 0;
        row->count = 0;
    }
}

int CsvRescue::GetRowCount() const {
    return m_rowCount;
}

int CsvRescue::GetColumnCount() const {
    return m_maxCols;
}

// セル取得（範囲外は空文字）
const char* CsvRescue::Get(int row, int col) const {
    if (row < 0 || row >= m_rowCount) return kEmpty;
    if (col < 0) return kEmpty;
    const Row* r = &m_rows[row];
    if (col >= r->count) return kEmpty;
    return r->cells[col] ? r->cells[col] : kEmpty;
}

// --- CSV parsing ---
// ・区切り：,
// ・クオート："..."
// ・クオート内改行はセル内に含める（内部では \n に統一）
// ・未閉クオートは「行末までを1セル」として確定して続行（次行へ持ち越さない）
bool CsvRescue::ParseCsv(const char* data, int size) {
    if (!EnsureRowCapacity(1)) return false;

    // 一時行バッファ（1行ぶんのセル配列）
    Row cur;
    int curCap;

    // セル構築用バッファ
    char* cell;
    int cellLen;
    int cellCap;

    // VC6対策：goto と初期化の相性が悪いので、宣言→代入で初期化
    int inQuote;
    int i;

    // 初期化
    cur.cells = 0;
    cur.count = 0;
    curCap = 0;

    cell = 0;
    cellLen = 0;
    cellCap = 0;

    inQuote = 0;
    i = 0;

    // cell は必ず確保してから使う
    cellCap = 64;
    cell = (char*)malloc((size_t)cellCap);
    if (!cell) goto oom;
    cell[0] = '\0';
    cellLen = 0;

    // cur.cells は最初に malloc しておく（realloc(NULL, ...) を避ける）
    curCap = 16;
    cur.cells = (char**)malloc(sizeof(char*) * (size_t)curCap);
    if (!cur.cells) goto oom;
    cur.count = 0;

    // --- マクロ（VC6向け） ---
    #define ENSURE_CELL_CAP(n) do { \
        if ((n) > cellCap) { \
            int nc = cellCap; \
            while (nc < (n)) nc *= 2; \
            char* np = (char*)realloc(cell, (size_t)nc); \
            if (!np) goto oom; \
            cell = np; \
            cellCap = nc; \
        } \
    } while(0)

    #define CELL_PUSH_CHAR(ch) do { \
        ENSURE_CELL_CAP(cellLen + 2); \
        cell[cellLen++] = (char)(ch); \
        cell[cellLen] = '\0'; \
    } while(0)

    #define ROW_ENSURE(n) do { \
        if ((n) > curCap) { \
            int nc = curCap; \
            while (nc < (n)) nc *= 2; \
            char** np = (char**)realloc(cur.cells, sizeof(char*) * (size_t)nc); \
            if (!np) goto oom; \
            cur.cells = np; \
            curCap = nc; \
        } \
    } while(0)

    #define ROW_PUSH_CELL() do { \
        ROW_ENSURE(cur.count + 1); \
        cur.cells[cur.count++] = dup_range((cell ? cell : ""), cellLen); \
        cellLen = 0; \
        if (cell) cell[0] = '\0'; \
    } while(0)

    while (i < size) {
        unsigned char ch = (unsigned char)data[i];

        if (inQuote) {
            if (ch == '"') {
                inQuote = 0;
                ++i;
                continue;
            }

            // クオート内改行を許容（内部は \n に統一）
            if (ch == '\r') {
                CELL_PUSH_CHAR('\n');
                if (i + 1 < size && data[i + 1] == '\n') i += 2;
                else i += 1;
                continue;
            }
            if (ch == '\n') {
                CELL_PUSH_CHAR('\n');
                ++i;
                continue;
            }

            CELL_PUSH_CHAR(ch);
            ++i;
            continue;
        }

        // 非クオート
        if (ch == '"') {
            inQuote = 1;
            ++i;
            continue;
        }

        if (ch == ',') {
            ROW_PUSH_CELL();
            ++i;
            continue;
        }

        // 行終端（CR/LF）
        if (ch == '\r' || ch == '\n') {
            // 未閉クオートは「行末まで1セル」で確定（ここではinQuote==0のはずだが保険）
            inQuote = 0;

            ROW_PUSH_CELL();
            if (!EnsureRowCapacity(m_rowCount + 1)) goto oom;
            PushRow(&cur);

            // 次の行のためにセル配列を再確保（PushRow が所有権を持ったので cur.cells は 0）
            curCap = 16;
            cur.cells = (char**)malloc(sizeof(char*) * (size_t)curCap);
            if (!cur.cells) goto oom;
            cur.count = 0;

            // 改行の消費（\r\n 対応）
            if (ch == '\r' && i + 1 < size && data[i + 1] == '\n') i += 2;
            else i += 1;

            continue;
        }

        // 通常文字
        CELL_PUSH_CHAR(ch);
        ++i;
    }

    // 最終セル・最終行
    inQuote = 0;
    ROW_PUSH_CELL();
    if (!EnsureRowCapacity(m_rowCount + 1)) goto oom;
    PushRow(&cur);

    if (cell) free(cell);

    #undef ENSURE_CELL_CAP
    #undef CELL_PUSH_CHAR
    #undef ROW_ENSURE
    #undef ROW_PUSH_CELL

    return true;

oom:
    if (cell) free(cell);
    FreeRow(&cur);

    #undef ENSURE_CELL_CAP
    #undef CELL_PUSH_CHAR
    #undef ROW_ENSURE
    #undef ROW_PUSH_CELL

    return false;
}

// v0.2 ============================================================

void CsvRescue::ClearIndex()
{
    m_index.clear();
}

void CsvRescue::AddKey(int row, const std::string& key)
{
    KeyIndexItem item;
    item.key = key;
    item.row = row;
    m_index.push_back(item);
}

bool CsvRescue::KeyIndexLess(
    const KeyIndexItem& a,
    const KeyIndexItem& b
)
{
    return a.key < b.key;
}

void CsvRescue::Sort()
{
    std::sort(m_index.begin(), m_index.end(), KeyIndexLess);
}

int CsvRescue::Find(const std::string& key) const
{
    int left = 0;
    int right = (int)m_index.size(); // [left, right)

    while (left < right) {
        int mid = left + (right - left) / 2;

        if (m_index[mid].key < key) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }

    if (left < (int)m_index.size() && m_index[left].key == key) {
        return m_index[left].row;
    }
    return -1;
}

void CsvRescue::FindRange(const std::string& key, int& begin, int& end) const
{
    int sz = (int)m_index.size();
    begin = 0;
    end = 0;
    if (sz == 0) return;

    // lower_bound: 最初に key 以上になる位置
    int left = 0;
    int right = sz;
    while (left < right) {
        int mid = left + (right - left) / 2;
        if (m_index[mid].key < key) left = mid + 1;
        else right = mid;
    }
    int lb = left;

    if (lb >= sz || m_index[lb].key != key) {
        // 無い
        begin = lb;
        end = lb;
        return;
    }

    // upper_bound: 最初に key より大きくなる位置
    left = lb;
    right = sz;
    while (left < right) {
        int mid = left + (right - left) / 2;
        if (key < m_index[mid].key) right = mid;
        else left = mid + 1;
    }
    int ub = left;

    begin = lb;
    end = ub;
}

int CsvRescue::FindRows(const std::string& key, std::vector<int>& outRows) const
{
    outRows.clear();

    int b, e;
    FindRange(key, b, e);

    for (int i = b; i < e; i++) {
        outRows.push_back(m_index[i].row);
    }
    return (int)outRows.size();
}

} // namespace csvr

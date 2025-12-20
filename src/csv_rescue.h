\
#ifndef CSV_RESCUE_H_INCLUDED
#define CSV_RESCUE_H_INCLUDED

// csv-rescue v0.1
// 壊れ気味CSVを「止まらずに読む」ための最小CSVパーサ（VC++6.0想定）
// - 例外を投げない（戻り値で成否を返す）
// - 行ごとの列数ズレを許容（足りない列は空文字）
// - クオート内改行を許容
// - 未閉クオートは「行末までを1セル」として扱う
//
// 文字コードは Shift-JIS（ANSI / CP932）前提（0.1）

namespace csvr {

class CsvRescue {
public:
    CsvRescue();
    ~CsvRescue();

    // 読み込み（既存データは破棄される）
    bool LoadFromFile(const char* path);
    bool LoadFromMemory(const char* data, int size);

    // クリア
    void Clear();

    // 行数
    int GetRowCount() const;

    // 最大列数（観測値）
    int GetColumnCount() const;

    // セル取得（範囲外は空文字）
    const char* Get(int row, int col) const;

private:
    // コピー禁止（VC6想定）
    CsvRescue(const CsvRescue&);
    CsvRescue& operator=(const CsvRescue&);

    struct Row {
        char** cells;
        int    count;
    };

    Row* m_rows;
    int  m_rowCount;
    int  m_rowCap;
    int  m_maxCols;

    static const char* kEmpty;

    bool EnsureRowCapacity(int need);
    void PushRow(Row* row);
    void FreeRow(Row* row);

    // パース
    bool ParseCsv(const char* data, int size);
};

} // namespace csvr

#endif

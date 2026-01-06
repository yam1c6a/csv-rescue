#include <stdio.h>
#include <string>
#include <vector>
#include "../src/csv_rescue.h"

// v0.2 インデックス検索ドライバ（テスト兼）
// ・読み込み
// ・KEYインデックス構築（ClearIndex/AddKey/Sort）
// ・Find（1件）
// ・FindRows（同一KEY複数行）
static void print_escaped(const char* s)
{
    const unsigned char* p = (const unsigned char*)s;
    while (*p) {
        if (*p == '\n') { printf("\\n"); }
        else if (*p == '\r') { printf("\\r"); }
        else if (*p == '\t') { printf("\\t"); }
        else { putchar(*p); }
        ++p;
    }
}

// テスト用KEY生成（例）
// ・id列：数字のみ→6桁0埋め
// ・date列：左0埋め8桁（"20240101"想定）
// 重要：登録側と検索側で同じルールを使うこと
static std::string make_key_id_date(const char* id, const char* date)
{
    // idは数字以外を除去して6桁
    std::string k1 = csvr::NormalizeNumberPad(id, 6);

    // dateはそのまま8桁左0埋め（必要なら NormalizeNumberPad(date, 8) にしてもOK）
    std::string k2 = csvr::PadLeft(std::string(date), 8, '0');

    return k1 + k2;
}

int main(int argc, char** argv)
{
    const char* path = "test_index.csv";
    if (argc >= 2) path = argv[1];

    // 検索したい id/date（未指定ならデフォルト）
    const char* find_id = "1";
    const char* find_date = "20240101";
    if (argc >= 4) {
        find_id = argv[2];
        find_date = argv[3];
    }

    csvr::CsvRescue csv;
    if (!csv.LoadFromFile(path)) {
        printf("Load failed: %s\n", path);
        return 1;
    }

    int rows = csv.GetRowCount();
    int cols = csv.GetColumnCount();
    printf("rows=%d, max_cols=%d\n", rows, cols);

    // インデックス構築
    csv.ClearIndex();

    // 0行目はヘッダ想定（id,date,value）
    int r;
    for (r = 1; r < rows; ++r) {
        const char* id = csv.Get(r, 0);
        const char* date = csv.Get(r, 1);

        std::string key = make_key_id_date(id, date);
        csv.AddKey(r, key);
    }

    csv.Sort();

    // 検索
    std::string find_key = make_key_id_date(find_id, find_date);
    printf("\nfind_key=\"%s\" (id=%s, date=%s)\n", find_key.c_str(), find_id, find_date);

    // Find（最初の1件）
    {
        int hit = csv.Find(find_key);
        if (hit >= 0) {
            printf("\n[Find] hit row=%d\n", hit);
            // 例：value列（2列目）を表示
            printf("  id=\"");   print_escaped(csv.Get(hit, 0)); printf("\"\n");
            printf("  date=\""); print_escaped(csv.Get(hit, 1)); printf("\"\n");
            printf("  value=\"");print_escaped(csv.Get(hit, 2)); printf("\"\n");
        } else {
            printf("\n[Find] not found\n");
        }
    }

    // FindRows（同一KEY複数行）
    {
        std::vector<int> hitRows;
        int count = csv.FindRows(find_key, hitRows);

        printf("\n[FindRows] count=%d\n", count);
        for (int i = 0; i < count; ++i) {
            int row = hitRows[i];
            printf("  row=%d: id=\"", row);   print_escaped(csv.Get(row, 0)); printf("\"");
            printf(", date=\"");              print_escaped(csv.Get(row, 1)); printf("\"");
            printf(", value=\"");             print_escaped(csv.Get(row, 2)); printf("\"\n");
        }
    }

    printf("\nusage: main_v02_index.exe [csv_path] [id] [date]\n");
    printf("example: main_v02_index.exe test_index.csv 1 20240101\n");

    printf("\n[Enter]で終了します...\n");
    getchar();

    return 0;
}

\
#include <stdio.h>
#include "../src/csv_rescue.h"

// 最小ドライバ（テスト兼）
// ・読み込み
// ・全セル走査して出力（改行を含むセルも確認できるようにする）
static void print_escaped(const char* s) {
    const unsigned char* p = (const unsigned char*)s;
    while (*p) {
        if (*p == '\n') { printf("\\n"); }
        else if (*p == '\r') { printf("\\r"); }
        else if (*p == '\t') { printf("\\t"); }
        else { putchar(*p); }
        ++p;
    }
}

int main(int argc, char** argv) {
    const char* path = "test_broken.csv";
    if (argc >= 2) path = argv[1];

    csvr::CsvRescue csv;
    if (!csv.LoadFromFile(path)) {
        printf("Load failed: %s\n", path);
        return 1;
    }

    int rows = csv.GetRowCount();
    int cols = csv.GetColumnCount();

    printf("rows=%d, max_cols=%d\n", rows, cols);

    int r, c;
    for (r = 0; r < rows; ++r) {
        printf("[row %d]\n", r);
        for (c = 0; c < cols; ++c) {
            const char* v = csv.Get(r, c);
            printf("  col %d = \"", c);
            print_escaped(v);
            printf("\"\n");
        }
    }

	printf("\n[Enter]で終了します...\n");
	getchar();

    return 0;
}

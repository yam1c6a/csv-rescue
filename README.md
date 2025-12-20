# csv-rescue

壊れ気味のCSVを「止まらずに読む」ことだけに特化した、VC++6.0対応の軽量CSVパーサです。
RFC4180準拠や厳密な検証よりも、日本の業務現場で遭遇するCSVデータの救出を優先します。

## csv-rescue 0.1 でできること

- VC++6.0でコンパイル可能（例外・STL依存を最小限）
- CSVファイルを最後まで読み込む（途中で止めない）
- 行ごとの列数が異なっていても読み込む（列揃えや切り捨ては行わない）
  - 足りない列は空文字として扱う
- クオートが壊れていても、できる限り読み進める
  - `"` が閉じていない場合は、その行末までを1セルとして扱う
- クオート内の改行を1セルとして扱う（CRLF / CR / LF 混在可）
- 文字コードは Shift-JIS（Windows-31J / CP932）前提で扱う（0.1）
  - UTF-8 / UTF-16 の自動判定や変換は行いません

※ 書き出し（Save）機能はありません

## 使い方（最小）

```cpp
#include "csv_rescue.h"

csvr::CsvRescue csv;
if (csv.LoadFromFile("test.csv")) {
    int rows = csv.GetRowCount();
    int cols = csv.GetColumnCount(); // 観測上の最大列数
    const char* v = csv.Get(0, 0);   // 範囲外は "" を返す
}
```

## ファイル構成

- `src/csv_rescue.h` / `src/csv_rescue.cpp` : ライブラリ本体
- `test/main.cpp` : 最小ドライバ（テスト兼）
- `test/test.csv` : 例（列ズレ・クオート内改行など）

## ライセンス

MIT License  
Copyright (c) 2025 yamicha

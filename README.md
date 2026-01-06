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

## csv-rescue 0.2 で追加された機能

0.1 の「壊れていても最後まで読む」方針はそのままに、  
**読み込んだ CSV を高速に検索するための仕組み**を追加しました。

### インデックス検索（KEY指定）

- 任意の文字列 KEY を使って行を検索可能
- CSV 本体は並び替えず、別インデックスを構築
- ソート＋二分探索により高速検索
- 同一 KEY が複数行に存在するケースに対応

### 特徴

- VC++6.0 でコンパイル可能
- `std::unordered_map` などの新しすぎる STL は未使用
- KEY の作り方は利用側で自由に定義
- 業務 CSV に多い「重複コード」「履歴行」に対応

## インデックス検索の使い方（v0.2）

### 1. CSVを読み込む

```cpp
csvr::CsvRescue csv;
csv.LoadFromFile("test.csv");
```

### 2. インデックスを構築する

```cpp
csv.ClearIndex();

for (int i = 0; i < csv.GetRowCount(); i++) {
    std::string key =
        csvr::NormalizeNumberPad(csv.Get(i, 1), 6) +
        csvr::PadLeft(csv.Get(i, 2), 8, '0');

    csv.AddKey(i, key);
}

csv.Sort();
```

### 3. 1件検索

```cpp
int row = csv.Find(findKey);
if (row >= 0) {
    const char* v = csv.Get(row, 0);
}
```

### 4. 複数行検索

```cpp
std::vector<int> rows;
int count = csv.FindRows(findKey, rows);

for (int i = 0; i < count; i++) {
    int row = rows[i];
    const char* v = csv.Get(row, 0);
}
```

## ファイル構成

- `src/csv_rescue.h` / `src/csv_rescue.cpp`  
  ライブラリ本体

- `test/main.cpp`  
  読み取り確認用（v0.1相当・壊れたCSVの挙動確認）

- `test/main_v02_index.cpp`  
  インデックス検索確認用（v0.2）

- `test/test.csv`  
  読み取りテスト用CSV（列ズレ・クオート内改行など）

- `test/test_index.csv`  
  インデックス検索テスト用CSV（同一KEY複数行）

## ライセンス

MIT License  
Copyright (c) 2025 yamicha

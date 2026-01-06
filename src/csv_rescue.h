#ifndef CSV_RESCUE_H_INCLUDED
#define CSV_RESCUE_H_INCLUDED

#include <string>
#include <vector>
#include <algorithm>

// csv-rescue v0.1
// 壊れ気味CSVを「止まらずに読む」ための最小CSVパーサ（VC++6.0想定）
// - 例外を投げない（戻り値で成否を返す）
// - 行ごとの列数ズレを許容（足りない列は空文字）
// - クオート内改行を許容
// - 未閉クオートは「行末までを1セル」として扱う
//
// 文字コードは Shift-JIS（ANSI / CP932）前提（0.1）

namespace csvr {

// v0.2 ==========================================================================
std::string PadLeft(const std::string& src, size_t width, char padChar);
std::string PadRight(const std::string& src, size_t width, char padChar);
std::string NormalizeNumber(const std::string& src);
std::string NormalizeNumberPad(const std::string& src, size_t width);


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

// v0.2 ==========================================================================

	// KEYインデックス用要素
	struct KeyIndexItem
	{
		std::string key;
		int row;   // CSV内の行番号
	};

    void ClearIndex();
    void AddKey(int row, const std::string& key);
    void Sort();
    int  Find(const std::string& key) const;
	void FindRange(const std::string& key, int& begin, int& end) const;
	int FindRows(const std::string& key, std::vector<int>& outRows) const;


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


// v0.2 ==========================================================================

	// KEY比較関数（昇順ソート用）
	static bool KeyIndexLess(
		const KeyIndexItem& a,
		const KeyIndexItem& b
	);

    std::vector<KeyIndexItem> m_index;

};

} // namespace csvr

#endif

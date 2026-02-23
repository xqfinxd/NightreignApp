#pragma once
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <SDL_log.h>

// Utility class for reading CSV files
class CsvReader
{
public:
    CsvReader() = default;
    ~CsvReader() = default;

    // Load CSV file from path
    bool load(const std::string& filePath, bool hasHeader = true, char delimiter = ',');
    bool loadFromString(const std::string& csvData, bool hasHeader = true, char delimiter = ',');

    // Get number of rows (excluding header if present)
    size_t getRowCount() const { return m_rows.size(); }

    // Get number of columns
    size_t getColumnCount() const { return m_columnCount; }

    // Get header names (empty if no header)
    const std::vector<std::string>& getHeaders() const { return m_headers; }

    // Get a specific cell value by row and column index
    std::string getValue(size_t row, size_t column) const;

    // Get a specific cell value by row index and column name (requires header)
    std::string getValue(size_t row, const std::string& columnName) const;

    // Get entire row as vector
    std::vector<std::string> getRow(size_t row) const;

    // Get entire column by index
    std::vector<std::string> getColumn(size_t column) const;

    // Get entire column by name (requires header)
    std::vector<std::string> getColumn(const std::string& columnName) const;

    // Get all rows as 2D vector
    const std::vector<std::vector<std::string>>& getAllRows() const { return m_rows; }

    // Check if file was loaded successfully
    bool isLoaded() const { return m_loaded; }

    // Get column index by name (returns -1 if not found)
    int getColumnIndex(const std::string& columnName) const;

private:
    std::vector<std::string> parseLine(const std::string& line, char delimiter);
    std::string trim(const std::string& str);

private:
    bool m_loaded = false;
    bool m_hasHeader = false;
    char m_delimiter = ',';
    size_t m_columnCount = 0;
    std::vector<std::string> m_headers;
    std::vector<std::vector<std::string>> m_rows;
    std::map<std::string, size_t> m_columnMap; // Map column names to indices
};

template<typename T>
std::vector<T> readCSVFile(const std::string& filename, bool skipHeader = true, char delimiter = ',') {
    std::vector<T> rows;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "CsvReader: Cannot open file %s", filename.c_str());
        return rows;
    }

    char bom[3] = { 0 };
    file.read(bom, 3);
    bool isBOM = (bom[0] == char(0xEF) && bom[1] == char(0xBB) && bom[2] == char(0xBF));
    if (!isBOM) file.seekg(0); // No BOM, rewind

    int lineNum = 0;
    while (std::getline(file, line)) {
        lineNum++;

        if (line.empty()) continue;

        if (skipHeader && lineNum == 1) continue;

        T row;
        if (row.parseFromCSV(line, delimiter)) {
            rows.push_back(row);
        }
        else {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "CsvReader: Failed to parse line %s:%d", filename.c_str(), lineNum);
        }
    }

    file.close();
    return rows;
}
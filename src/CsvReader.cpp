#include "CsvReader.h"
#include <SDL_log.h>
#include <algorithm>

bool CsvReader::load(const std::string& filePath, bool hasHeader, char delimiter)
{
    m_loaded = false;
    m_hasHeader = hasHeader;
    m_delimiter = delimiter;
    m_rows.clear();
    m_headers.clear();
    m_columnMap.clear();
    m_columnCount = 0;

    std::ifstream file(filePath);
    if (!file.is_open())
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "CsvReader: Failed to open file: %s", filePath.c_str());
        return false;
    }

    std::string line;
    bool firstLine = true;

    while (std::getline(file, line))
    {
        // Skip empty lines
        if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos)
            continue;

        std::vector<std::string> row = parseLine(line, delimiter);
        
        if (row.empty())
            continue;

        if (firstLine)
        {
            m_columnCount = row.size();
            
            if (hasHeader)
            {
                m_headers = row;
                // Build column name to index map
                for (size_t i = 0; i < m_headers.size(); ++i)
                {
                    m_columnMap[m_headers[i]] = i;
                }
            }
            else
            {
                m_rows.push_back(row);
            }
            
            firstLine = false;
        }
        else
        {
            // Ensure all rows have the same number of columns
            if (row.size() != m_columnCount)
            {
                row.resize(m_columnCount); // Pad with empty strings or truncate
            }
            
            m_rows.push_back(row);
        }
    }

    file.close();

    m_loaded = true;
    SDL_Log("CsvReader: Loaded %s - %zu rows, %zu columns", 
        filePath.c_str(), m_rows.size(), m_columnCount);

    return true;
}

std::vector<std::string> CsvReader::parseLine(const std::string& line, char delimiter)
{
    std::vector<std::string> result;
    std::string cell;
    bool inQuotes = false;

    for (size_t i = 0; i < line.length(); ++i)
    {
        char c = line[i];

        if (c == '"')
        {
            // Toggle quote state
            inQuotes = !inQuotes;
            
            // Handle escaped quotes ("")
            if (inQuotes && i + 1 < line.length() && line[i + 1] == '"')
            {
                cell += '"';
                ++i; // Skip next quote
            }
        }
        else if (c == delimiter && !inQuotes)
        {
            // End of cell
            result.push_back(trim(cell));
            cell.clear();
        }
        else
        {
            cell += c;
        }
    }

    // Add the last cell
    result.push_back(trim(cell));

    return result;
}

std::string CsvReader::trim(const std::string& str)
{
    const char* whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos)
        return "";
    
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::string CsvReader::getValue(size_t row, size_t column) const
{
    if (!m_loaded || row >= m_rows.size() || column >= m_columnCount)
        return "";
    
    if (column >= m_rows[row].size())
        return "";
    
    return m_rows[row][column];
}

std::string CsvReader::getValue(size_t row, const std::string& columnName) const
{
    if (!m_hasHeader)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "CsvReader: Cannot get value by column name without header");
        return "";
    }

    auto it = m_columnMap.find(columnName);
    if (it == m_columnMap.end())
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "CsvReader: Column '%s' not found", columnName.c_str());
        return "";
    }

    return getValue(row, it->second);
}

std::vector<std::string> CsvReader::getRow(size_t row) const
{
    if (!m_loaded || row >= m_rows.size())
        return std::vector<std::string>();
    
    return m_rows[row];
}

std::vector<std::string> CsvReader::getColumn(size_t column) const
{
    std::vector<std::string> result;
    
    if (!m_loaded || column >= m_columnCount)
        return result;
    
    for (const auto& row : m_rows)
    {
        if (column < row.size())
            result.push_back(row[column]);
        else
            result.push_back("");
    }
    
    return result;
}

std::vector<std::string> CsvReader::getColumn(const std::string& columnName) const
{
    if (!m_hasHeader)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "CsvReader: Cannot get column by name without header");
        return std::vector<std::string>();
    }

    auto it = m_columnMap.find(columnName);
    if (it == m_columnMap.end())
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "CsvReader: Column '%s' not found", columnName.c_str());
        return std::vector<std::string>();
    }

    return getColumn(it->second);
}

int CsvReader::getColumnIndex(const std::string& columnName) const
{
    auto it = m_columnMap.find(columnName);
    if (it == m_columnMap.end())
        return -1;
    
    return static_cast<int>(it->second);
}

function getPath(filename)
    local basedir = DATA_PATH or ""
    return basedir .. filename
end

function loadCsv(filename, keyfield)
    local csv = {}
    csv.header = {}
    csv.rows = {}
    local csvFile = io.open(getPath(filename), "r")
    if not csvFile then
        return nil
    end

    for line in csvFile:lines() do
        local keyrow = 1
        if #csv.header == 0 then
            for cell in line:gmatch("([^,]*)") do
                table.insert(csv.header, cell)
                if cell == keyfield then
                    keyrow = #csv.header
                end
            end
        else
            local row = {}
            local col = 1
            for cell in line:gmatch("([^,]*)") do
                local key = csv.header[col]
                row[key] = cell
                if col == keyrow then
                    csv.rows[tonumber(cell) or cell] = row
                end
                col = col + 1
            end
        end
    end
    csvFile:close()
    return csv
end

function table2Csv(tableData, filename)
    assert(type(tableData) == "table", "table2Csv: tableData must be a table")

    local basedir = DATA_PATH or ""
    filename = getPath(filename or "output.csv")
    local csvFile = io.open(filename, "w")
    if not csvFile then
        return false
    end
    
    -- Write header
    local headerWritten = false
    for _, row in pairs(tableData) do
        if not headerWritten then
            local headerLine = ""
            for key, _ in pairs(row) do
                if headerLine ~= "" then
                    headerLine = headerLine .. ","
                end
                headerLine = headerLine .. key
            end
            csvFile:write(headerLine .. "\n")
            headerWritten = true
        end
        -- Write row
        local rowLine = ""
        for key, value in pairs(row) do
            if rowLine ~= "" then
                rowLine = rowLine .. ","
            end
            rowLine = rowLine .. tostring(value)
        end
        csvFile:write(rowLine .. "\n")
    end
    csvFile:close()
    return true
end
function getPath(filename)
    local basedir = DATA_PATH or ""
    return basedir .. filename
end

function isCmdMode()
    return not CPP_ENV
end

local function loadCsv(filename, keyfield)
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

local function table2Csv(tableData, filename, postprocess)
    if not isCmdMode() then
        return false
    end
    assert(type(tableData) == "table", "table2Csv: tableData must be a table")

    local basedir = DATA_PATH or ""
    filename = getPath(filename or "output.csv")
    local csvFile = io.open(filename, "w")
    if not csvFile then
        return false
    end

    if not postprocess then
        postprocess = function(k, v) return tostring(v) end
    end
    
    local headerWritten = false
    for _, row in pairs(tableData) do
        -- Write header
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
            rowLine = rowLine .. postprocess(key, value)
        end
        csvFile:write(rowLine .. "\n")
    end
    csvFile:close()
    return true
end

-- cache of known icon types
local knownIcons = {
    [1] = "site of grace",
    [2] = "church",
    [3] = "ruins",
    [7] = "fort",
    [11] = "castle",
    [13] = "spectral hawk tree",
    [16] = "field boss",
    [17] = "scarab",
    [21] = "merchant",
    [23] = "mine",
    [25] = "scale-bearing merchant",
    [28] = "formidable field boss",
    [30] = "church-completed",
    [60] = "personal objective",
    [61] = "shifting earth power",
    [62] = "buried treasure",
    [137] = "spiritstream",
    [178] = "rope door",
    -- dlc
    [51] = "city rooftop",
    [71] = "great crystal",
    [73] = "divine tower",
    [74] = "rb city",
    [75] = "lt city",
    [76] = "portal",
    [79] = "star merchant", -- undefined
}

local csvModule = {
    loadCsv = loadCsv,
    table2Csv = table2Csv,
}

if isCmdMode() then
    return csvModule
else
    package.preload["csv"] = function()
        return csvModule
    end
end


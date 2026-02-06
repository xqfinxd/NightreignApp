require("schema")

function loadCsv(filename)
    local csv = {}
    csv.header = {}
    local csvFile = io.open(filename, "r")
    if not csvFile then
        return nil
    end
    local lineNumber = -1
    for line in csvFile:lines() do
        if lineNumber < 0 then
            for cell in line:gmatch("([^,]*)") do
                table.insert(csv.header, cell)
            end
        else
            local row = {}
            local cellIndex = 1
            for cell in line:gmatch("([^,]*)") do
                local key = csv.header[cellIndex]
                row[key] = tonumber(cell) or cell
                cellIndex = cellIndex + 1
            end
            table.insert(csv, row)
        end
        lineNumber = lineNumber + 1
    end
    csvFile:close()
    return csv
end

function processRecursively(valueName, valueSchema, row, context, parentnode)
    local valueType = valueSchema.type

    if valueType == "map" and valueSchema.values then
        local curnode = parentnode[valueName] or {}
        local keyField = valueSchema.key
        local key = row[keyField]
        local childnode = curnode[key] or {}
        for childValueName, childValueSchema in pairs(valueSchema.values) do
            processRecursively(childValueName, childValueSchema, row, context, childnode)
        end
        curnode[key] = childnode
        parentnode[valueName] = curnode
        
    elseif valueType == "list" and valueSchema.values then
        local curnode = parentnode[valueName] or {}
        local childnode = {}
        for childValueName, childValueSchema in pairs(valueSchema.values) do
            processRecursively(childValueName, childValueSchema, row, context, childnode)
        end
        table.insert(curnode, childnode)
        parentnode[valueName] = curnode
    
    elseif valueType == "table" then
        local refKey = valueSchema.key
        local refId = row[refKey]
        local refTableName = valueSchema.ref
        table.insert(context.postProcess, function() 
            -- Reference to another table
            if context[refTableName] and refId then
                parentnode[valueName] = context[refTableName][refId]
            end
        end)
    else
        parentnode[valueName] = processValue(valueSchema, row)
    end
end

function processValue(valueSchema, row)
    local valueType = valueSchema.type
    
    if valueType == "number" then
        -- Single number extraction
        local key = valueSchema.key
        return tonumber(row[key]) or 0
        
    elseif valueType == "number[]" then
        -- Array of numbers from multiple keys
        local result = {}
        for _, key in ipairs(valueSchema.key) do
            local value = tonumber(row[key]) or 0
            table.insert(result, value)
        end
        return result
        
    elseif valueType == "function" then
        -- Evaluate a function
        local func = valueSchema.func
        local args = {}
        -- Get argument values from row
        if valueSchema.args then
            for _, argName in ipairs(valueSchema.args) do
                table.insert(args, row[argName])
            end
        end
        -- Load and execute the function
        local fn = load("return " .. func)()
        return fn(table.unpack(args))
        
    end
    
    return nil
end

-- Main function to build data from schema
function buildDataFromSchema(schema, csvSources)
    local context = { postProcess = {} }
    local result = {}
    
    for tableName, tableSchema in pairs(schema) do
        local srcFile = tableSchema.src
        local csvData = csvSources[srcFile]
        
        if not csvData then
            return
        end
        for _, row in ipairs(csvData) do
            processRecursively(tableName, tableSchema, row, context, context)
        end
    end

    for _, fn in ipairs(context.postProcess) do
        fn()
    end
    
    -- Copy context to result
    for tableName, _ in pairs(schema) do
        result[tableName] = context[tableName]
    end
    
    return result
end

-- Build structured data from schema
local csvSources = {
    ["LotResultSmallBaseAndSpot"] = loadCsv("LotResultSmallBaseAndSpot.csv"),
    ["LotResultMapPatternFlag"] = loadCsv("LotResultMapPatternFlag.csv"),
    ["NightBossMenuParam"] = loadCsv("NightBossMenuParam.csv"),
    ["WorldMapPointParam"] = loadCsv("WorldMapPointParam.csv"),
    ["SmallBaseAndSpotAttachPoint"] = loadCsv("SmallBaseAndSpotAttachPoint.csv"),
    ["SmallBaseMapVariationParam"] = loadCsv("SmallBaseMapVariationParam.csv"),
    ["User_SpotDefine"] = loadCsv("User_SpotDefine.csv"),
}

-- Generate the structured data tables
Data = buildDataFromSchema(DataSchema, csvSources)

print(Data.categories[1005].maptype)  -- Example access to the generated data

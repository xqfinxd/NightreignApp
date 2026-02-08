function loadCsv(filename)
    local basedir = DATA_PATH or ""
    filename = basedir .. filename
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
        
    elseif valueType == "string" then
        local key = valueSchema.key
        return row[key] or ""
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

function normalize(point)
    local tileSize = Scene.TEXTURE_TILE_SIZE or 256
    local gridX = point.posX / tileSize + point.gridX - 41
    local gridZ = point.posZ / tileSize + point.gridZ - 35
    return gridX, gridZ
end

function loadSpotsByPattern(patternId)
    if Data == nil then
        -- Build structured data from schema
        local csvSources = {
            ["LotResultSmallBaseAndSpot"] = loadCsv("LotResultSmallBaseAndSpot.csv"),
            ["LotResultMapPatternFlag"] = loadCsv("LotResultMapPatternFlag.csv"),
            ["NightBossMenuParam"] = loadCsv("NightBossMenuParam.csv"),
            ["WorldMapPointParam"] = loadCsv("WorldMapPointParam.csv"),
            ["SmallBaseAndSpotAttachPoint"] = loadCsv("SmallBaseAndSpotAttachPoint.csv"),
            ["SmallBaseMapVariationParam"] = loadCsv("SmallBaseMapVariationParam.csv"),
            ["LotResultPlayAreaParam"] = loadCsv("LotResultPlayAreaParam.csv"),
            ["PlayAreaCreateParam"] = loadCsv("PlayAreaCreateParam.csv"),
            ["User_SpotDefine"] = loadCsv("User_SpotDefine.csv"),
        }
        Data = buildDataFromSchema(DataSchema, csvSources)
    end
    
    Scene:loadMapTiles(Data.categories[patternId].maptype, 0)
    for _, spot in ipairs(Data.patterns[patternId].spots) do
        local point = spot.attachPoint or spot.attachPoint2
        local option = spot.variation and spot.variation.option or {}

        if point and option[1] ~= 180 then
            local x, z = normalize(point)
            local uid = spot.variationId * 10 + spot.variationIndex
            local label = Data.locales[uid] and Data.locales[uid].label or "UNKNOWN"
            if not spot.attachPoint then
                label = "(alt) " .. label
            end
            --Scene:addSpot(x, z, "launch", 0.1, { label = ""..spot.attachId })
        end
    end
    local maptype = Data.categories[patternId].maptype
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
    for id, point in pairs(Data.points) do
        if point.pad == maptype * 10 and not knownIcons[point.worldMapPointIconId] then
            local x, z = normalize(point)

            --Scene:addSpot(x, z, "launch", 0.1, { label = ""..point.worldMapPointIconId })
        end
    end
    local newspots = loadCsv("step-03.csv")
    for _, row in pairs(newspots) do
        local pos = tostring(row["id"])
        local map = tonumber(row["map"])
        local fixed = row["fixed"]
        if map == maptype and fixed == "true" then
            local posvalue = {}
            for ep in pos:gmatch("([^_]*)") do
                table.insert(posvalue, ep)
            end
            local npos = {
                gridX = tonumber(posvalue[1]),
                gridZ = tonumber(posvalue[2]),
                posX = tonumber(posvalue[3]),
                posZ = tonumber(posvalue[4]),
            }
            local x, z = normalize(npos)
            Scene:addSpot(x, z, "launch", 0.1, { label = ""..map })
        end
    end
end

function queryVariation(variationId)
    for patternId, patternData in pairs(Data.patterns) do
        for _, spot in ipairs(patternData.spots) do
            if spot.variationId == variationId then
                return patternId
            end
        end
    end
    return -1
end
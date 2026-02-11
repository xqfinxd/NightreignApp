local csv = require("csv")
local pattern_spots = dofile(getPath("03_list_spots.lua"))
local pattern_mapbase = dofile(getPath("01_list_patterns.lua"))
local spot_variations = dofile(getPath("06_list_variation.lua"))

local function normalize(point)
    local tileSize = Scene.TEXTURE_TILE_SIZE or 256
    local gridX = point.posX / tileSize + point.gridXNo - 41
    local gridZ = point.posZ / tileSize + point.gridZNo - 35
    return gridX, gridZ
end

-- Load variation labels from User_SpotDefine.csv
local variationLabels = {}
local function loadVariationLabels()
    if next(variationLabels) == nil then
        local spotDefine = csv.loadCsv("User_SpotDefine.csv", "ID")
        for id, row in pairs(spotDefine.rows) do
            variationLabels[tonumber(id)] = row.label
        end
    end
    return variationLabels
end

local function getVariationKey(id, type)
    return id * 10 + type
end

local function getVariationLabel(id, type)
    loadVariationLabels()
    local key = getVariationKey(id, type)
    return variationLabels[key] or string.format("Unknown(%d,%d)", id, type)
end

function loadSpotsByPattern(patternId)
    local map = pattern_mapbase[patternId].map
    Scene:loadMapTiles(map, 0)
    
    for _, spot in pairs(pattern_spots) do
        local spotpos = spot.id
        local spotmap = spot.map
        local static = spot.static
        if spotmap == map and static then
            local x, z = normalize(spotpos)
            Scene:addSpot(x, z, "launch", 0.1, { label = ""..tostring(static) })
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

-- Filter mode functions
function loadStaticSpotsByMap(mapIndex)
    Scene:loadMapTiles(mapIndex, 0)
    
    local spots = {}
    
    for _, spot in pairs(pattern_spots) do
        if spot.map == mapIndex and spot.static then
            local x, z = normalize(spot.id)
            local key = string.format("%d_%d_%.2f_%.2f", 
                spot.id.gridXNo, spot.id.gridZNo, spot.id.posX, spot.id.posZ)
            spots[key] = {x = x, z = z, id = spot.id}
            Scene:addSpot(x, z, "launch", 0.1, { label = "", key = key })
        end
    end
    
    return spots
end

function getVariationsAtSpot(mapIndex, spotKey, markedSpots)
    local status, result = pcall(function()
        print("getVariationsAtSpot called: mapIndex=" .. tostring(mapIndex) .. ", spotKey=" .. tostring(spotKey))
        
        if not spot_variations then
            print("ERROR: spot_variations is nil")
            return {}
        end
        
        print("spot_variations loaded, count=" .. #spot_variations)
        
        -- If there are marked spots, first get matching patterns
        local validPatterns = nil
        if markedSpots and next(markedSpots) ~= nil then
            local markedCount = 0
            for _ in pairs(markedSpots) do markedCount = markedCount + 1 end
            print("Filtering by " .. tostring(markedCount) .. " marked spots")
            
            -- Build pattern spots map
            local patternSpots = {}
            for _, variation in ipairs(spot_variations) do
                if variation.map == mapIndex and variation.location then
                    local key = string.format("%d_%d_%.2f_%.2f",
                        variation.location.gridXNo, variation.location.gridZNo,
                        variation.location.posX, variation.location.posZ)
                    
                    local patternId = variation.pattern
                    if not patternSpots[patternId] then
                        patternSpots[patternId] = {}
                    end
                    
                    if not patternSpots[patternId][key] then
                        patternSpots[patternId][key] = {}
                    end
                    
                    local varKey = getVariationKey(variation.id, variation.type)
                    table.insert(patternSpots[patternId][key], varKey)
                end
            end
            
            -- Filter patterns matching all marked spots
            validPatterns = {}
            for patternId, spots in pairs(patternSpots) do
                local matches = true
                
                for markedSpotKey, requiredVarKey in pairs(markedSpots) do
                    local spotHasVariation = false
                    if spots[markedSpotKey] then
                        for _, varKey in ipairs(spots[markedSpotKey]) do
                            if varKey == requiredVarKey then
                                spotHasVariation = true
                                break
                            end
                        end
                    end
                    
                    if not spotHasVariation then
                        matches = false
                        break
                    end
                end
                
                if matches then
                    validPatterns[patternId] = true
                end
            end
            
            local validCount = 0
            for _ in pairs(validPatterns) do validCount = validCount + 1 end
            print("Valid patterns after filtering: " .. tostring(validCount))
        end
        
        local variations = {}
        local seen = {}
        
        for _, variation in ipairs(spot_variations) do
            if variation.map == mapIndex and variation.location then
                local key = string.format("%d_%d_%.2f_%.2f",
                    variation.location.gridXNo, variation.location.gridZNo,
                    variation.location.posX, variation.location.posZ)
                
                if key == spotKey then
                    -- If we have valid patterns filter, only include variations from those patterns
                    if validPatterns == nil or validPatterns[variation.pattern] then
                        local varKey = getVariationKey(variation.id, variation.type)
                        if not seen[varKey] then
                            seen[varKey] = true
                            table.insert(variations, {
                                id = variation.id,
                                type = variation.type,
                                key = varKey,
                                label = getVariationLabel(variation.id, variation.type),
                                patterns = {}
                            })
                        end
                        
                        -- Add pattern to this variation's list
                        for _, v in ipairs(variations) do
                            if v.key == varKey then
                                table.insert(v.patterns, variation.pattern)
                                break
                            end
                        end
                    end
                end
            end
        end
        
        print("Returning " .. #variations .. " variations")
        return variations
    end)
    
    if not status then
        print("ERROR in getVariationsAtSpot: " .. tostring(result))
        return {}
    end
    
    return result
end

function filterPatternsByMarkedSpots(mapIndex, markedSpots)
    -- Build a map of pattern -> spots with variation keys
    local patternSpots = {}
    for _, variation in ipairs(spot_variations) do
        if variation.map == mapIndex and variation.location then
            local spotKey = string.format("%d_%d_%.2f_%.2f",
                variation.location.gridXNo, variation.location.gridZNo,
                variation.location.posX, variation.location.posZ)
            
            local patternId = variation.pattern
            if not patternSpots[patternId] then
                patternSpots[patternId] = {}
            end
            
            if not patternSpots[patternId][spotKey] then
                patternSpots[patternId][spotKey] = {}
            end
            
            local varKey = getVariationKey(variation.id, variation.type)
            table.insert(patternSpots[patternId][spotKey], varKey)
        end
    end
    
    -- Filter patterns that match ALL marked spots
    local matchingPatterns = {}
    for patternId, spots in pairs(patternSpots) do
        local matches = true
        
        for spotKey, requiredVarKey in pairs(markedSpots) do
            local spotHasVariation = false
            if spots[spotKey] then
                for _, varKey in ipairs(spots[spotKey]) do
                    if varKey == requiredVarKey then
                        spotHasVariation = true
                        break
                    end
                end
            end
            
            if not spotHasVariation then
                matches = false
                break
            end
        end
        
        if matches then
            table.insert(matchingPatterns, patternId)
        end
    end
    
    table.sort(matchingPatterns)
    return matchingPatterns
end
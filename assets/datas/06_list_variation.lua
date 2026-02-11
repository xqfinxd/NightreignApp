local csv = require("csv")

local spotbase = csv.loadCsv("LotResultSmallBaseAndSpot.csv", "ID")
local pointbase = csv.loadCsv("WorldMapPointParam.csv", "ID")
local altpointbase = csv.loadCsv("SmallBaseAndSpotAttachPoint.csv", "ID")
local pattern_mapbase = dofile(getPath("01_list_patterns.lua"))

local function getLocation(attachId)
    local point = pointbase.rows[attachId] or altpointbase.rows[attachId]
    if point then
        return {
            gridXNo = tonumber(point.gridXNo),
            gridZNo = tonumber(point.gridZNo),
            posX = tonumber(point.posX),
            posZ = tonumber(point.posZ)
        }
    else
        return nil
    end
end

local spot_variations = {}
for _, row in pairs(spotbase.rows) do
    local patternId = tonumber(row["patternId"])
    local map = pattern_mapbase[patternId].map
    assert(map ~= nil)
    local attachId = tonumber(row["attachId"])
    
    table.insert(spot_variations, {
        id = tonumber(row["smallBaseMapId"]),
        type = tonumber(row["variationId"]),
        index = tonumber(row["mapIndex"]),
        location = getLocation(attachId),
        map = map,
        pattern = patternId,
    })
end

csv.table2Csv(spot_variations, "step-06.csv", function(k, v)
    if k == "location" and v then
        return string.format("%d_%d_%.2f_%.2f", v.gridXNo, v.gridZNo, v.posX, v.posZ)
    else
        return tostring(v)
    end
end)

return spot_variations
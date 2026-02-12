local csv = require("csv")

local flagbase = csv.loadCsv("LotResultMapPatternFlag.csv", "ID")
local createbase = csv.loadCsv("PlayAreaCreateParam.csv", "ID")
local areabase = csv.loadCsv("LotResultPlayAreaParam.csv", "ID")

local pattern_mapbase = {}
for _, row in pairs(flagbase.rows) do
    local patternId = tonumber(row["patternId"])
    if pattern_mapbase[patternId] == nil then
        local pattern = {}
        pattern.id = patternId

        pattern.map = tonumber(row["rareMap"])
        pattern.boss = tonumber(row["targetBoss"])
        pattern.isdlc = tonumber(row["patternSetId"]) >= 1000

        pattern_mapbase[patternId] = pattern
    end
end

local function findArea(areaId)
    local area = createbase.rows[areaId]
    if area == nil then
        return {}
    else
        return {
            gridXNo = tonumber(area["gridXNo"]),
            gridZNo = tonumber(area["gridZNo"]),
            posX = tonumber(area["posX"]),
            posZ = tonumber(area["posZ"]),
        }
    end
end

for _, row in pairs(areabase.rows) do
    local patternId = tonumber(row["patternId"])
    local mapbase = pattern_mapbase[patternId]
    if mapbase ~= nil then
        mapbase.id = patternId

        local playArea1 = findArea(tonumber(row["playArea1"]))
        mapbase.playArea1_gridXNo = playArea1.gridXNo or 0
        mapbase.playArea1_gridZNo = playArea1.gridZNo or 0
        mapbase.playArea1_posX = playArea1.posX or 0
        mapbase.playArea1_posZ = playArea1.posZ or 0

        local playArea2 = findArea(tonumber(row["playArea2"]))
        mapbase.playArea2_gridXNo = playArea2.gridXNo or 0
        mapbase.playArea2_gridZNo = playArea2.gridZNo or 0
        mapbase.playArea2_posX = playArea2.posX or 0
        mapbase.playArea2_posZ = playArea2.posZ or 0

        mapbase.bossId1 = tonumber(row["bossId1"]) or 0
        mapbase.bossId2 = tonumber(row["bossId2"]) or 0
        mapbase.extraBossId1 = tonumber(row["extraBossId1"]) or 0
        mapbase.extraBossId2 = tonumber(row["extraBossId2"]) or 0
    end
end

csv.table2Csv(pattern_mapbase, "autogen_pattern_list.csv") 
return pattern_mapbase
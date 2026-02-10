require("csv")

local createbase = loadCsv("PlayAreaCreateParam.csv", "ID")
local areabase = loadCsv("LotResultPlayAreaParam.csv", "ID")

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

local pattern_nights = {}
for _, row in pairs(areabase.rows) do
    local patternId = tonumber(row["patternId"])
    if pattern_nights[patternId] == nil then
        local night = {}
        night.id = patternId

        local playArea1 = findArea(tonumber(row["playArea1"]))
        night.playArea1_gridXNo = playArea1.gridXNo or 0
        night.playArea1_gridZNo = playArea1.gridZNo or 0
        night.playArea1_posX = playArea1.posX or 0
        night.playArea1_posZ = playArea1.posZ or 0

        local playArea2 = findArea(tonumber(row["playArea2"]))
        night.playArea2_gridXNo = playArea2.gridXNo or 0
        night.playArea2_gridZNo = playArea2.gridZNo or 0
        night.playArea2_posX = playArea2.posX or 0
        night.playArea2_posZ = playArea2.posZ or 0

        night.bossId1 = tonumber(row["bossId1"]) or 0
        night.bossId2 = tonumber(row["bossId2"]) or 0
        night.extraBossId1 = tonumber(row["extraBossId1"]) or 0
        night.extraBossId2 = tonumber(row["extraBossId2"]) or 0

        pattern_nights[patternId] = night
    end
end

table2Csv(pattern_nights, "step-02.csv")

return pattern_nights
require("csv")

local flagbase = loadCsv("LotResultMapPatternFlag.csv", "ID")
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

table2Csv(pattern_mapbase, "step-01.csv") 

return pattern_mapbase
require("csv")

local flags = loadCsv("LotResultMapPatternFlag.csv", "ID")
local patterns = {}
for _, row in pairs(flags.rows) do
    local patternId = tonumber(row["patternId"])
    if patterns[patternId] == nil then
        local pattern = {}
        pattern.id = patternId

        pattern.map = tonumber(row["rareMap"])
        pattern.boss = tonumber(row["targetBoss"])
        pattern.isdlc = tonumber(row["patternSetId"]) >= 1000

        patterns[patternId] = pattern
    end
end

table2Csv(patterns, "step-01.csv") 

return patterns
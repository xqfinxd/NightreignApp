
require("csv")

function print_starter_dist()
    local flagbase = loadCsv("LotResultMapPatternFlag.csv", "ID")
    local staters = {}
    for _, row in pairs(flagbase.rows) do
        local map = tonumber(row["rareMap"])
        local flag = tonumber(row["modifierSet"])
        if flag == 190 or flag == 160 then
            local starter = staters[map] or {}
            local starterid = tonumber(row["modifier"])
            
            local num = starter[starterid] or 0
            starter[starterid] = num + 1
            
            staters[map] = starter
        end
    end

    for map, ids in pairs(staters) do
        local sets = {}
        for id, num in pairs(ids) do
            if num > 0 then
                table.insert(sets, id)
            end
        end
        print(string.format("Map %d: Starters %s", map, table.concat(sets, ", ")))
    end
end

local starter_dist = {
    [1] = { 705, 706, 707, 708, 700, 701, 702, 703, 704 },
    [2] = { 700, 708, 707, 704, 705, 706, 701 },
    [3] = { 700, 708, 702, 704, 705, 701, 707 },
    [4] = { 13002, 13000, 13001 },
    [5] = { 707, 708, 702, 703, 704, 705, 706 },
}

local starter_locations = {
    [13000] = {42,36,28.44,88.18},
    [13002] = {45,39,-29.87,-76.80},
    [13001] = {43,36,58.74,-64.57},
    [700] = {42,36,-74.00,62.00},
    [701] = {42,37,-66.00,44.00},
    [702] = {42,38,-48.00,98.46},
    [703] = {43,38,-86.00,32.00},
    [704] = {44,36,-22.00,-72.00},
    [705] = {44,37,-52.00,-76.00},
    [706] = {44,39,-72.00,44.00},
    [707] = {45,37,86.00,4.00},
    [708] = {45,38,-86.00,81.00},
}

return starter_dist, starter_locations
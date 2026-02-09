
require("csv")

function print_starter_dist()
    local flags = loadCsv("LotResultMapPatternFlag.csv", "ID")
    local staters = {}
    for _, row in pairs(flags.rows) do
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
    [13000] = "42_36_28.44_88.18",
    [13002] = "45_39_-29.87_-76.80",
    [13001] = "43_36_58.74_-64.57",
    [700] = "42_36_-74.00_62.00",
    [701] = "42_37_-66.00_44.00",
    [702] = "42_38_-48.00_98.46",
    [703] = "43_38_-86.00_32.00",
    [704] = "44_36_-22.00_-72.00",
    [705] = "44_37_-52.00_-76.00",
    [706] = "44_39_-72.00_44.00",
    [707] = "45_37_86.00_4.00",
    [708] = "45_38_-86.00_81.00",
}

return starter_dist, starter_locations
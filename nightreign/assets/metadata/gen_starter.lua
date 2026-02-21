local csv = require("csv")

function print_starter_dist()
    local flagbase = csv.loadCsv("LotResultMapPatternFlag.csv", "ID")
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

local starter_distribution = {
    {id=0, starters = { 700, 701, 702, 703, 704, 705, 706, 707, 708 },},
    {id=1, starters = { 700, 701, 704, 705, 706, 707, 708 },},
    {id=2, starters = { 700, 701, 702, 704, 705, 707, 708 },},
    {id=3, starters = { 700, 701, 702, 703, 706, 708 },},
    {id=4, starters = { 13002, 13000, 13001 },},
    {id=5, starters = { 702, 703, 704, 705, 706, 707, 708 },},
}

local starter_list = {
    {id=13000, gridXNo=42, gridZNo=36, posX=28.44, posZ=88.18},
    {id=13002, gridXNo=45, gridZNo=39, posX=-29.87, posZ=-76.80},
    {id=13001, gridXNo=43, gridZNo=36, posX=58.74, posZ=-64.57},
    {id=700, gridXNo=42, gridZNo=36, posX=-74.00, posZ=62.00},
    {id=701, gridXNo=42, gridZNo=37, posX=-66.00, posZ=44.00},
    {id=702, gridXNo=42, gridZNo=38, posX=-48.00, posZ=98.46},
    {id=703, gridXNo=43, gridZNo=38, posX=-86.00, posZ=32.00},
    {id=704, gridXNo=44, gridZNo=36, posX=-22.00, posZ=-72.00},
    {id=705, gridXNo=44, gridZNo=37, posX=-52.00, posZ=-76.00},
    {id=706, gridXNo=44, gridZNo=39, posX=-72.00, posZ=44.00},
    {id=707, gridXNo=45, gridZNo=37, posX=86.00, posZ=4.00},
    {id=708, gridXNo=45, gridZNo=38, posX=-86.00, posZ=81.00},
}

csv.table2Csv(starter_distribution, "autogen_starter_distribution.csv", function(k, v)
    if k == "starters" then
        return table.concat(v, "_")
    else
        return tostring(v)
    end
end)
csv.table2Csv(starter_list, "autogen_starter_list.csv")
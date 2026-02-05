function loadCsv(filename)
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
                row[key] = cell
                cellIndex = cellIndex + 1
            end
            table.insert(csv, row)
        end
        lineNumber = lineNumber + 1
    end
    csvFile:close()
    return csv
end

LotResultSmallBaseAndSpot = loadCsv("LotResultSmallBaseAndSpot.csv")
LotResultMapPatternFlag = loadCsv("LotResultMapPatternFlag.csv")
NightBossMenuParam = loadCsv("NightBossMenuParam.csv")
SmallBaseAndSpotAttachPoint = loadCsv("SmallBaseAndSpotAttachPoint.csv")
SmallBaseMapVariationParam = loadCsv("SmallBaseMapVariationParam.csv")
WorldMapPointParam = loadCsv("WorldMapPointParam.csv")
User_SpotDefine = loadCsv("User_SpotDefine.csv")

for i, spot in ipairs(LotResultSmallBaseAndSpot) do
    --print(spot.ID, spot.modifier)
end
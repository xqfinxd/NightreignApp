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
                row[key] = tonumber(cell) or cell
                cellIndex = cellIndex + 1
            end
            table.insert(csv, row)
        end
        lineNumber = lineNumber + 1
    end
    csvFile:close()
    return csv
end

-- All spots data
LotResultSmallBaseAndSpot = loadCsv("LotResultSmallBaseAndSpot.csv")
-- pattern flag data
LotResultMapPatternFlag = loadCsv("LotResultMapPatternFlag.csv")
-- night boss data
NightBossMenuParam = loadCsv("NightBossMenuParam.csv")
-- spot location data
WorldMapPointParam = loadCsv("WorldMapPointParam.csv")
-- spot location data (unreliable)
SmallBaseAndSpotAttachPoint = loadCsv("SmallBaseAndSpotAttachPoint.csv")
-- spot attachment data
SmallBaseMapVariationParam = loadCsv("SmallBaseMapVariationParam.csv")
-- attachment translation data
User_SpotDefine = loadCsv("User_SpotDefine.csv")

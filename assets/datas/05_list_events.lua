require("csv")

-- Event Flags: 7705, 7725
local InvasionEvents = {
    [510]  = "Caligo Blizzard",
    [520]  = "Gladius Invasion",
    [540]  = "Maris Bubbles",
    [550]  = "Gnoster Plague",
    [560]  = "Libra Curse",

    [3040] = "Morgott Invasion",
    [3050] = "Maris Bubbles",
    [3060] = "Gnoster Plague",
    [3070] = "Libra Curse",
    [3110] = "Caligo Blizzard",
    [3120] = "Gladius Invasion",
    [3130] = "Balancers Raid",
}

local EventDefinitions = {
    [7704] = "Day 1".."Night Horde",
    [7724] = "Day 2".."Night Horde",

    [7701] = "Day 1".."Meteor Strike",
    [7721] = "Day 2".."Meteor Strike",

    [7702] = "Walking Mausoleum",
    [7722] = "Walking Mausoleum",

    [7700] = "Day 1".."Extra Night Boss",
    [7720] = "Day 2".."Extra Night Boss",

    [7707] = "Frenzy Tower",
    [7727] = "Frenzy Tower",

    [7706] = "Difficult Sorcerer's Rise",
    [7726] = "Difficult Sorcerer's Rise",

    -- [8075] = "Morgott Invasion",
    -- [8076] = "Maris Bubbles",
    -- [8077] = "Gladius Invasion",
    -- [8078] = "Gnoster Plague",
    -- [8079] = "Libra Curse",
    -- [8080] = "Caligo Blizzard",
    -- [8081] = "Balancers Raid",
    [7705] = function(modifier, modifierSet) return "Day 1"..InvasionEvents[modifierSet] end,
    [7725] = function(modifier, modifierSet) return "Day 2"..InvasionEvents[modifierSet] end,
}

local flagsortbase = loadCsv("LotBaseMapPatternFlag.csv", "ID")
local flagdistbase = loadCsv("LotResultMapPatternFlag.csv", "ID")

local function tableInsertUnique(t, value)
    for _, v in ipairs(t) do
        if v == value then
            return
        end
    end
    table.insert(t, value)
end

local flag_filters = {}
for _, row in pairs(flagsortbase.rows) do
    local eventFlag = tonumber(row["eventFlag"])
    if eventFlag ~= 0 then
        local filters = flag_filters[eventFlag] or {}
        table.insert(filters, {
            require1 = tonumber(row["requireModifier1"]),
            require2 = tonumber(row["requireModifier2"]),
            exclude1 = tonumber(row["excludeModifier1"]),
            exclude2 = tonumber(row["excludeModifier2"]),
            modifierSet = tonumber(row["modifierSet"]),
            modifier = tonumber(row["modifier"]),
        })
        flag_filters[eventFlag] = filters
    end
end

local pattern_flags = {}
for _, row in pairs(flagdistbase.rows) do
    local patternId = tonumber(row["patternId"])
    local pattern = pattern_flags[patternId] or { id = patternId, modifiers = {}, flags = {} }
    
    local modifier = tonumber(row["modifier"])
    if modifier ~= 0 then
        pattern.modifiers[modifier] = tonumber(row["modifierSet"])
    end
    tableInsertUnique(pattern.flags, tonumber(row["eventFlag"]))

    pattern_flags[patternId] = pattern
end

local pattern_events = {}
for patternId, data in pairs(pattern_flags) do
    local patternEventFlags = {}
    for _, eventFlag in ipairs(data.flags) do
        local filterSet = flag_filters[eventFlag]
        if filterSet then
            for _, filter in ipairs(filterSet) do
                local match = true
                if filter.require1 ~= 0 and not data.modifiers[filter.require1] then
                    match = false
                end
                if filter.require2 ~= 0 and not data.modifiers[filter.require2] then
                    match = false
                end
                if filter.exclude1 ~= 0 and data.modifiers[filter.exclude1] then
                    match = false
                end
                if filter.exclude2 ~= 0 and data.modifiers[filter.exclude2] then
                    match = false
                end
                local isInvasion = (eventFlag == 7705 or eventFlag == 7725)
                if isInvasion then
                    if match and filter.modifierSet == data.modifiers[filter.modifier] then
                        table.insert(patternEventFlags, {
                            eventFlag = eventFlag,
                            modifierSet = filter.modifierSet,
                            modifier = filter.modifier,
                        })
                        break
                    end
                else
                    if match then
                        table.insert(patternEventFlags, {
                            eventFlag = eventFlag,
                        })
                        break
                    end
                end
            end
        end
    end
    for _, eventData in ipairs(patternEventFlags) do
        local definition = EventDefinitions[eventData.eventFlag]
        local eventName = nil
        if type(definition) == "function" then
            eventName = definition(eventData.modifier, eventData.modifierSet)
        else
            eventName = definition
        end
        if eventName then
            pattern_events[patternId] = { id = patternId, eventFlag = eventData.eventFlag, event = eventName }
        end
    end
end

table2Csv(pattern_events, "step-05.csv")
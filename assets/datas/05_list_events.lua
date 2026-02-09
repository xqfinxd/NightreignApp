require("csv")
local flags = {
    starter = {160, 190},
    -- "modifierSet" stands for special event types
    -- In LotBaseMapPatternFlag.csv, these modifierSet treat as key, then filtering eventFlag
    extra_night_boss = {"modifierSet", 3000, 505},
    walking_mausoleum = {"modifierSet", 3020},
    frenzy_tower = {"modifierSet", 3500, 1000},
    difficult_sorcerers_rise = {"modifierSet", 3090},
    night_horde = {"modifierSet", 3030},
    meteor_strike = {"modifierSet", 3010},
    morgott_invasion = {"modifierSet", 3040},
    maris_bubbles = {"modifierSet", 3050, 540},
    gnoster_plague = {"modifierSet", 3060, 550},
    libra_curse = {"modifierSet", 3070, 560},
    caligo_blizzard = {"modifierSet", 3110, 510},
    gladius_invasion = {"modifierSet", 3120, 520},
    balancers_raid = {"modifierSet", 3130},
    buried_treasure = {800},
}
local FlagDefinitions = {
    [505]  = "Extra Night Boss",
    [1000] = "Frenzy Tower",
    [540]  = "Maris Bubbles",
    [550]  = "Gnoster Plague",
    [560]  = "Libra Curse",
    [510]  = "Caligo Blizzard",
    [520]  = "Gladius Invasion",
    [3000] = "Extra Night Boss",
    [3020] = "Walking Mausoleum",
    [3500] = "Frenzy Tower",
    [3090] = "Difficult Sorcerer's Rise",
    [3030] = "Night Horde",
    [3010] = "Meteor Strike",
    [3040] = "Morgott Invasion",
    [3050] = "Maris Bubbles",
    [3060] = "Gnoster Plague",
    [3070] = "Libra Curse",
    [3110] = "Caligo Blizzard",
    [3120] = "Gladius Invasion",
    [3130] = "Balancers Raid",
    --[800]  = "Buried Treasure",
    --[160] = "Starter",
    --[190] = "Starter",
}

local lots = loadCsv("LotBaseMapPatternFlag.csv", "ID")
local flags = loadCsv("LotResultMapPatternFlag.csv", "ID")
local patterns = {}
for _, row in pairs(flags.rows) do
    local patternId = tonumber(row["patternId"])
    local pattern = patterns[patternId] or { id = patternId, modifiers = {}, flags = {}, groups = {} }
    
    pattern.modifiers[tonumber(row["modifier"])] = true
    pattern.groups[tonumber(row["modifierSet"])] = true
    table.insert(pattern.flags, tonumber(row["eventFlag"]))

    patterns[patternId] = pattern
end

local cache = {}
for _, row in pairs(lots.rows) do
    local flagId = tonumber(row["eventFlag"])
    local flag = cache[flagId] or {}
    table.insert(flag, {
        require1 = tonumber(row["requireModifier1"]),
        require2 = tonumber(row["requireModifier2"]),
        exclude1 = tonumber(row["excludeModifier1"]),
        exclude2 = tonumber(row["excludeModifier2"]),
        modifierSet = tonumber(row["modifierSet"]),
        modifier = tonumber(row["modifier"]),
    })
    cache[flagId] = flag
end

for eventFlag, conditions in pairs(cache) do
    for _, condition in ipairs(conditions) do
        
    end
end

for patternId, data in pairs(patterns) do
    for _, eventFlag in ipairs(data.flags) do
        local conditionSet = cache[eventFlag]
        if conditionSet then
            for _, condition in ipairs(conditionSet) do
                local match = true
                if condition.require1 ~= 0 and not data.modifiers[condition.require1] then
                    match = false
                end
                if condition.require2 ~= 0 and not data.modifiers[condition.require2] then
                    match = false
                end
                if condition.exclude1 ~= 0 and data.modifiers[condition.exclude1] then
                    match = false
                end
                if condition.exclude2 ~= 0 and data.modifiers[condition.exclude2] then
                    match = false
                end

                if match then
                    data.groups[condition.modifierSet] = condition.modifier
                end
            end
        end
    end
end

for patternId, data in pairs(patterns) do
    local sets = {}
    for modifierSet, modifier in pairs(data.groups) do
        local eventDef = FlagDefinitions[modifierSet]
        if modifier and eventDef then
            table.insert(sets, eventDef)
        end
    end
    print(string.format("Pattern %d has event sets: %s", patternId, table.concat(sets, ", ")))
end

return flags
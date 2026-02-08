-- NEVER NEST LISTS OR MAPS INSIDE LIST
DataSchema = {
    patterns = {
        src = "LotResultSmallBaseAndSpot",
        type = "map",
        key = "patternId",
        values = {
            spots = {
                type = "list",
                values = {
                    id = {
                        type = "number",
                        key = "ID",
                    },
                    variationId = {
                        type = "number",
                        key = "smallBaseMapId",
                    },
                    index = {
                        type = "number",
                        key = "mapIndex"
                    },
                    attachId = {
                        type = "number",
                        key = "attachId",
                    },
                    attachPoint = {
                        type = "table",
                        key = "attachId",
                        ref = "points"
                    },
                    attachPoint2 = {
                        type = "table",
                        key = "attachId",
                        ref = "points2"
                    },
                    variationIndex = {
                        type = "number",
                        key = "variationId",
                    },
                    variation = {
                        type = "table",
                        key = "smallBaseMapId",
                        ref = "variations",
                    },
                },
            },
        },
    },
    categories = {
        src = "LotResultMapPatternFlag",
        type = "map",
        key = "patternId",
        values = {
            datas = {
                type = "list",
                values = {
                    patternSetId = {
                        type = "number",
                        key = "patternSetId",
                    },
                    eventFlag = {
                        type = "number",
                        key = "eventFlag",
                    },
                    modifierSet = {
                        type = "number",
                        key = "modifierSet",
                    },
                    modifier = {
                        type = "number",
                        key = "modifier",
                    },
                },
            },
            maptype = {
                type = "function",
                func = "function(v) return v % 10 end",
                args = {
                    "patternSetId",
                },
            },
            nightlord = {
                type = "function",
                func = "function(v) return math.floor((v % 100) / 10) end",
                args = {
                    "patternSetId",
                },
            },
        },
    },
    points = {
        src = "WorldMapPointParam",
        type = "map",
        key = "ID",
        values = {
            gridX = {
                type = "number",
                key = "gridXNo",
            },
            gridZ = {
                type = "number",
                key = "gridZNo",
            },
            posX = {
                type = "number",
                key = "posX",
            },
            posZ = {
                type = "number",
                key = "posZ",
            },
            pad = {
                type = "number",
                key = "pad",
            },
            worldMapPointIconId = {
                type = "number",
                key = "worldMapPointIconId",
            },
        },
    },
    points2 = {
        src = "SmallBaseAndSpotAttachPoint",
        type = "map",
        key = "ID",
        values = {
            gridX = {
                type = "number",
                key = "gridXNo",
            },
            gridZ = {
                type = "number",
                key = "gridZNo",
            },
            posX = {
                type = "number",
                key = "posX",
            },
            posZ = {
                type = "number",
                key = "posZ",
            },
            pad = {
                type = "number",
                key = "pad",
            },
        },
    },
    variations = {
        src = "SmallBaseMapVariationParam",
        type = "map",
        key = "ID",
        values = {
            option = {
                type = "number[]",
                key = {
                    "modifier1",
                    "modifier2",
                    "modifier3",
                }
            },
            index = {
                type = "number[]",
                key = {
                    "variationValue_0",
                    "variationValue_1",
                    "variationValue_2",
                    "variationValue_3",
                    "variationValue_4",
                    "variationValue_5",
                    "variationValue_6",
                    "variationValue_7",
                    "variationValue_8",
                    "variationValue_9",
                    "variationValue_10",
                },
            },
        },
    },
    locales = {
        src = "User_SpotDefine",
        type = "map",
        key = "ID",
        values = {
            label = {
                type = "string",
                key = "label",
            },
        },
    },
}
#include "MetaData.h"
#include "CsvReader.h"
#include <SDL_log.h>
#include <SDL_assert.h>
#include <algorithm>
#include <set>

void MetaData::load()
{
    CsvReader lrSpotCsv;
    if (!lrSpotCsv.load("nightreign/assets/datas/LotResultSmallBaseAndSpot.csv", true))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "MetaData: Failed to load LotResultSmallBaseAndSpot.csv");
        return;
    }
    std::unordered_map<PatternID, PatternMetaData> patternSpotMetaDatas;
    for(auto& row : lrSpotCsv.getAllRows())
    {
        PatternID patternId = std::stoi(row[lrSpotCsv.getColumnIndex("patternId")]);
        auto& patternMetaData = patternSpotMetaDatas[patternId];
        auto spotId = std::stoi(row[lrSpotCsv.getColumnIndex("ID")]);
        auto& spotMetaData = patternMetaData.spotMetaDatas[spotId];

        spotMetaData.pointID = std::stoi(row[lrSpotCsv.getColumnIndex("attachId")]);
        spotMetaData.variationID = std::stoi(row[lrSpotCsv.getColumnIndex("smallBaseMapId")]);
        spotMetaData.variantIndex = std::stoi(row[lrSpotCsv.getColumnIndex("variationId")]);
        spotMetaData.mapIndex = std::stoi(row[lrSpotCsv.getColumnIndex("mapIndex")]);
        spotMetaData.modifier = std::stoi(row[lrSpotCsv.getColumnIndex("modifier")]);
    }

    CsvReader spotPointCsv;
    if (!spotPointCsv.load("nightreign/assets/datas/WorldMapPointParam.csv", true))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "MetaData: Failed to load WorldMapPointParam.csv");
        return;
    }
    std::unordered_map<int, size_t> spotPointMap;
    for (size_t i = 0; i < spotPointCsv.getRowCount(); ++i)
    {
        int id = std::stoi(spotPointCsv.getValue(i, "ID"));
        spotPointMap[id] = i;
    }

    CsvReader spotPoint2Csv;
    if (!spotPoint2Csv.load("nightreign/assets/datas/SmallBaseAndSpotAttachPoint.csv", true))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "MetaData: Failed to load SmallBaseAndSpotAttachPoint.csv");
        return;
    }
    std::unordered_map<int, size_t> spotPoint2Map;
    for (size_t i = 0; i < spotPoint2Csv.getRowCount(); ++i)
    {
        int id = std::stoi(spotPoint2Csv.getValue(i, "ID"));
        spotPoint2Map[id] = i;
    }

    CsvReader mapVariationCsv;
    if (!mapVariationCsv.load("nightreign/assets/datas/SmallBaseMapVariationParam.csv", true))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "MetaData: Failed to load SmallBaseMapVariationParam.csv");
        return;
    }
    std::unordered_map<int, size_t> mapVariationMap;
    for (size_t i = 0; i < mapVariationCsv.getRowCount(); ++i)
    {
        int id = std::stoi(mapVariationCsv.getValue(i, "ID"));
        mapVariationMap[id] = i;
    }

    CsvReader spotLabelCsv;
    if (!spotLabelCsv.load("nightreign/assets/datas/User_SpotDefine.csv", true))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "MetaData: Failed to load User_SpotDefine.csv");
        return;
    }
    std::unordered_map<int, std::string> spotLabelMap;
    for (size_t i = 0; i < spotLabelCsv.getRowCount(); ++i)
    {
        int id = std::stoi(spotLabelCsv.getValue(i, "ID"));
        std::string label = spotLabelCsv.getValue(i, "label");
        spotLabelMap[id] = label;
    }

    CsvReader lrPatternCsv;
    if (!lrPatternCsv.load("nightreign/assets/datas/LotResultMapPatternFlag.csv", true))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "MetaData: Failed to load LotResultMapPatternFlag.csv");
        return;
    }
    for(auto& row : lrPatternCsv.getAllRows())
    {
        PatternID patternId = std::stoi(row[lrPatternCsv.getColumnIndex("patternId")]);
        NightlordID nightlordId = std::stoi(row[lrPatternCsv.getColumnIndex("targetBoss")]);
        MapID mapId = std::stoi(row[lrPatternCsv.getColumnIndex("rareMap")]);
        
        PatternData& patternData = patterns[patternId];
        if( patternData.nightlord == -1 && patternData.map == -1)
        {
            patternData.nightlord = nightlordId;
            patternData.map = mapId;
        }
        else
        {
            SDL_assert(patternData.nightlord == nightlordId && patternData.map == mapId);
        }
    }

    for(const auto& metaPair : patternSpotMetaDatas)
    {
        PatternID patternId = metaPair.first;
        auto patternIt = patterns.find(patternId);
        SDL_assert(patternIt != patterns.end());
        
        std::unordered_map<int, AttachPointID> nightHordes; // mapIndex -> PointID
        std::set<SpotID> baseSpots;
        std::for_each(metaPair.second.spotMetaDatas.begin(), metaPair.second.spotMetaDatas.end(),
            [&](const auto& spotPair)
            {
                const SpotMetaData& spotMetaData = spotPair.second;
                auto iter = mapVariationMap.find(spotMetaData.variationID);
                if(iter == mapVariationMap.end())
                    return;
                
                int modifier1 = std::stoi(mapVariationCsv.getValue(iter->second, "modifier1"));
                int modifier2 = std::stoi(mapVariationCsv.getValue(iter->second, "modifier2"));
                int modifier3 = std::stoi(mapVariationCsv.getValue(iter->second, "modifier3"));
                if(modifier1 == 180 || modifier1 == 604)
                {
                    // night horde
                    nightHordes[spotMetaData.mapIndex] = spotPair.first;
                    return;
                }
                else if(modifier1 == 13)
                {
                    // rotted
                }
                baseSpots.insert(spotPair.first);
            }
        );

        for(auto spotID : baseSpots)
        {
            const SpotMetaData& spotMetaData = metaPair.second.spotMetaDatas.at(spotID);
            MetaData::SpotData spotData;

            auto attachIt = spotPointMap.find(spotMetaData.pointID);
            if (attachIt != spotPointMap.end())
            {
                spotData.location.gridX = std::stoi(spotPointCsv.getValue(attachIt->second, "gridXNo")) - GRID_OFFSET_X;
                spotData.location.gridZ = std::stoi(spotPointCsv.getValue(attachIt->second, "gridZNo")) - GRID_OFFSET_Z;
                spotData.location.posX = std::stof(spotPointCsv.getValue(attachIt->second, "posX"));
                spotData.location.posZ = std::stof(spotPointCsv.getValue(attachIt->second, "posZ"));
            }
            auto attach2It = spotPoint2Map.find(spotMetaData.pointID);
            if (attach2It != spotPoint2Map.end())
            {
                spotData.locationExtra.gridX = std::stoi(spotPoint2Csv.getValue(attach2It->second, "gridXNo")) - GRID_OFFSET_X;
                spotData.locationExtra.gridZ = std::stoi(spotPoint2Csv.getValue(attach2It->second, "gridZNo")) - GRID_OFFSET_Z;
                spotData.locationExtra.posX = std::stof(spotPoint2Csv.getValue(attach2It->second, "posX"));
                spotData.locationExtra.posZ = std::stof(spotPoint2Csv.getValue(attach2It->second, "posZ"));
            }

            spotData.attachment.variantId = spotMetaData.variationID;
            spotData.attachment.variantIndex = spotMetaData.variantIndex;
            spotData.attachment.label = spotLabelMap[spotData.attachment.UID()];
            patternIt->second.baseSpots[spotID] = spotData;
        }

        for(auto nightHorde : nightHordes)
        {
            const SpotMetaData& spotMetaData = metaPair.second.spotMetaDatas.at(nightHorde.second);
            MetaData::SpotData spotData;

            auto attachIt = spotPointMap.find(spotMetaData.pointID);
            if (attachIt != spotPointMap.end())
            {
                spotData.location.gridX = std::stoi(spotPointCsv.getValue(attachIt->second, "gridXNo")) - GRID_OFFSET_X;
                spotData.location.gridZ = std::stoi(spotPointCsv.getValue(attachIt->second, "gridZNo")) - GRID_OFFSET_Z;
                spotData.location.posX = std::stof(spotPointCsv.getValue(attachIt->second, "posX"));
                spotData.location.posZ = std::stof(spotPointCsv.getValue(attachIt->second, "posZ"));
            }
            auto attach2It = spotPoint2Map.find(spotMetaData.pointID);
            if (attach2It != spotPoint2Map.end())
            {
                spotData.locationExtra.gridX = std::stoi(spotPoint2Csv.getValue(attach2It->second, "gridXNo")) - GRID_OFFSET_X;
                spotData.locationExtra.gridZ = std::stoi(spotPoint2Csv.getValue(attach2It->second, "gridZNo")) - GRID_OFFSET_Z;
                spotData.locationExtra.posX = std::stof(spotPoint2Csv.getValue(attach2It->second, "posX"));
                spotData.locationExtra.posZ = std::stof(spotPoint2Csv.getValue(attach2It->second, "posZ"));
            }

            spotData.attachment.variantId = spotMetaData.variationID;
            spotData.attachment.variantIndex = spotMetaData.variantIndex;
            spotData.attachment.label = spotLabelMap[spotData.attachment.UID()];
            patternIt->second.eventCandidates[nightHorde.first] = spotData;
        }
    }
}

int MetaData::queryByAttachmentID(VariationID variationID) const
{
    for (const auto& patternPair : patterns)
    {
        const PatternData& patternData = patternPair.second;
        for (const auto& spotPair : patternData.baseSpots)
        {
            const SpotData& spot = spotPair.second;
            if (spot.attachment.UID() == variationID)
            {
                return patternPair.first;
            }
        }
    }

    return -1;
}

const MetaData::SpotData* MetaData::queryBySpotID(SpotID spotID) const
{
    for (const auto& patternPair : patterns)
    {
        const PatternData& patternData = patternPair.second;
        auto spotIt = patternData.baseSpots.find(spotID);
        if (spotIt != patternData.baseSpots.end())
        {
            return &spotIt->second;
        }
    }
    return nullptr;
}

#include "MetaData.h"
#include "CsvReader.h"
#include <SDL_log.h>
#include <SDL_assert.h>

void MetaData::load()
{
    CsvReader lrSpotCsv;
    if (!lrSpotCsv.load("nightreign/assets/datas/LotResultSmallBaseAndSpot.csv", true))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "MetaData: Failed to load LotResultSmallBaseAndSpot.csv");
        return;
    }
    CsvReader lrPatternCsv;
    if (!lrPatternCsv.load("nightreign/assets/datas/LotResultMapPatternFlag.csv", true))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "MetaData: Failed to load LotResultMapPatternFlag.csv");
        return;
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

    for(auto& row : lrSpotCsv.getAllRows())
    {
        PatternID patternId = std::stoi(row[lrSpotCsv.getColumnIndex("patternId")]);
        auto patternIt = patterns.find(patternId);
        SDL_assert(patternIt != patterns.end());

        SpotID spotId = std::stoi(row[lrSpotCsv.getColumnIndex("ID")]);
        int variationId = std::stoi(row[lrSpotCsv.getColumnIndex("smallBaseMapId")]);
        int variationIndex = std::stoi(row[lrSpotCsv.getColumnIndex("variationId")]);
        int pointID = std::stoi(row[lrSpotCsv.getColumnIndex("attachId")]);

        auto mapIt = mapVariationMap.find(variationId);
        if (mapIt != mapVariationMap.end()) {
            int NTFlag = std::stoi(mapVariationCsv.getValue(mapIt->second, "modifier1"));
            if( NTFlag > 0)
                continue;
        }
        
        CsvReader* pointCsvToUse = nullptr;
        int attachRow = -1;
        auto attachIt = spotPointMap.find(pointID);
        if (attachIt != spotPointMap.end())
        {
            pointCsvToUse = &spotPointCsv;
            attachRow = attachIt->second;
        }
        else
        {
            attachIt = spotPoint2Map.find(pointID);
            if (attachIt != spotPoint2Map.end())
            {
                pointCsvToUse = &spotPoint2Csv;
                attachRow = attachIt->second;
            }
        }
        if(!pointCsvToUse || attachRow == -1)
        {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "MetaData: Attach point ID %d not found for spot ID %d", pointID, spotId);
            continue;
        }

        SpotData spotData;
        spotData.gridX = std::stoi(pointCsvToUse->getValue(attachRow, "gridXNo")) - GRID_OFFSET_X;
        spotData.gridZ = std::stoi(pointCsvToUse->getValue(attachRow, "gridZNo")) - GRID_OFFSET_Z;
        spotData.posX = std::stof(pointCsvToUse->getValue(attachRow, "posX"));
        spotData.posZ = std::stof(pointCsvToUse->getValue(attachRow, "posZ"));
        spotData.attachment.variantId = variationId;
        spotData.attachment.variantIndex = variationIndex;
        spotData.attachment.label = spotLabelMap[spotData.attachment.UID()];
        patternIt->second.spots[spotId] = spotData;
    }
}
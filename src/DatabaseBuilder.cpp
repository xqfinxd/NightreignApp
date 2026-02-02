#include "CsvReader.h"
#include <cassert>

void collectSpots(const std::string& output) {
    std::ofstream outFile(output, std::ios::out);
    if (!outFile.is_open()) {
        return;
    }

    // Load CSV files
	CsvReader lotResultCsv;
	CsvReader attachPointCsv;
	CsvReader mapVariationCsv;
	
    assert(lotResultCsv.load("nightreign/assets/datas/LotResultSmallBaseAndSpot.csv", true));
	assert(attachPointCsv.load("nightreign/assets/datas/SmallBaseAndSpotAttachPoint.csv", true));
    assert(mapVariationCsv.load("nightreign/assets/datas/SmallBaseMapVariationParam.csv", true));
	
	// Build lookup maps for faster access
	std::map<int, size_t> attachPointMap; // ID -> row index
	for (size_t i = 0; i < attachPointCsv.getRowCount(); ++i)
	{
		int id = std::stoi(attachPointCsv.getValue(i, "ID"));
		attachPointMap[id] = i;
	}
	
	std::map<int, size_t> mapVariationMap; // ID -> row index
	for (size_t i = 0; i < mapVariationCsv.getRowCount(); ++i)
	{
		int id = std::stoi(mapVariationCsv.getValue(i, "ID"));
		mapVariationMap[id] = i;
	}
	
	// Find all entries matching the patternId
	int spotsLoaded = 0;
    outFile << "smallBaseMapId,variationId,patternId\n";
	for (size_t i = 0; i < lotResultCsv.getRowCount(); ++i)
	{
		int pattern = std::stoi(lotResultCsv.getValue(i, "patternId"));
		int attachId = std::stoi(lotResultCsv.getValue(i, "attachId"));
		int smallBaseMapId = std::stoi(lotResultCsv.getValue(i, "smallBaseMapId"));

		auto mapIt = mapVariationMap.find(smallBaseMapId);
		if (mapIt == mapVariationMap.end())
			continue;
		int disableNT = std::stoi(mapVariationCsv.getValue(mapIt->second, "disableParam_NT"));
		if( disableNT == 0)
			continue;
        
		// Look up attach point data
		auto attachIt = attachPointMap.find(attachId);
		if (attachIt == attachPointMap.end())
			continue;
		
		// Look up map variation name
		std::string spotLabel = std::to_string(smallBaseMapId);
		spotLabel.append("," + lotResultCsv.getValue(i, "variationId"));
        spotLabel.append("," + std::to_string(pattern));
		outFile << spotLabel << "\n";
	}
}

int main() {

    collectSpots(R"(D:\Temp\spots.csv)");

    return 0;
}
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <glm/glm.hpp>

struct MetaData
{
    static constexpr int GRID_OFFSET_X = 41;
    static constexpr int GRID_OFFSET_Z = 35;
    static constexpr float GRID_SIZE = 256.0f;

    using PatternID = int;
    using SpotID = int;
    using AttachPointID = int;
    using VariationID = int;
    using NightlordID = int;
    using MapID = int;

    struct SpotMetaData
    {
        AttachPointID pointID;
        VariationID variationID;
        int variantIndex;
        int mapIndex;
        int modifier;
    };

    struct PatternMetaData
    {
        std::unordered_map<SpotID, SpotMetaData> spotMetaDatas;
    };

    struct Location
    {
        int gridX = -1;
        int gridZ = -1;
        float posX = 0.0f;
        float posZ = 0.0f;

        bool isValid() const
        {
            return gridX > -1 && gridZ > -1;
        }

        glm::vec2 normalized() const
        {
            float normalizedX = posX / GRID_SIZE;
            float normalizedZ = posZ / GRID_SIZE;
            
            // Final grid position
            float finalGridX = gridX + normalizedX;
            float finalGridZ = gridZ + normalizedZ;
            return glm::vec2(finalGridX, finalGridZ);
        }
    };

    struct Attachment
    {
        int variantId = -1;
        int variantIndex = -1;
        std::string label;

        VariationID UID() const { return variantId * 10 + variantIndex; }
    };
    
    struct SpotData
    {
        Location location;
        Location locationExtra; // For additional positioning if needed
        Attachment attachment;
    };
    
    struct PatternData {
        NightlordID nightlord = -1;
        MapID map = -1;
        std::unordered_map<SpotID, SpotData> baseSpots;
        SpotData starter;
        std::unordered_map<SpotID, SpotData> eventCandidates;
    };

    std::unordered_map<PatternID, PatternData> patterns;

    void load();
    int queryByAttachmentID(VariationID attachmentID) const;
    const SpotData* queryBySpotID(SpotID spotID) const;
};

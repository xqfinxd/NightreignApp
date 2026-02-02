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
    struct Attachment
    {
        int variantId = -1;
        int variantIndex = -1;

        int UID() const { return variantId * 10 + variantIndex; }
    };
    
    using SpotID = int;
    struct SpotData
    {
        int gridX = -1;
        int gridZ = -1;
        float posX = 0.0f;
        float posZ = 0.0f;
        Attachment attachment;

        glm::vec2 getGridPos() const
        {
            float normalizedX = posX / GRID_SIZE;
            float normalizedZ = posZ / GRID_SIZE;
            
            // Final grid position
            float finalGridX = gridX + normalizedX;
            float finalGridZ = gridZ + normalizedZ;
            return glm::vec2(finalGridX, finalGridZ);
        }
    };
    
    using PatternID = int;
    using NightlordID = int;
    using MapID = int;
    struct PatternData {
        NightlordID nightlord = -1;
        MapID map = -1;
        std::unordered_map<SpotID, SpotData> spots;
        SpotData starter;
    };

    std::unordered_map<PatternID, PatternData> patterns;

    void load();
};

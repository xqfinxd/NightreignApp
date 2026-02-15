#pragma once
#include "public.h"
#include "GameData.h"
#include "components/MapSpot.h"
#include <entt/entt.hpp>
#include <vector>
#include <map>
#include <set>

class Renderer;
class Device;
class Camera;

class Scene
{
public:
	Scene();
	~Scene();

	void initialize();
	void cleanup();

	void update(float deltaTime);
	void render(Renderer* renderer);
	void drawUI();

	// Map control
	void loadMapTiles(int mapIndex, int layer = 0);

	// Map spots
	entt::entity addSpot(const glm::vec2 &gridPos);
	entt::entity addBaseSpot(const glm::vec2 &gridPos, int attachId, const VariationInfo& info);
	entt::entity addFilterSpot(const glm::vec2 &gridPos, int spotId, const VariationInfo& info);
	entt::entity addStarterSpot(const glm::vec2 &gridPos, int starterId, const VariationInfo& info);
	void updateSpot(entt::entity entity, const VariationInfo& info);
	void clearSpots();
	void loadSpotsByPattern(int patternId);
	void loadSpotsByMap(int map);

	// Mouse input
	using SpotClickCallback = std::function<void(entt::entity spotEntity, const MapSpot& spot)>;
	void onMouseClick(int screenX, int screenY, int windowWidth, int windowHeight);
	void onMouseRightClick(int screenX, int screenY, int windowWidth, int windowHeight);
	void onMouseMove(int screenX, int screenY, int windowWidth, int windowHeight);
	void onMouseWheel(float deltaY);
	void onMouseButton(int button, bool pressed);
	void setSpotClickCallback(SpotClickCallback callback) { m_spotClickCallback = callback; }
	void handleSpotClick(entt::entity spotEntity, const MapSpot& spot);

	// ECS access
	entt::registry &getRegistry() { return m_registry; }
	const entt::registry &getRegistry() const { return m_registry; }

	Camera *getCamera();
	const Camera *getCamera() const;

	// Get clear color from camera
	glm::vec4 getClearColor() const;

private:
	void clearMapTiles();

	entt::registry m_registry;
	std::vector<entt::entity> m_mapTileEntities;
	std::vector<entt::entity> m_mapSpotEntities;

	int m_currentMapIndex = 0;
	int m_currentLayer = 0;
	float m_tileSize = 1.0f;
	int m_gridWidth = 6;
	int m_gridHeight = 6;
	int m_textureTileSize = 256;
	SpotClickCallback m_spotClickCallback = nullptr;
	
	// Mouse interaction state
	bool m_isMouseDragging = false;
	glm::vec2 m_lastMousePos = glm::vec2(0.0f);
	glm::vec3 m_mouseWorldPos = glm::vec3(0.0f);
	float m_minZoom = 0.5f;
	float m_maxZoom = 20.0f;
	
	int m_currentPatternId = -1;
	
	int m_patternInput = 0;
	int m_attachmentInput = -1;
	
	bool m_enableB1Overlay = false;
	
	entt::entity m_selectedSpotEntity = entt::null;
	
	// Pattern filter mode
	struct VariationOption {
		VariationInfo info;
		std::set<int> patterns;
	};
	
	struct ContextMenuData {
		glm::vec2 worldPosition = glm::vec2(0.0f);
		std::vector<entt::entity> entities;
		std::vector<int> starterIds;
		std::vector<int> filterSpotIds;
		std::vector<VariationOption> variations;
		
		void clear() {
			entities.clear();
			starterIds.clear();
			filterSpotIds.clear();
			variations.clear();
		}
		
		bool hasStarters() const { return !starterIds.empty(); }
		bool hasFilters() const { return !filterSpotIds.empty(); }
		bool isEmpty() const { return starterIds.empty() && filterSpotIds.empty(); }
	};
	
	VariationInfo getFilterSpot() const;
	VariationInfo getFilterStarter() const;
	
	bool m_filterMode = false;
	int m_filterMapSelection = -1;
	int m_currentSpotId = -1;  // Currently selected spot ID
	std::vector<VariationOption> m_availableVariations;
	int m_selectedVariationIndex = -1;
	std::map<int, int> m_markedSpots; // spotId -> variationKey
	std::set<int> m_filteredPatterns; // Result of pattern filtering
	
	// Context menu state
	bool m_showContextMenu = false;
	glm::vec2 m_contextMenuPos = glm::vec2(0.0f);
	ContextMenuData m_contextMenuData;
};

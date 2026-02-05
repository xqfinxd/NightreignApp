#pragma once
#include "public.h"
#include "components/MapSpot.h"
#include "MetaData.h"
#include <entt/entt.hpp>
#include <vector>

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
	entt::entity addSpot(const glm::vec2 &gridPos, const std::string &textureName, float size = 0.2f);
	void clearSpots();
	void loadSpotsByPattern(int patternId);

	// Mouse input
	using SpotClickCallback = std::function<void(entt::entity spotEntity, const MapSpot& spot)>;
	void onMouseClick(int screenX, int screenY, int windowWidth, int windowHeight);
	void onMouseMove(int screenX, int screenY, int windowWidth, int windowHeight);
	void onMouseWheel(float deltaY);
	void onMouseButton(int button, bool pressed);
	void setSpotClickCallback(SpotClickCallback callback) { m_spotClickCallback = callback; }

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
	SpotClickCallback m_spotClickCallback = nullptr;
	
	// Mouse interaction state
	bool m_isMouseDragging = false;
	glm::vec2 m_lastMousePos = glm::vec2(0.0f);
	float m_minZoom = 0.5f;
	float m_maxZoom = 20.0f;
	
	MetaData m_metaData;
	bool m_metaDataLoaded = false;
	int m_currentPatternId = -1;
	
	int m_patternInput = 0;
	int m_attachmentInput = -1;
	
	bool m_enableB1Overlay = false;
	
	entt::entity m_selectedSpotEntity = entt::null;
};

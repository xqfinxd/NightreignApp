#pragma once
#include "public.h"
#include "components/MapSpot.h"
#include <entt/entt.hpp>
#include <vector>

class RenderSystem;
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
	void render();
	void drawUI();

	// Map control
	void loadMapTiles(int mapIndex, int layer = 0);
	void setMapIndex(int index);
	int getMapIndex() const { return m_currentMapIndex; }

	// Map spots
	void addSpot(const glm::vec2 &gridPos, const std::string &textureName, float size = 0.2f);
	void clearSpots();
	void loadSpotsByPattern(int patternId);

	// Mouse input
	typedef void (*SpotClickCallback)(entt::entity spotEntity, const MapSpot& spot);
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
	RenderSystem *m_render_system = nullptr;
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
	
	// Spot pattern input
	int m_patternInput = 1;
};

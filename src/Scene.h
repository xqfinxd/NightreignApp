#pragma once
#include "public.h"
#include "GameData.h"
#include "components/Map.h"
#include "components/Transform.h"
#include "InputHandler.h"
#include <entt/entt.hpp>
#include <vector>
#include <map>
#include <set>

class Renderer;
class Device;
class Camera;

class Entity {
public:
	Entity(std::shared_ptr<entt::registry> registry, const std::string& name);
	~Entity();
	void destroy() { m_deleted = true; }
	void reuse() { m_deleted = false; }

	entt::entity getEntity() const { return m_entity; }
	const std::string& getName() const { return m_name; }
	const std::string& getTag() const { return m_tag; }
	bool hasTag(const std::string& subtag) const { return m_tag.find(subtag) != m_tag.npos; }
	void setTag(const std::string& tag) { m_tag.assign(tag); }
	Transform& getTransform() { return m_transform; }
	const Transform& getTransform() const { const_cast<Entity*>(this)->getTransform(); }
	bool valid() const { return m_registry && m_entity != entt::null && m_registry->valid(m_entity); }
	bool deleted() const { return m_deleted; }
	bool getEnabled() const { return m_enabled; }
	void setEnabled(bool enabled) { m_enabled = enabled; }

	// component operation
	template<class TComp>
	TComp* getComponent() {
		SDL_assert(valid());
		return m_registry->try_get<TComp>(m_entity);
	}
	template<class TComp>
	const TComp* getComponent() const {
		return const_cast<Entity*>(this)->getComponent<TComp>();
	}
	template<class TComp, typename... Args>
	TComp& addComponent(Args &&...args) {
		SDL_assert(valid());
		return m_registry->get_or_emplace<TComp>(m_entity, std::forward<Args>(args)...);
	}

	// template specify
	template<>
	const Transform* getComponent<Transform>() const {
		return &m_transform;
	}
	template<>
	Transform* getComponent<Transform>() {
		return &m_transform;
	}
	template<>
	Transform& addComponent<Transform>() {
		return m_transform;
	}

private:
	std::shared_ptr<entt::registry> m_registry;
	entt::entity m_entity = entt::null;
	std::string m_name;
	std::string m_tag;
	Transform m_transform;
	bool m_enabled = true;
	bool m_deleted = false;
};

class EntityManager {
public:
	EntityManager();
	virtual ~EntityManager();

	const entt::registry& getRegistry() const { return *m_registry; }

	Entity& addObject(const std::string& name);
	Entity& retrieveObject(const std::string& name);
	bool removeObject(Entity*& object);
	// cleanup
	void clearObjects();
	// update
	void updateObjects();

	Entity* findObject(const std::string& name);
	Entity* findObject(entt::entity entity);

	template<class TComp>
	TComp* findComponent() {
		auto view = m_registry->view<TComp>();
		for (auto entity : view)
		{
			return m_registry->try_get<TComp>(entity);
		}
		return nullptr;
	}
	template<class TComp>
	std::vector<Entity*> findObjectsByComponent() {
		auto view = m_registry->view<TComp>();
		std::vector<Entity*> result;
		for (auto entity : view)
		{
			auto o = findObject(entity);
			if (o) result.push_back(o);
		}
		return result;
	}

private:
	std::shared_ptr<entt::registry> m_registry;
	std::vector<Entity*> m_entities;
	std::unordered_map<std::string_view, Entity*> m_name2Entities;
	std::unordered_map<entt::entity, Entity*> m_entt2Entities;
};

class Scene : public EntityManager
{
public:
	Scene();
	~Scene();

	void initialize();
	void cleanup();

	void update(float deltaTime);
	void render(Renderer* renderer);
	void drawUI();
	void drawContextMenu();

	void setScreenText(const std::string& text) { m_screenText = text; }
	const std::string& getScreenText() const { return m_screenText; }

	// Map control
	void loadMapTiles(int mapIndex, int layer = 0);
	void onFilterPatterns(int patternId);
	void resetMapFilters();
	void filterMap(int map);
	void filterNightlord(int nightlord);
	
	// B1 Overlay control
	void setEnableB1Overlay(bool enable) { m_enableB1Overlay = enable; updateB1Overlay(); }
	bool getEnableB1Overlay() const { return m_enableB1Overlay; }
	void updateB1Overlay();

	// Map spots
	Entity& addSpot(const std::string& name, const MapPoint &gridPos);
	Entity& addBaseSpot(const MapPoint &gridPos, int attachId, const VariationInfo& info);
	Entity& addFilterSpot(const MapPoint &gridPos, int spotId, const VariationInfo& info);
	Entity& addStarterSpot(const MapPoint &gridPos, int starterId, const VariationInfo& info);
	void updateSpotIcon(Entity& gameObject, const std::string& icon, float scale);
	void updateSpotText(Entity& gameObject, const std::string& text);
	void clearSpots();
	void loadSpotsByPattern(int patternId);
	void loadSpotsByMap(int map);

	// Mouse input
	using ClickCallback = std::function<void(Entity&)>;
	std::vector<Entity*> Raycast(Camera& camera, int screenX, int screenY, int width, int height);
	void onSelectSpots(int screenX, int screenY, int width, int height);

	void handleInput(const InputState& result, int width, int height);

	Camera *getCamera();
	const Camera *getCamera() const;

	// Get clear color from camera
	glm::vec4 getClearColor() const;

private:
	void clearMapTiles();
    bool isOverlayPoint(const MapPoint& point) const;
	void updatePanelInfo(const PatternInfo& point);

private:
	PROPERTY float m_minZoom = 0.5f;
	PROPERTY float m_maxZoom = 20.0f;
	PROPERTY float m_tileSize = 1.0f;
	PROPERTY int m_gridWidth = 6;
	PROPERTY int m_gridHeight = 6;
	PROPERTY int m_textureTileSize = 256;

	int m_currentMapIndex = 0;
	int m_currentLayer = 0;
	int m_currentPatternId = -1;
	bool m_enableB1Overlay = false;
	std::vector<glm::ivec2> m_overlayGrids;

	ClickCallback m_tapCallback = nullptr;
	
	struct VariationOption {
		VariationInfo info;
		std::set<int> patterns;
	};
	
	struct SelectionData {
		glm::vec2 worldPosition = glm::vec2(0.0f);
		std::vector<entt::entity> entities;
		std::vector<int> starterIds;
		std::vector<int> attachIds;
		std::vector<VariationOption> variations;
		
		void clear() {
			entities.clear();
			starterIds.clear();
			attachIds.clear();
			variations.clear();
		}
		
		bool hasStarters() const { return !starterIds.empty(); }
		bool hasFilters() const { return !attachIds.empty(); }
		bool isEmpty() const { return starterIds.empty() && attachIds.empty(); }
	};
	
	VariationInfo getFilterSpot() const;
	VariationInfo getFilterStarter() const;
	VariationInfo getPlayArea(int day) const;
    VariationInfo getConstant(const ConstantInfo& info) const;
    VariationInfo getRottedPower() const;
	VariationInfo getGreatHollowSmallBoss(const GreatHollowBindingInfo& info) const;
	
	bool m_filterMode = false;
	std::set<int> m_filteredPatterns; // Result of pattern filtering
	
	// Context menu state
	bool m_showContextMenu = false;
	glm::vec2 m_contextMenuPos = glm::vec2(0.0f);
	SelectionData m_selectionData;
	std::unique_ptr<GameData> m_gameData;
	std::string m_screenText;
};

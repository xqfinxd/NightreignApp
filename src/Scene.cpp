#include "Scene.h"
#include "Renderer.h"
#include "components/Mesh.h"
#include "components/Transform.h"
#include "components/MapTileGrid.h"
#include "components/MapSpot.h"
#include "components/BlendMode.h"
#include "components/Tag.h"
#include "Device.h"
#include "ResourceManager.h"
#include "components/Camera.h"
#include "CsvReader.h"
#include <imgui.h>
#include <SDL_log.h>
#include <entt/entt.hpp>
#include <sstream>
#include <iomanip>
#include <map>
#include <set>
#include <SDL_events.h>

float MapSpot::textScale = 0.005f;

struct HitResult {
	bool hit = false;
	float distance = 0.0f;
	glm::vec3 point;
	glm::vec3 normal;
};

struct Ray {
	glm::vec3 origin;
	glm::vec3 direction;

	HitResult intersectPlane(const glm::vec3& planePoint, const glm::vec3& planeNormal) const {
		HitResult result;

		float denom = glm::dot(planeNormal, direction);
		if (fabs(denom) > FLT_EPSILON) {
			glm::vec3 p0l0 = planePoint - origin;
			float t = glm::dot(p0l0, planeNormal) / denom;

			if (t >= 0) {
				result.hit = true;
				result.distance = t;
				result.point = origin + direction * t;
				result.normal = planeNormal;
			}
		}

		return result;
	}

	HitResult intersectSphere(const glm::vec3& center, float radius) const {
		HitResult result;

		glm::vec3 oc = origin - center;
		float a = glm::dot(direction, direction);
		float b = 2.0f * glm::dot(oc, direction);
		float c = glm::dot(oc, oc) - radius * radius;
		float discriminant = b * b - 4 * a * c;

		if (discriminant > 0) {
			float t = (-b - sqrt(discriminant)) / (2.0f * a);
			if (t >= 0) {
				result.hit = true;
				result.distance = t;
				result.point = origin + direction * t;
				result.normal = glm::normalize(result.point - center);
			}
		}

		return result;
	}

	HitResult intersectAABB(const glm::vec3& minBounds, const glm::vec3& maxBounds) const {
		HitResult result;

		float tmin = 0.0f;
		float tmax = 100000.0f;

		for (int i = 0; i < 3; i++) {
			if (fabs(direction[i]) < 1e-6) {
				if (origin[i] < minBounds[i] || origin[i] > maxBounds[i])
					return result;
			}
			else {
				float ood = 1.0f / direction[i];
				float t1 = (minBounds[i] - origin[i]) * ood;
				float t2 = (maxBounds[i] - origin[i]) * ood;

				if (t1 > t2) std::swap(t1, t2);
				if (t1 > tmin) tmin = t1;
				if (t2 < tmax) tmax = t2;

				if (tmin > tmax) return result;
			}
		}

		if (tmin > 0) {
			result.hit = true;
			result.distance = tmin;
			result.point = origin + direction * tmin;

			glm::vec3 center = (minBounds + maxBounds) * 0.5f;
			glm::vec3 halfSize = (maxBounds - minBounds) * 0.5f;
			glm::vec3 localHit = result.point - center;

			float minDist = FLT_MAX;
			for (int i = 0; i < 3; i++) {
				float dist = fabs(halfSize[i] - fabs(localHit[i]));
				if (dist < minDist) {
					minDist = dist;
					result.normal = glm::vec3(0);
					result.normal[i] = (localHit[i] > 0) ? 1.0f : -1.0f;
				}
			}
		}

		return result;
	}
};

Scene::Scene()
{
	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Scene: ECS registry created");
}

Scene::~Scene()
{
	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Scene: ECS registry destroyed (entities: %zu)", m_registry.size());
}

void Scene::initialize()
{
	// Create default camera
	{
		auto entity = m_registry.create();
		auto &camera = m_registry.emplace<Camera>(entity);

		camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
		camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
		camera.updateMatrices();
	}

	// Set up spot click callback
	setSpotClickCallback([this](entt::entity entity, const MapSpot& spot) {
		if(m_selectedSpotEntity != entt::null)
		{
			auto& prevSpot = m_registry.get<Transform>(m_selectedSpotEntity);
			prevSpot.scale = glm::vec3(0.1f, 0.1f, 1.0f);
		}

		if(m_selectedSpotEntity == entity)
		{
			m_selectedSpotEntity = entt::null;
		}
		else
		{
			m_selectedSpotEntity = entity;
		}

		if(m_selectedSpotEntity != entt::null)
		{
			auto& newSpot = m_registry.get<Transform>(m_selectedSpotEntity);
			newSpot.scale = glm::vec3(0.3f, 0.3f, 1.0f);
		}
	});

	// Add some sample spots on the map with Chinese labels
	loadSpotsByPattern(0);

	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Scene initialized (ECS ready, entities: %zu)", m_registry.size());
}

void Scene::cleanup()
{
	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Scene cleanup (clearing %zu entities)", m_registry.size());
	m_selectedSpotEntity = entt::null;
	m_registry.clear();
}

void Scene::update(float deltaTime)
{
}

void Scene::onMouseClick(int screenX, int screenY, int windowWidth, int windowHeight)
{
	auto camera = getCamera();
	if (!camera)
		return;

	// Convert screen coordinates to NDC (Normalized Device Coordinates)
	// Screen Y is top-down, NDC Y is bottom-up
	float ndcX = (2.0f * screenX) / windowWidth - 1.0f;
	float ndcY = 1.0f - (2.0f * screenY) / windowHeight;

	// Convert NDC to world space
	glm::vec4 nearPointWorld = camera->clip2World(glm::vec4(ndcX, ndcY, -1.0f, 1.0f));
	glm::vec4 farPointWorld = camera->clip2World(glm::vec4(ndcX, ndcY, 1.0f, 1.0f));
	nearPointWorld /= nearPointWorld.w;
	farPointWorld /= farPointWorld.w;
	Ray ray;
	ray.origin = glm::vec3(nearPointWorld);
	ray.direction = glm::normalize(glm::vec3(farPointWorld) - ray.origin);
	
	// Collect all spots at this click location
	std::vector<entt::entity> hitsAtLocation;
	int selectedIndex = -1;
	for (auto it = m_mapSpotEntities.rbegin(); it != m_mapSpotEntities.rend(); ++it)
	{
		auto entity = *it;
		if (!m_registry.valid(entity))
			continue;

		auto* transform = m_registry.try_get<Transform>(entity);
		auto* mapSpot = m_registry.try_get<MapSpot>(entity);
		if (!transform || !mapSpot)
			continue;

		// Check if click is within spot bounds
		float halfSize = (mapSpot->size * m_tileSize) * 0.5f;
		auto result = ray.intersectSphere(transform->position, halfSize);

		if (result.hit)
		{
			hitsAtLocation.push_back(entity);
			if (entity == m_selectedSpotEntity)
			{
				selectedIndex = static_cast<int>(hitsAtLocation.size()) - 1;
			}
		}
	}
	
	if (m_spotClickCallback && !hitsAtLocation.empty())
	{
		entt::entity nextSelectedEntity;
		if(selectedIndex < 0 || static_cast<size_t>(selectedIndex) >= hitsAtLocation.size() - 1)
		{
			nextSelectedEntity = hitsAtLocation[0];
		}
		else
		{
			nextSelectedEntity = hitsAtLocation[selectedIndex + 1];
		}

		auto* mapSpot = m_registry.try_get<MapSpot>(nextSelectedEntity);
		if (mapSpot)
		{
			m_spotClickCallback(nextSelectedEntity, *mapSpot);
		}
	}
}

void Scene::onMouseButton(int button, bool pressed)
{
	// Right mouse button for dragging
	if (button == SDL_BUTTON_LEFT)
	{
		m_isMouseDragging = pressed;
		
		if (!pressed)
		{
			// Reset drag state when button released
			m_lastMousePos = glm::vec2(0.0f);
		}
	}
}

void Scene::onMouseMove(int screenX, int screenY, int windowWidth, int windowHeight)
{
	glm::vec2 currentMousePos = glm::vec2(screenX, screenY);
	
	if (m_isMouseDragging && m_lastMousePos != glm::vec2(0.0f))
	{
		auto camera = getCamera();
		if (!camera)
			return;
		
		// Calculate mouse delta in screen space
		glm::vec2 delta = currentMousePos - m_lastMousePos;
		
		// Convert screen delta to world delta based on camera
		float worldDeltaScale = 0.01f;
		if (camera->isOrthographic)
		{
			// For orthographic camera, scale based on ortho size
			worldDeltaScale = camera->orthoSize / (float)windowHeight;
		}
		else
		{
			// For perspective camera, scale based on distance from target
			float distance = glm::length(camera->position - camera->target);
			worldDeltaScale = distance * 0.001f;
		}
		
		// Calculate world space movement (inverted for natural drag feel)
		glm::vec3 right = glm::normalize(glm::cross(camera->target - camera->position, camera->up));
		glm::vec3 worldUp = camera->up;
		
		glm::vec3 movement = -right * delta.x * worldDeltaScale + worldUp * delta.y * worldDeltaScale;
		
		// Update camera position and target
		camera->position += movement;
		camera->target += movement;
		camera->updateMatrices();
	}
	
	m_lastMousePos = currentMousePos;
}

void Scene::onMouseWheel(float deltaY)
{
	auto camera = getCamera();
	if (!camera)
		return;
	
	// Zoom by adjusting ortho size or camera distance
	if (camera->isOrthographic)
	{
		// Zoom by changing orthographic size
		float zoomFactor = 1.0f - deltaY * 0.1f;
		camera->orthoSize *= zoomFactor;
		
		// Clamp zoom level
		camera->orthoSize = glm::clamp(camera->orthoSize, m_minZoom, m_maxZoom);
	}
	else
	{
		// Zoom by moving camera closer/farther from target
		glm::vec3 direction = glm::normalize(camera->target - camera->position);
		float distance = glm::length(camera->position - camera->target);
		float zoomAmount = distance * deltaY * 0.1f;
		
		camera->position += direction * zoomAmount;
		
		// Clamp distance
		float newDistance = glm::length(camera->position - camera->target);
		if (newDistance < m_minZoom)
		{
			camera->position = camera->target - direction * m_minZoom;
		}
		else if (newDistance > m_maxZoom)
		{
			camera->position = camera->target - direction * m_maxZoom;
		}
	}
	
	camera->updateMatrices();
}

void Scene::render(Renderer* renderer)
{
	// Get camera
	auto camera = getCamera();
	if (!camera)
	{
		return;
	}

	// Render all entities with MeshComponent
	renderer->renderEntities(m_registry);
	
	// Render spot labels with ImGui's font texture
	glm::vec4 black = glm::vec4(0.0f, 0.0f, 0.0f, 0.5f);
	glm::vec4 white = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	renderer->renderSpotLabels(m_registry, *camera, black, white);
}

void Scene::drawUI()
{
	ImGui::Text("ECS Statistics:");
	ImGui::Text("  Entities: %zu", m_registry.size());
	ImGui::Text("  Alive: %zu", m_registry.alive());

	ImGui::Separator();
	ImGui::Text("Map Control:");
	ImGui::Text("  Current Index: %d", m_currentMapIndex);
	ImGui::Text("  Current Layer: %d", m_currentLayer);
	ImGui::Text("  Tiles: %zu", m_mapTileEntities.size());
	ImGui::Text("  Spots: %zu", m_mapSpotEntities.size());

	ImGui::Separator();
	if(m_selectedSpotEntity != entt::null)
	{
		auto& spot = m_registry.get<MapSpot>(m_selectedSpotEntity);
		ImGui::Text("Selected Spot:");
		ImGui::Text("  ID: %s", spot.metadata.has_value() ? std::any_cast<std::string>(spot.metadata).c_str() : "UNKNOWN");
		ImGui::Text("  Grid Position: (%.1f, %.1f)", spot.gridPosition.x, spot.gridPosition.y);
	}
	else
	{
		ImGui::Text("No spot selected");
	}

	ImGui::Separator();
	ImGui::Text("Pattern ID:");
	ImGui::InputInt("##PatternID", &m_patternInput);
	if (ImGui::Button("Load Spots by Pattern"))
	{
		loadSpotsByPattern(m_patternInput);
	}

	ImGui::Separator();
	ImGui::Text("Attachment ID:");
	ImGui::InputInt("##AttachmentID", &m_attachmentInput);
	if (ImGui::Button("Search Attachment and Load Pattern"))
	{
		int patternId = m_metaData.queryByAttachmentID(m_attachmentInput);
		if(patternId >= 0)
		{
			loadSpotsByPattern(patternId);
			m_patternInput = patternId;
		}
		else
		{
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Scene: Attachment ID %d not found in metadata", m_attachmentInput);
		}
	}

	ImGui::Separator();
	ImGui::Text("Camera:");
	if (auto camera = getCamera())
	{
		bool dirty = false;
		dirty |= ImGui::DragFloat3("Position", glm::value_ptr(camera->position));
		dirty |= ImGui::Checkbox("Orthographic", &camera->isOrthographic);

		if (camera->isOrthographic)
		{
			dirty |= ImGui::DragFloat("Ortho Size", &camera->orthoSize);
		}
		else
		{	
			dirty |= ImGui::DragFloat("FOV", &camera->fov);
		}

		if (dirty)
			camera->updateMatrices();
	}
	else
	{
		ImGui::Text("  No camera");
	}
}

Camera *Scene::getCamera()
{
	auto view = m_registry.view<Camera>();
	for (auto entity : view)
	{
		return &m_registry.get<Camera>(entity); // Return first camera found
	}
	return nullptr;
}

const Camera *Scene::getCamera() const
{
	return const_cast<Scene *>(this)->getCamera();
}

glm::vec4 Scene::getClearColor() const
{
	if (auto camera = getCamera())
	{
		return camera->clearColor;
	}
	return glm::vec4(0.1f, 0.1f, 0.15f, 1.0f); // Default color
}

void Scene::loadMapTiles(int mapIndex, int layer)
{
	// Clear existing tiles first
	clearMapTiles();

	m_currentMapIndex = mapIndex;
	m_currentLayer = layer;

	ResourceManager *resMgr = ResourceManager::getInstance();
	if (!resMgr)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Scene: ResourceManager not available");
		return;
	}

	m_gridWidth = 6;
	m_gridHeight = 6;
	m_tileSize = 1.0f;

	// Calculate centering offset
	float gridTotalWidth = m_gridWidth * m_tileSize;
	float gridTotalHeight = m_gridHeight * m_tileSize;
	float offsetX = -gridTotalWidth / 2.0f + m_tileSize / 2.0f;
	float offsetY = -gridTotalHeight / 2.0f + m_tileSize / 2.0f;

	// Load and create entities for each tile
	for (int y = 0; y < m_gridHeight; y++)
	{
		for (int x = 0; x < m_gridWidth; x++)
		{
			// Generate texture path and name
			std::stringstream ss;
			ss << "nightreign/assets/textures/" << mapIndex << "/MENU_MapTile_L" << layer
			   << "_" << std::setw(2) << std::setfill('0') << x
			   << "_" << std::setw(2) << std::setfill('0') << y << ".png";
			std::string texturePath = ss.str();

			std::stringstream nameStream;
			nameStream << "tile_" << mapIndex << "_L" << layer << "_" << x << "_" << y;
			std::string textureName = nameStream.str();

			// Load texture
			resMgr->loadTexture(textureName, texturePath);

			// Create tile entity
			auto entity = m_registry.create();
			m_registry.emplace<Tag>(entity, "map");
			m_registry.emplace<MeshComponent>(entity, "quad", "texture", textureName);

			auto &transform = m_registry.emplace<Transform>(entity);
			transform.position = glm::vec3(offsetX + x * m_tileSize, offsetY + y * m_tileSize, 0.0f);
			transform.scale = glm::vec3(m_tileSize, m_tileSize, 1.0f);

			// Store entity for later cleanup
			m_mapTileEntities.push_back(entity);
		}
	}

	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Scene: Loaded map tiles (index: %d, layer: %d, tiles: %dx%d)",
			mapIndex, layer, m_gridWidth, m_gridHeight);
}

void Scene::clearMapTiles()
{
	// Destroy all tile entities
	for (auto entity : m_mapTileEntities)
	{
		if (m_registry.valid(entity))
		{
			m_registry.destroy(entity);
		}
	}
	m_mapTileEntities.clear();
}

entt::entity Scene::addSpot(const glm::vec2 &gridPos, const std::string &textureName, float size)
{
	// Calculate centering offset (same as tile grid)
	float gridTotalWidth = m_gridWidth * m_tileSize;
	float gridTotalHeight = m_gridHeight * m_tileSize;
	float offsetX = -gridTotalWidth / 2.0f + m_tileSize / 2.0f;
	float offsetY = -gridTotalHeight / 2.0f + m_tileSize / 2.0f;

	// Convert grid position to world position
	float worldX = offsetX + gridPos.x * m_tileSize;
	float worldY = offsetY + gridPos.y * m_tileSize;

	// Create spot entity
	auto entity = m_registry.create();
	m_registry.emplace<Tag>(entity, "spot");
	m_registry.emplace<MapSpot>(entity, gridPos, textureName, size);
	m_registry.emplace<MeshComponent>(entity, "quad", "texture", textureName);
	m_registry.emplace<BlendMode>(entity, BlendMode::Type::DestAlpha);

	auto &transform = m_registry.emplace<Transform>(entity);
	transform.position = glm::vec3(worldX, worldY, 0.5f); // Slightly above tiles
	transform.scale = glm::vec3(size, size, 1.0f);

	// Store entity for later cleanup
	m_mapSpotEntities.push_back(entity);

	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Scene: Added spot at grid position (%.2f, %.2f)", gridPos.x, gridPos.y);
	return entity;
}

void Scene::clearSpots()
{
	// Destroy all spot entities
	for (auto entity : m_mapSpotEntities)
	{
		if (m_registry.valid(entity))
		{
			m_registry.destroy(entity);
		}
	}
	m_selectedSpotEntity = entt::null;
	m_mapSpotEntities.clear();
}

void Scene::loadSpotsByPattern(int patternId)
{
	// Clear existing spots
	clearSpots();
	
	if(!m_metaDataLoaded)
	{
		m_metaData.load();
		m_metaDataLoaded = true;
	}
	auto patternIt = m_metaData.patterns.find(patternId);
	if(patternIt == m_metaData.patterns.end())
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Scene: Pattern ID %d not found in metadata", patternId);
		return;
	}
    const MetaData::PatternData& patternData = patternIt->second;
    for (const auto& spotPair : patternData.spots)
    {
        const MetaData::SpotData& spot = spotPair.second;
        std::string textureName = "launch";
        auto spotEntity = addSpot(spot.getGridPos(), textureName, 0.1f);
		auto& mapSpot = m_registry.get<MapSpot>(spotEntity);
		mapSpot.label = spotPair.second.attachment.label.empty() ? "UNKNOWN" : spotPair.second.attachment.label;
		mapSpot.metadata = std::to_string(spotPair.first) + " : " + std::to_string(spotPair.second.attachment.UID());
    }
	loadMapTiles(patternData.map);
}
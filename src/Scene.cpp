#include "Scene.h"
#include "GameData.h"
#include "Renderer.h"
#include "components/Mesh.h"
#include "components/Transform.h"
#include "components/MapSpot.h"
#include "components/RenderOptions.h"
#include "components/Tag.h"
#include "Device.h"
#include "ResourceManager.h"
#include "Texture.h"
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

float MapSpot::textScale = 0.004f;
float MapSpot::iconSize = 0.2f;

struct HitResult {
	bool hit = false;
	float distance = 0.0f;
	glm::vec3 point{ 0.f };
	glm::vec3 normal{ 0.f };
};

struct Ray {
	glm::vec3 origin;
	glm::vec3 direction;

	Ray(const Camera& camera, int x, int y, int w, int h) {
		// Convert screen coordinates to NDC (Normalized Device Coordinates)
		// Screen Y is top-down, NDC Y is bottom-up
		float ndcX = (2.0f * x) / w - 1.0f;
		float ndcY = 1.0f - (2.0f * y) / h;

		// Convert NDC to world space
		glm::vec4 nearPointWorld = camera.clip2World(glm::vec4(ndcX, ndcY, -1.0f, 1.0f));
		glm::vec4 farPointWorld = camera.clip2World(glm::vec4(ndcX, ndcY, 1.0f, 1.0f));
		nearPointWorld /= nearPointWorld.w;
		farPointWorld /= farPointWorld.w;
		origin = glm::vec3(nearPointWorld);
		direction = glm::normalize(glm::vec3(farPointWorld) - origin);
	}

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
	// Load game data from CSV files
	if (!GameData::getInstance().loadFromCSV("nightreign/assets/datas"))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Application: Failed to load game data");
	}

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
		// Handle filter mode
		if (m_filterMode && spot.metadata > 0)
		{
			m_currentSpotId = spot.metadata;
			m_availableVariations.clear();
			m_selectedVariationIndex = -1;

			auto& gameData = GameData::getInstance();
			auto results = gameData.getVariationsAtSpot(spot.metadata, m_filteredPatterns);
			std::map<int, size_t> uniqueVars;
			for (const auto& res : results)
			{
				auto outPatterns = gameData.filterByVariation(
					m_filteredPatterns, spot.metadata, res->getKey());
				auto it = uniqueVars.find(res->getKey());
				if (it != uniqueVars.end() && it->second < m_availableVariations.size())
				{
					m_availableVariations[it->second].patterns.insert(
						outPatterns.begin(), outPatterns.end());
					continue;
				}
				VariationOption var;
				var.info = *res;
				var.patterns.insert(outPatterns.begin(), outPatterns.end());
				m_availableVariations.push_back(var);
				uniqueVars[res->getKey()] = m_availableVariations.size() - 1;
			}
		}
		
		// Normal mode - just visual selection
		if(m_selectedSpotEntity != entt::null)
		{
			auto& prevSpot = m_registry.get<MapSpot>(m_selectedSpotEntity);
			prevSpot.selected = false;
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
			auto& newSpot = m_registry.get<MapSpot>(m_selectedSpotEntity);
			newSpot.selected = true;
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

	Ray ray(*camera, screenX, screenY, windowWidth, windowHeight);
	
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
		float halfSize = (transform->scale.x * m_tileSize) * 0.5f;
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
		auto* tag = m_registry.try_get<Tag>(nextSelectedEntity);
		bool interactable = tag && (tag->name == "starter spot" || tag->name == "filter spot")
			&& mapSpot && mapSpot->metadata > 0;
		if (interactable)
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
	auto camera = getCamera();
	if (!camera)
		return;
	
	glm::vec2 currentMousePos = glm::vec2(screenX, screenY);
	Ray ray(*camera, screenX, screenY, windowWidth, windowHeight);
	auto result = ray.intersectPlane(glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
	if(result.hit)
	{
		m_mouseWorldPos = result.point;
	}
	
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
	// Scene statistics window
	ImGui::Begin("Scene View");
	ImGui::Text("ECS Statistics:");
	ImGui::Text("  Entities: %zu", m_registry.size());
	ImGui::Text("  Alive: %zu", m_registry.alive());

	ImGui::Separator();
	ImGui::Text("Map Info:");
	ImGui::Text("  Current Index: %d", m_currentMapIndex);
	ImGui::Text("  Current Layer: %d", m_currentLayer);
	ImGui::Text("  Tiles: %zu", m_mapTileEntities.size());
	ImGui::Text("  Spots: %zu", m_mapSpotEntities.size());
	ImGui::Text("  Pattern ID: %d", m_currentPatternId);

	ImGui::Separator();
	ImGui::Text("Camera:");
	if (auto camera = getCamera())
	{
		ImGui::Text("  Position: (%.2f, %.2f, %.2f)", camera->position.x, camera->position.y, camera->position.z);
		ImGui::Text("  Orthographic: %s", camera->isOrthographic ? "Yes" : "No");
		if (camera->isOrthographic)
		{
			ImGui::Text("  Ortho Size: %.2f", camera->orthoSize);
		}
		else
		{
			ImGui::Text("  FOV: %.2f", camera->fov);
		}
		glm::vec2 gridpos{m_mouseWorldPos.x / m_tileSize + (m_gridWidth / 2.0f),
	                      m_mouseWorldPos.y / m_tileSize + (m_gridHeight / 2.0f)};
		glm::vec2 gridNo{0,0};
		glm::vec2 pos = glm::modf(gridpos, gridNo);
		gridNo += glm::vec2(41,35);
		pos = pos * (float)m_textureTileSize - (m_textureTileSize / 2.0f);
		ImGui::Text("  Grid No: (%.2f, %.2f)", gridNo.x, gridNo.y);
		ImGui::Text("  Local Pos: (%.2f, %.2f)", pos.x, pos.y);
	}
	else
	{
		ImGui::Text("  No camera");
	}
	ImGui::End();

	// Scene tool window
	ImGui::Begin("Scene Tool");
	ImGui::Checkbox("Enable B1 Overlay", &m_enableB1Overlay);
	
	ImGui::Separator();
	ImGui::Text("Pattern ID:");
	ImGui::InputInt("##PatternID", &m_patternInput);
	if (ImGui::Button("Load Spots by Pattern"))
	{
		loadSpotsByPattern(m_patternInput);
	}

	ImGui::Separator();
	ImGui::Text("=== Pattern Filter Mode ===");
	ImGui::Checkbox("Enable Filter Mode", &m_filterMode);
	
	if (m_filterMode)
	{
		ImGui::Text("1. Select Map:");
		auto names = GameData::getInstance().getMapNames();
		auto mapCount = GameData::getInstance().getMapCount();
		if (ImGui::Combo("##MapSelect", &m_filterMapSelection, names.data(), mapCount))
		{
			// Clear marked spots when changing maps
			m_markedSpots.clear();
			loadSpotsByMap(m_filterMapSelection);
			auto patterns = GameData::getInstance().getPatternsByMap(m_filterMapSelection);
			m_filteredPatterns.clear();
			m_filteredPatterns.insert(patterns.begin(), patterns.end());
		}
		
		ImGui::Separator();
		ImGui::Text("2. Click spots to select:");
		if (!m_markedSpots.empty())
		{
			ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), 
				"Filter active: %zu spots marked", m_markedSpots.size());
		}
		
		// Show available variations for clicked spot
		if (m_currentSpotId > 0 && !m_availableVariations.empty())
		{
			ImGui::Text("Spot: %d", m_currentSpotId);
			ImGui::Text("Available Variations:");
			
			ImGui::BeginChild("VariationList", ImVec2(0, 200), true);
			for (size_t i = 0; i < m_availableVariations.size(); ++i)
			{
				const auto& var = m_availableVariations[i];
				bool isSelected = (m_selectedVariationIndex == static_cast<int>(i));
				
				if (ImGui::Selectable(var.info.getText().c_str(), isSelected))
				{
					m_selectedVariationIndex = static_cast<int>(i);
				}
				
				if (isSelected)
				{
					ImGui::Text("  ID: %d, Type: %d", var.info.variationId, var.info.variationType);
					ImGui::Text("  In %zu patterns", var.patterns.size());
				}
			}
			ImGui::EndChild();
			
			if (m_selectedVariationIndex >= 0 && ImGui::Button("Add to Filter"))
			{
				const auto& selectedVar = m_availableVariations[m_selectedVariationIndex];
				m_markedSpots[m_currentSpotId] = selectedVar.info.getKey();
				SDL_Log("Added filter: spot %d -> %s", m_currentSpotId, selectedVar.info.getText().c_str());
				
				// Reset all spot scales
				for (auto entity : m_mapSpotEntities)
				{
					if (!m_registry.valid(entity)) continue;
					auto* mapSpot = m_registry.try_get<MapSpot>(entity);
					if (!mapSpot) continue;
					if (mapSpot->metadata == m_currentSpotId)
					{
						updateSpot(entity, selectedVar.info);
					}
					mapSpot->selected = false;
				}

				if (selectedVar.patterns.size() > 0)
				{
					std::vector<int> inPatterns{m_filteredPatterns.begin(), m_filteredPatterns.end()};
					m_filteredPatterns = GameData::getInstance().filterByVariation(
						m_filteredPatterns, m_currentSpotId, selectedVar.info.getKey());
				}

				// Clear selection for next spot
				m_currentSpotId = -1;
				m_availableVariations.clear();
				m_selectedVariationIndex = -1;
			}
		}
		else if (m_currentSpotId <= 0)
		{
			ImGui::Text("No variations available at this spot");
		}
		
		ImGui::Separator();
		ImGui::Text("3. Filter patterns:");
		ImGui::Text("Marked Spots: %zu", m_markedSpots.size());
		
		if (ImGui::Button("Clear Marked Spots"))
		{
			m_markedSpots.clear();
			m_filteredPatterns.clear();
			auto patterns = GameData::getInstance().getPatternsByMap(m_filterMapSelection);
			m_filteredPatterns.insert(patterns.begin(), patterns.end());
		}
		
		// Display marked spots
		if (!m_markedSpots.empty())
		{
			ImGui::Separator();
			ImGui::Text("Marked Filters:");
			ImGui::BeginChild("MarkedSpots", ImVec2(0, 150), true);
			
			std::vector<int> toRemove;
			for (const auto& [spotId, varKey] : m_markedSpots)
			{
				auto varInfo = GameData::getInstance().getVariation(varKey);
				ImGui::PushID(std::to_string(spotId).c_str());
				ImGui::Text("%d -> %s", spotId, varInfo->sublabel.c_str());
				ImGui::SameLine();
				if (ImGui::SmallButton("Remove"))
				{
					toRemove.push_back(spotId);
				}
				ImGui::PopID();
			}
			
			// Remove spots marked for deletion
			for (const auto& key : toRemove)
			{
				m_markedSpots.erase(key);
				
				// Reset spot scale
				for (auto entity : m_mapSpotEntities)
				{
					if (m_registry.valid(entity))
					{
						auto* mapSpot = m_registry.try_get<MapSpot>(entity);
						if (mapSpot && mapSpot->metadata > 0 && mapSpot->metadata == key)
						{
							auto& transform = m_registry.get<Transform>(entity);
							transform.scale = glm::vec3(0.1f, 0.1f, 1.0f);
							break;
						}
					}
				}
			}
			
			ImGui::EndChild();
		}
		
		// Display filtered patterns
		if (!m_filteredPatterns.empty())
		{
			ImGui::Separator();
			ImGui::Text("Matching Patterns: %zu", m_filteredPatterns.size());
			ImGui::BeginChild("FilteredPatterns", ImVec2(0, 100), true);
			for (int patternId : m_filteredPatterns)
			{
				ImGui::Text("Pattern %d", patternId);
				ImGui::SameLine();
				if (ImGui::SmallButton(("Load##" + std::to_string(patternId)).c_str()))
				{
					loadSpotsByPattern(patternId);
					m_filterMode = false; // Exit filter mode after loading
				}
			}
			ImGui::EndChild();
		}
	}

	ImGui::End();
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

			// Load B1 overlay if enabled
			if (m_enableB1Overlay)
			{
				std::stringstream b1Stream;
				b1Stream << "nightreign/assets/textures/" << mapIndex << "/MENU_MapTile_L" << layer
						 << "_" << std::setw(2) << std::setfill('0') << x
						 << "_" << std::setw(2) << std::setfill('0') << y << "_B1.png";
				std::string b1TexturePath = b1Stream.str();

				std::stringstream b1NameStream;
				b1NameStream << "tile_" << mapIndex << "_L" << layer << "_" << x << "_" << y << "_B1";
				std::string b1TextureName = b1NameStream.str();

				// Try to load B1 texture (may not exist for all tiles)
				Texture* b1Texture = resMgr->loadTexture(b1TextureName, b1TexturePath);
				if (b1Texture && b1Texture->isValid())
				{
					// Create overlay entity
					auto overlayEntity = m_registry.create();
					m_registry.emplace<Tag>(overlayEntity, "map_b1");
					m_registry.emplace<MeshComponent>(overlayEntity, "quad", "texture", b1TextureName);
					m_registry.emplace<RenderOptions>(overlayEntity).order = 1;

					auto &overlayTransform = m_registry.emplace<Transform>(overlayEntity);
					overlayTransform.position = glm::vec3(offsetX + x * m_tileSize, offsetY + y * m_tileSize, 0.01f); // Slightly above base tile
					overlayTransform.scale = glm::vec3(m_tileSize, m_tileSize, 1.0f);

					// Store overlay entity for cleanup
					m_mapTileEntities.push_back(overlayEntity);
				}
			}
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

VariationInfo Scene::getFilterSpot() const
{
    VariationInfo temp{};
	temp.icon = "undefined";
	temp.label = "Mark Point";
	temp.visible = true;
	return temp;
}

VariationInfo Scene::getFilterStarter() const
{
	VariationInfo temp{};
	temp.icon = "launch";
	temp.label = "Launch Point";
	temp.visible = true;
	return temp;
}

entt::entity Scene::addSpot(const glm::vec2 &gridPos)
{
	float gridTotalWidth = m_gridWidth * m_tileSize;
	float gridTotalHeight = m_gridHeight * m_tileSize;
	float offsetX = -gridTotalWidth / 2.0f + m_tileSize / 2.0f;
	float offsetY = -gridTotalHeight / 2.0f + m_tileSize / 2.0f;

	// Convert grid position to world position
	float worldX = offsetX + gridPos.x * m_tileSize;
	float worldY = offsetY + gridPos.y * m_tileSize;

    // Create spot entity
	auto entity = m_registry.create();
	auto& mapSpot = m_registry.emplace<MapSpot>(entity);
	m_registry.emplace<MeshComponent>(entity);
	m_registry.emplace<RenderOptions>(entity, BlendType::Standard, 2.f);
	auto &transform = m_registry.emplace<Transform>(entity);
	transform.position = glm::vec3(worldX, worldY, 0.1f); // Slightly above tiles

	// Store entity for later cleanup
	m_mapSpotEntities.push_back(entity);

	return entity;
}

entt::entity Scene::addBaseSpot(const glm::vec2 &gridPos, int attachId, const VariationInfo &info)
{
	auto entity = addSpot(gridPos);
	auto& mapSpot = m_registry.get<MapSpot>(entity);
	mapSpot.metadata = attachId;
	m_registry.emplace<Tag>(entity, "base spot");
	updateSpot(entity, info);
    return entity;
}

entt::entity Scene::addFilterSpot(const glm::vec2 &gridPos, int spotId, const VariationInfo &info)
{
	auto entity = addSpot(gridPos);
	auto& mapSpot = m_registry.get<MapSpot>(entity);
	mapSpot.metadata = spotId;
	m_registry.emplace<Tag>(entity, "filter spot");
	updateSpot(entity, info);
    return entity;
}

entt::entity Scene::addStarterSpot(const glm::vec2 &gridPos, int starterId, const VariationInfo &info)
{
	auto entity = addSpot(gridPos);
	auto& mapSpot = m_registry.get<MapSpot>(entity);
	mapSpot.metadata = starterId;
	m_registry.emplace<Tag>(entity, "starter spot");
	updateSpot(entity, info);
    return entity;
}

void Scene::updateSpot(entt::entity entity, const VariationInfo &info)
{
	ResourceManager* resMgr = ResourceManager::getInstance();
	Texture* texture = nullptr;
	if (!info.icon.empty() && resMgr)
	{
		std::string iconPath = "nightreign/assets/textures/spots/" + info.icon + ".png";
		texture = resMgr->loadTexture(info.icon, iconPath);
		if(auto* meshComp = m_registry.try_get<MeshComponent>(entity))
		{
			meshComp->textureName = info.icon;
			meshComp->meshName = "quad";
			meshComp->shaderName = "texture";
		}
	}

	auto& transform = m_registry.get<Transform>(entity);
	if (texture && texture->isValid())
	{
		float scale  = MapSpot::iconSize * info.iconScale;
		float aspectRatio = static_cast<float>(texture->getWidth()) / static_cast<float>(texture->getHeight());
		if (aspectRatio < 1.0f && aspectRatio > 0.0f)
			transform.scale = glm::vec3(scale, scale / aspectRatio, 1.0f);
		else
			transform.scale = glm::vec3(scale * aspectRatio, scale, 1.0f);
	}
	else
	{
		transform.scale = glm::vec3(0, 0, 1.0f);
	}
	
	if (auto* mapSpot = m_registry.try_get<MapSpot>(entity))
	{
		mapSpot->label = info.getText();
		mapSpot->visible = info.visible;
	}
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
	auto& gameData = GameData::getInstance();
	auto* patternInfo = gameData.getPattern(patternId);
	auto distList = gameData.getDists(patternId);
	
	for (const auto dist: distList)
	{	
		auto spot = gameData.getAttach(dist->attachId);
		auto varInfo = gameData.getVariation(dist->patternId, dist->attachId);
		if (!spot || !varInfo) continue;
		auto pos = spot->normalize();
		
		addBaseSpot(pos, dist->attachId, *varInfo);
	}

	loadMapTiles(gameData.getPattern(patternId)->map, 0);

	m_currentPatternId = patternId;
}

void Scene::loadSpotsByMap(int map)
{
	// Clear existing spots
	clearSpots();
	auto& gameData = GameData::getInstance();
	auto staticSpots = gameData.getStaticSpotsByMap(map);
	for (auto spotId : staticSpots)
	{
		auto spot = gameData.getSpot(spotId);
		if (!spot) continue;
		auto pos = spot->normalize();
		auto tmp = getFilterSpot();
		addFilterSpot(pos, spotId, tmp);
	}

	auto starterSpots = gameData.getStarterSpotsByMap(map);
	for (auto starterId : starterSpots)
	{
		auto starter = gameData.getStarterSpot(starterId);
		if (!starter) continue;
		auto pos = starter->normalize();
		auto tmp = getFilterStarter();
		addStarterSpot(pos, starterId, tmp);
	}

	loadMapTiles(map, 0);

	m_currentPatternId = -1;
}

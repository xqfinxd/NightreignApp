#include "Scene.h"
#include "public.h"
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

float MapSpot::textScale = 0.003f;
float MapSpot::iconSize = 0.2f;

struct HitResult
{
	bool hit = false;
	float distance = 0.0f;
	glm::vec3 point{0.f};
	glm::vec3 normal{0.f};
};

struct Ray
{
	glm::vec3 origin;
	glm::vec3 direction;

	Ray(const Camera &camera, int x, int y, int w, int h)
	{
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

	HitResult intersectPlane(const glm::vec3 &planePoint, const glm::vec3 &planeNormal) const
	{
		HitResult result;

		float denom = glm::dot(planeNormal, direction);
		if (fabs(denom) > FLT_EPSILON)
		{
			glm::vec3 p0l0 = planePoint - origin;
			float t = glm::dot(p0l0, planeNormal) / denom;

			if (t >= 0)
			{
				result.hit = true;
				result.distance = t;
				result.point = origin + direction * t;
				result.normal = planeNormal;
			}
		}

		return result;
	}

	HitResult intersectSphere(const glm::vec3 &center, float radius) const
	{
		HitResult result;

		glm::vec3 oc = origin - center;
		float a = glm::dot(direction, direction);
		float b = 2.0f * glm::dot(oc, direction);
		float c = glm::dot(oc, oc) - radius * radius;
		float discriminant = b * b - 4 * a * c;

		if (discriminant > 0)
		{
			float t = (-b - sqrt(discriminant)) / (2.0f * a);
			if (t >= 0)
			{
				result.hit = true;
				result.distance = t;
				result.point = origin + direction * t;
				result.normal = glm::normalize(result.point - center);
			}
		}

		return result;
	}

	HitResult intersectAABB(const glm::vec3 &minBounds, const glm::vec3 &maxBounds) const
	{
		HitResult result;

		float tmin = 0.0f;
		float tmax = 100000.0f;

		for (int i = 0; i < 3; i++)
		{
			if (fabs(direction[i]) < 1e-6)
			{
				if (origin[i] < minBounds[i] || origin[i] > maxBounds[i])
					return result;
			}
			else
			{
				float ood = 1.0f / direction[i];
				float t1 = (minBounds[i] - origin[i]) * ood;
				float t2 = (maxBounds[i] - origin[i]) * ood;

				if (t1 > t2)
					std::swap(t1, t2);
				if (t1 > tmin)
					tmin = t1;
				if (t2 < tmax)
					tmax = t2;

				if (tmin > tmax)
					return result;
			}
		}

		if (tmin > 0)
		{
			result.hit = true;
			result.distance = tmin;
			result.point = origin + direction * tmin;

			glm::vec3 center = (minBounds + maxBounds) * 0.5f;
			glm::vec3 halfSize = (maxBounds - minBounds) * 0.5f;
			glm::vec3 localHit = result.point - center;

			float minDist = FLT_MAX;
			for (int i = 0; i < 3; i++)
			{
				float dist = fabs(halfSize[i] - fabs(localHit[i]));
				if (dist < minDist)
				{
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
	ResourceManager *resMgr = ResourceManager::getInstance();
	// Load shaders
	resMgr->loadShader("texture", "nightreign/assets/shaders/texture.vert", "nightreign/assets/shaders/texture.frag");
	resMgr->loadShader("font", "nightreign/assets/shaders/font.vert", "nightreign/assets/shaders/font.frag");

	// Create a simple quad mesh for tiles
	std::vector<Vertex> quadVertices = {
		Vertex(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f)),
		Vertex(glm::vec3(0.5f, -0.5f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec4(1.0f)),
		Vertex(glm::vec3(0.5f, 0.5f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f)),
		Vertex(glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec4(1.0f))};

	std::vector<uint32_t> quadIndices = {
		0, 1, 2,
		2, 3, 0};

	resMgr->createMesh("quad", quadVertices, quadIndices);

	// Load spot texture
	resMgr->loadTexture("spot_launch");

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
	setSpotClickCallback([this](entt::entity entity, const MapSpot &spot)
						 { handleSpotClick(entity, spot); });

	// Add some sample spots on the map with Chinese labels
	filterMap(0);

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

		auto *transform = m_registry.try_get<Transform>(entity);
		auto *mapSpot = m_registry.try_get<MapSpot>(entity);
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
		if (selectedIndex < 0 || static_cast<size_t>(selectedIndex) >= hitsAtLocation.size() - 1)
		{
			nextSelectedEntity = hitsAtLocation[0];
		}
		else
		{
			nextSelectedEntity = hitsAtLocation[selectedIndex + 1];
		}

		auto *mapSpot = m_registry.try_get<MapSpot>(nextSelectedEntity);
		auto *tag = m_registry.try_get<Tag>(nextSelectedEntity);
		bool interactable = tag && (tag->name == "starter spot" || tag->name == "filter spot") && mapSpot && mapSpot->metadata > 0;
		if (interactable)
		{
			m_spotClickCallback(nextSelectedEntity, *mapSpot);
		}
	}
}

void Scene::onMouseRightClick(int screenX, int screenY, int windowWidth, int windowHeight)
{
	// Only handle right click in filter mode
	if (!m_filterMode)
		return;

	auto camera = getCamera();
	if (!camera)
		return;

	Ray ray(*camera, screenX, screenY, windowWidth, windowHeight);

	// Clear previous context menu data
	m_contextMenuData.clear();

	// Collect all spots at this click location
	std::vector<std::pair<entt::entity, float>> hitsAtLocation;

	for (auto it = m_mapSpotEntities.rbegin(); it != m_mapSpotEntities.rend(); ++it)
	{
		auto entity = *it;
		if (!m_registry.valid(entity))
			continue;

		auto *transform = m_registry.try_get<Transform>(entity);
		auto *mapSpot = m_registry.try_get<MapSpot>(entity);
		auto *tag = m_registry.try_get<Tag>(entity);
		if (!transform || !mapSpot || !tag)
			continue;

		// Check if this is an interactable spot
		if (mapSpot->metadata <= 0)
			continue;

		// Only consider starter and filter spots
		if (tag->name != "filter spot" && tag->name != "starter spot")
			continue;

		// Check if click is within spot bounds
		float halfSize = (transform->scale.x * m_tileSize) * 0.5f;
		auto result = ray.intersectSphere(transform->position, halfSize);

		if (result.hit)
		{
			hitsAtLocation.push_back({entity, result.distance});
		}
	}

	// If no hits, return early
	if (hitsAtLocation.empty())
		return;

	// Sort by distance (closest first)
	std::sort(hitsAtLocation.begin(), hitsAtLocation.end(),
			  [](const auto &a, const auto &b)
			  { return a.second < b.second; });

	// Collect all spot types and IDs at this location
	auto &gameData = GameData::getInstance();
	std::map<int, size_t> uniqueVars; // For deduplicating variations

	for (const auto &[entity, distance] : hitsAtLocation)
	{
		auto *mapSpot = m_registry.try_get<MapSpot>(entity);
		auto *tag = m_registry.try_get<Tag>(entity);
		auto *transform = m_registry.try_get<Transform>(entity);

		if (!mapSpot || !tag || !transform)
			continue;

		m_contextMenuData.entities.push_back(entity);

		if (tag->name == "starter spot")
		{
			// Add starter ID
			m_contextMenuData.starterIds.push_back(mapSpot->metadata);
		}
		else if (tag->name == "filter spot")
		{
			// Add filter spot ID
			m_contextMenuData.filterSpotIds.push_back(mapSpot->metadata);

			// Collect variations for this spot
			auto results = gameData.getVariationsAtSpot(mapSpot->metadata, m_filteredPatterns);
			for (const auto &res : results)
			{
				auto outPatterns = gameData.filterByVariation(
					m_filteredPatterns, mapSpot->metadata, res->getKey());

				// Deduplicate variations by key
				auto it = uniqueVars.find(res->getKey());
				if (it != uniqueVars.end() && it->second < m_contextMenuData.variations.size())
				{
					// Merge patterns into existing variation
					m_contextMenuData.variations[it->second].patterns.insert(
						outPatterns.begin(), outPatterns.end());
					continue;
				}

				// Add new variation
				VariationOption var;
				var.info = *res;
				var.patterns.insert(outPatterns.begin(), outPatterns.end());
				m_contextMenuData.variations.push_back(var);
				uniqueVars[res->getKey()] = m_contextMenuData.variations.size() - 1;
			}
		}

		// Store world position from the first hit
		if (m_contextMenuData.worldPosition == glm::vec2(0.0f))
		{
			m_contextMenuData.worldPosition = glm::vec2(transform->position.x, transform->position.y);
		}
	}

	// Show context menu if we have any data
	if (!m_contextMenuData.isEmpty())
	{
		m_contextMenuPos = glm::vec2(screenX, screenY);
		m_showContextMenu = true;
	}
}

void Scene::handleSpotClick(entt::entity entity, const MapSpot &spot)
{
	// Handle filter mode
	if (m_filterMode && spot.metadata > 0)
	{
		m_currentSpotId = spot.metadata;
		m_availableVariations.clear();
		m_selectedVariationIndex = -1;

		auto &gameData = GameData::getInstance();
		auto results = gameData.getVariationsAtSpot(spot.metadata, m_filteredPatterns);
		std::map<int, size_t> uniqueVars;
		for (const auto &res : results)
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
	if (m_selectedSpotEntity != entt::null)
	{
		auto &prevSpot = m_registry.get<MapSpot>(m_selectedSpotEntity);
		prevSpot.selected = false;
	}

	if (m_selectedSpotEntity == entity)
	{
		m_selectedSpotEntity = entt::null;
	}
	else
	{
		m_selectedSpotEntity = entity;
	}

	if (m_selectedSpotEntity != entt::null)
	{
		auto &newSpot = m_registry.get<MapSpot>(m_selectedSpotEntity);
		newSpot.selected = true;
	}
}

void Scene::handleInput(const InputState &result, int windowWidth, int windowHeight)
{
	auto camera = getCamera();
	if (!camera)
		return;

	switch (result.type)
	{
	case InputState::eSingleTap:
	{
		// Single tap - trigger click
		onMouseRightClick(static_cast<int>(result.pos.x), static_cast<int>(result.pos.y), windowWidth, windowHeight);
		break;
	}

	case InputState::eDoubleTap:
	{
		// Double tap - zoom in with pos as center
		if (camera->isOrthographic)
		{
			// Calculate world position at tap location before zoom
			Ray ray(*camera, static_cast<int>(result.pos.x), static_cast<int>(result.pos.y), windowWidth, windowHeight);
			auto hitResult = ray.intersectPlane(glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
			
			if (hitResult.hit)
			{
				glm::vec3 worldPosBefore = hitResult.point;
				
				// Apply zoom
				float zoomFactor = 1.0f / result.scale;
				camera->orthoSize *= zoomFactor;
				camera->orthoSize = glm::clamp(camera->orthoSize, m_minZoom, m_maxZoom);
				
				// Recalculate world position at same screen location after zoom
				camera->updateMatrices();
				Ray rayAfter(*camera, static_cast<int>(result.pos.x), static_cast<int>(result.pos.y), windowWidth, windowHeight);
				auto hitResultAfter = rayAfter.intersectPlane(glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
				
				if (hitResultAfter.hit)
				{
					// Adjust camera to keep the tap point at the same screen location
					glm::vec3 worldPosAfter = hitResultAfter.point;
					glm::vec3 offset = worldPosBefore - worldPosAfter;
					camera->position += offset;
					camera->target += offset;
					camera->updateMatrices();
				}
			}
		}
		break;
	}

	case InputState::eDragMove:
	{
		// Drag move - pan camera
		if (camera->isOrthographic)
		{
			// Convert screen delta to world delta
			float worldDeltaScale = camera->orthoSize / static_cast<float>(windowHeight);
			
			// Calculate world space movement (inverted for natural drag feel)
			glm::vec3 right = glm::normalize(glm::cross(camera->target - camera->position, camera->up));
			glm::vec3 worldUp = camera->up;
			
			glm::vec3 movement = -right * result.delta.x * worldDeltaScale + 
			                     worldUp * result.delta.y * worldDeltaScale;
			
			// Update camera position and target
			camera->position += movement;
			camera->target += movement;
			camera->updateMatrices();
		}
		else
		{
			// Perspective camera
			float distance = glm::length(camera->position - camera->target);
			float worldDeltaScale = distance * 0.001f;
			
			glm::vec3 right = glm::normalize(glm::cross(camera->target - camera->position, camera->up));
			glm::vec3 worldUp = camera->up;
			
			glm::vec3 movement = -right * result.delta.x * worldDeltaScale + 
			                     worldUp * result.delta.y * worldDeltaScale;
			
			camera->position += movement;
			camera->target += movement;
			camera->updateMatrices();
		}
		break;
	}

	case InputState::eZoomLocal:
	{
		// Zoom with pos as center
		if (camera->isOrthographic)
		{
			// Calculate world position at zoom center before zoom
			Ray ray(*camera, static_cast<int>(result.pos.x), static_cast<int>(result.pos.y), windowWidth, windowHeight);
			auto hitResult = ray.intersectPlane(glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
			
			if (hitResult.hit)
			{
				glm::vec3 worldPosBefore = hitResult.point;
				
				// Apply zoom and pan adjustment from result.delta
				float zoomFactor = 1.0f / result.scale;
				camera->orthoSize *= zoomFactor;
				camera->orthoSize = glm::clamp(camera->orthoSize, m_minZoom, m_maxZoom);
				
				// Apply panning from multi-touch gesture
				float worldDeltaScale = camera->orthoSize / static_cast<float>(windowHeight);
				glm::vec3 right = glm::normalize(glm::cross(camera->target - camera->position, camera->up));
				glm::vec3 worldUp = camera->up;
				glm::vec3 panMovement = -right * result.delta.x * worldDeltaScale + 
				                        worldUp * result.delta.y * worldDeltaScale;
				
				camera->position += panMovement;
				camera->target += panMovement;
				
				// Recalculate world position at same screen location after zoom
				camera->updateMatrices();
				Ray rayAfter(*camera, static_cast<int>(result.pos.x), static_cast<int>(result.pos.y), windowWidth, windowHeight);
				auto hitResultAfter = rayAfter.intersectPlane(glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
				
				if (hitResultAfter.hit)
				{
					// Adjust camera to keep the zoom center at the same screen location
					glm::vec3 worldPosAfter = hitResultAfter.point;
					glm::vec3 offset = worldPosBefore - worldPosAfter;
					camera->position += offset;
					camera->target += offset;
					camera->updateMatrices();
				}
			}
		}
		break;
	}

	case InputState::eZoomCenter:
	{
		// Zoom with camera center as pivot (standard mouse wheel behavior)
		if (camera->isOrthographic)
		{
			float zoomFactor = 1.0f / result.scale;
			camera->orthoSize *= zoomFactor;
			camera->orthoSize = glm::clamp(camera->orthoSize, m_minZoom, m_maxZoom);
			camera->updateMatrices();
		}
		else
		{
			// Perspective camera - move closer or farther
			glm::vec3 direction = glm::normalize(camera->target - camera->position);
			float distance = glm::length(camera->position - camera->target);
			float zoomAmount = distance * (1.0f - 1.0f / result.scale);
			
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
			camera->updateMatrices();
		}
		break;
	}

	case InputState::eLongTouch:
	{
		// Long touch - trigger right click (context menu)
		onMouseRightClick(static_cast<int>(result.pos.x), static_cast<int>(result.pos.y), windowWidth, windowHeight);
		break;
	}

	case InputState::eNone:
	default:
		break;
	}
}



void Scene::render(Renderer *renderer)
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
	#ifdef _DEBUG
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
		glm::vec2 gridNo{0, 0};
		glm::vec2 pos = glm::modf(gridpos, gridNo);
		gridNo += glm::vec2(41, 35);
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
	if (ImGui::Checkbox("Enable B1 Overlay", &m_enableB1Overlay))
	{
		updateB1Overlay();
	}

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
			filterMap(m_filterMapSelection);
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
				const auto &var = m_availableVariations[i];
				bool isSelected = (m_selectedVariationIndex == static_cast<int>(i));

				// Build display label with label and sublabel
				std::string displayLabel;
				if (!var.info.label.empty() && !var.info.sublabel.empty())
				{
					displayLabel = var.info.label + " - " + var.info.sublabel;
				}
				else if (!var.info.label.empty())
				{
					displayLabel = var.info.label;
				}
				else
				{
					displayLabel = var.info.getText();
				}

				if (ImGui::Selectable(displayLabel.c_str(), isSelected))
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
				const auto &selectedVar = m_availableVariations[m_selectedVariationIndex];
				m_markedSpots[m_currentSpotId] = selectedVar.info.getKey();
				SDL_Log("Added filter: spot %d -> %s", m_currentSpotId, selectedVar.info.getText().c_str());

				// Reset all spot scales
				for (auto entity : m_mapSpotEntities)
				{
					if (!m_registry.valid(entity))
						continue;
					auto *mapSpot = m_registry.try_get<MapSpot>(entity);
					if (!mapSpot)
						continue;
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

		// Display marked spots
		if (!m_markedSpots.empty())
		{
			ImGui::Separator();
			ImGui::Text("Marked Filters:");
			ImGui::BeginChild("MarkedSpots", ImVec2(0, 150), true);

			std::vector<int> toRemove;
			for (const auto &[spotId, varKey] : m_markedSpots)
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
			for (const auto &key : toRemove)
			{
				m_markedSpots.erase(key);

				// Reset spot scale
				for (auto entity : m_mapSpotEntities)
				{
					if (m_registry.valid(entity))
					{
						auto *mapSpot = m_registry.try_get<MapSpot>(entity);
						if (mapSpot && mapSpot->metadata > 0 && mapSpot->metadata == key)
						{
							auto &transform = m_registry.get<Transform>(entity);
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
					onFilterPatterns(patternId);
					break;
				}
			}
			ImGui::EndChild();
		}
	}

	ImGui::End();
	#endif
	// Context menu (appears at mouse position)
	drawContextMenu();
}

void Scene::drawContextMenu()
{
	// Get display size
	ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	
	if (m_showContextMenu)
	{
		// Adjust position to keep menu within window bounds
		float menuX = m_contextMenuPos.x;
		float menuY = m_contextMenuPos.y;

		// Estimate menu size
		float estimatedWidth = SDL_min(displaySize.x * 0.9f, 600.0f);
		float estimatedHeight = 400.0f; // Will auto-resize

		// Check boundaries
		if (menuX + estimatedWidth > displaySize.x)
			menuX = displaySize.x - estimatedWidth - 10.0f;
		if (menuX < 10.0f)
			menuX = 10.0f;

		if (menuY + estimatedHeight > displaySize.y)
			menuY = displaySize.y - estimatedHeight - 10.0f;
		if (menuY < 10.0f)
			menuY = 10.0f;

		ImGui::SetNextWindowPos(ImVec2(menuX, menuY), ImGuiCond_Appearing);
		ImGui::OpenPopup("ContextMenuPopup");
		m_showContextMenu = false;
	}

	// Use Popup instead of regular window (auto-closes on click outside)
	if (ImGui::BeginPopup("ContextMenuPopup", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
	{
		auto &gameData = GameData::getInstance();

		// Display header
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), CHS("选项"));
		ImGui::Separator();

		// === Starter Options Section ===
		if (m_contextMenuData.hasStarters())
		{
			ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), CHS("初始点:"));
			ImGui::Indent();

			int validStarterCount = 0;
			for (int starterId : m_contextMenuData.starterIds)
			{
				auto starterSpot = gameData.getStarterSpot(starterId);
				if (!starterSpot)
					continue;

				// Calculate how many patterns this would leave
				auto resultPatterns = gameData.filterByStarter(m_filteredPatterns, starterId);

				// Skip if no patterns match
				if (resultPatterns.empty())
					continue;

				validStarterCount++;
				std::string label = CHS("应用此初始点");
#ifdef _DEBUG
				label += std::to_string(starterId);
				label += " (" + std::to_string(resultPatterns.size()) + " patterns)";
#endif

				if (ImGui::MenuItem(label.c_str()))
				{
					// Apply starter filter
					m_filteredPatterns = resultPatterns;

					// Mark all entities at this location as selected
					for (auto entity : m_contextMenuData.entities)
					{
						if (!m_registry.valid(entity))
							continue;
						auto *mapSpot = m_registry.try_get<MapSpot>(entity);
						auto *tag = m_registry.try_get<Tag>(entity);
						if (mapSpot && tag && tag->name == "starter spot" && mapSpot->metadata == starterId)
						{
							mapSpot->selected = true;
						}
					}

					// Auto-load if only one pattern remains
					if (m_filteredPatterns.size() == 1)
					{
						onFilterPatterns(*m_filteredPatterns.begin());
					}

					showToast("已应用初始点");

					// Close menu
					m_contextMenuData.clear();
					ImGui::CloseCurrentPopup();
				}
			}

			// Show message if no valid starters
			if (validStarterCount == 0)
			{
				ImGui::TextDisabled(CHS("不可用初始点"));
			}

			ImGui::Unindent();

			// Add separator if we also have filter options
			if (m_contextMenuData.hasFilters())
			{
				ImGui::Spacing();
			}
		}

		// === Filter/Variation Options Section ===
		if (m_contextMenuData.hasFilters())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), CHS("交互点:"));

			if (m_contextMenuData.variations.empty())
			{
				ImGui::Indent();
				ImGui::TextDisabled(CHS("无可用交互点"));
				ImGui::Unindent();
			}
			else
			{
				// Count valid variations (with patterns > 0)
				std::vector<size_t> validIndices;
				for (size_t i = 0; i < m_contextMenuData.variations.size(); ++i)
				{
					if (!m_contextMenuData.variations[i].patterns.empty())
						validIndices.push_back(i);
				}

				if (validIndices.empty())
				{
					ImGui::Indent();
					ImGui::TextDisabled(CHS("无可用交互点"));
					ImGui::Unindent();
				}
				else
				{
					// Use Table layout for better space utilization
					int numColumns = 1;
					if (validIndices.size() > 6)
						numColumns = 2; // 2 columns if many items
					if (validIndices.size() > 12)
						numColumns = 3; // 3 columns if very many items

					// Calculate table size to minimize scrolling
					float availWidth = SDL_min(displaySize.x * 0.85f, 700.0f);
					float availHeight = SDL_min(displaySize.y * 0.6f, 500.0f);

					ImVec2 tableSize(availWidth, 0); // Auto height

					// If too many items, set max height
					if (validIndices.size() > 15)
						tableSize.y = availHeight;

					if (ImGui::BeginTable("VariationsTable", numColumns,
										  ImGuiTableFlags_Borders |
											  ImGuiTableFlags_RowBg |
											  ImGuiTableFlags_ScrollY,
										  tableSize))
					{
						// Setup columns with equal width
						for (int col = 0; col < numColumns; ++col)
						{
							ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
						}

						// Render variations in table cells
						int cellIndex = 0;
						for (size_t idx : validIndices)
						{
							const auto &var = m_contextMenuData.variations[idx];

							// Start new row every numColumns items
							if (cellIndex % numColumns == 0)
								ImGui::TableNextRow();

							ImGui::TableSetColumnIndex(cellIndex % numColumns);

							// Build label
							std::string menuLabel;
							if (!var.info.label.empty() && !var.info.sublabel.empty())
							{
								menuLabel = var.info.label + " - " + var.info.sublabel;
							}
							else if (!var.info.label.empty())
							{
								menuLabel = var.info.label;
							}
							else
							{
								menuLabel = var.info.getText();
							}
#ifdef _DEBUG
							menuLabel += "\n(" + std::to_string(var.patterns.size()) + " patterns)";
#endif

							// Use Selectable for full cell click area
#ifdef __EMSCRIPTEN__
							// Mobile: larger buttons
							if (ImGui::Selectable(menuLabel.c_str(), false, ImGuiSelectableFlags_None, ImVec2(0, 50)))
#else
							// Desktop: normal size
							if (ImGui::Selectable(menuLabel.c_str(), false))
#endif
							{
								// Find which filter spot this variation belongs to
								int targetSpotId = m_contextMenuData.filterSpotIds[0];

								// Add to filter
								m_markedSpots[targetSpotId] = var.info.getKey();
								SDL_Log("Added filter: spot %d -> %s", targetSpotId, var.info.getText().c_str());

								// Update visual for all filter spots at this location
								for (auto entity : m_contextMenuData.entities)
								{
									if (!m_registry.valid(entity))
										continue;

									auto *mapSpot = m_registry.try_get<MapSpot>(entity);
									auto *tag = m_registry.try_get<Tag>(entity);

									if (!mapSpot || !tag)
										continue;

									if (tag->name == "filter spot" && mapSpot->metadata == targetSpotId)
									{
										updateSpot(entity, var.info);
									}

									mapSpot->selected = false;
								}

								// Apply filter
								m_filteredPatterns = gameData.filterByVariation(
									m_filteredPatterns, targetSpotId, var.info.getKey());

								// Auto-load if only one pattern remains
								if (m_filteredPatterns.size() == 1)
								{
									onFilterPatterns(*m_filteredPatterns.begin());
								}

								// Close menu
								m_contextMenuData.clear();
								ImGui::CloseCurrentPopup();
							}

							cellIndex++;
						}

						ImGui::EndTable();
					}
				}
			}
		}

		// Show message if empty
		if (m_contextMenuData.isEmpty())
		{
			ImGui::TextDisabled(CHS("无可用选项"));
		}

		ImGui::EndPopup();
	}
	else
	{
		m_contextMenuData.clear();
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
			// Generate texture alias
			std::stringstream nameStream;
			nameStream << "tile_" << mapIndex << "_L" << layer << "_"
					   << std::setw(2) << std::setfill('0') << x << "_"
					   << std::setw(2) << std::setfill('0') << y;
			std::string textureName = nameStream.str();

			// Load texture using alias
			resMgr->loadTexture(textureName);

			// Create tile entity
			auto entity = m_registry.create();
			m_registry.emplace<Tag>(entity, "tile");
			m_registry.emplace<MeshComponent>(entity, "quad", "texture", textureName);

			auto &transform = m_registry.emplace<Transform>(entity);
			transform.position = glm::vec3(offsetX + x * m_tileSize, offsetY + y * m_tileSize, 0.0f);
			transform.scale = glm::vec3(m_tileSize, m_tileSize, 1.0f);

			// Store entity for later cleanup
			m_mapTileEntities.push_back(entity);

			// Load B1 overlay if enabled
			// Generate B1 texture alias
			std::stringstream b1NameStream;
			b1NameStream << "tile_" << mapIndex << "_L" << layer << "_"
						 << std::setw(2) << std::setfill('0') << x << "_"
						 << std::setw(2) << std::setfill('0') << y << "_B1";
			std::string b1TextureName = b1NameStream.str();

			// Try to load B1 texture using alias (may not exist for all tiles)
			Texture *b1Texture = resMgr->loadTexture(b1TextureName);
			if (b1Texture && b1Texture->isValid())
			{
				// Create overlay entity
				auto overlayEntity = m_registry.create();
				m_registry.emplace<Tag>(overlayEntity, "tile_b1");
				m_registry.emplace<MeshComponent>(overlayEntity, "quad", "texture", b1TextureName, false);
				m_registry.emplace<RenderOptions>(overlayEntity).order = 1;

				auto &overlayTransform = m_registry.emplace<Transform>(overlayEntity);
				overlayTransform.position = glm::vec3(offsetX + x * m_tileSize, offsetY + y * m_tileSize, 0.01f); // Slightly above base tile
				overlayTransform.scale = glm::vec3(m_tileSize, m_tileSize, 1.0f);

				// Store overlay entity for cleanup
				m_mapTileEntities.push_back(overlayEntity);
			}
		}
	}

	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Scene: Loaded map tiles (index: %d, layer: %d, tiles: %dx%d)",
				   mapIndex, layer, m_gridWidth, m_gridHeight);
}

void Scene::onFilterPatterns(int patternId)
{
	loadSpotsByPattern(patternId);
	m_filterMode = false;

	resetMapFilters();
}

void Scene::resetMapFilters()
{
	m_markedSpots.clear();
	m_filteredPatterns.clear();
	m_currentSpotId = -1;
	m_availableVariations.clear();
	m_selectedVariationIndex = -1;
}

void Scene::filterMap(int map)
{
	m_filterMapSelection = map;
	m_filterMode = true;
	m_markedSpots.clear();
	loadSpotsByMap(m_filterMapSelection);
	auto& gameData = GameData::getInstance();
	m_filteredPatterns = gameData.filterByMap(m_filterMapSelection);

	auto mapNames = gameData.getMapNames();
	if (map >= 0 && map < mapNames.size()) {
		showToast(std::string("已切换至: ") + mapNames[map]);
	}
}

void Scene::filterNightlord(int nightlord)
{
	auto& gameData = GameData::getInstance();
	auto testPatterns = gameData.filterByNightlord(m_filteredPatterns, nightlord);
	if (!testPatterns.empty())
	{
		m_filteredPatterns = testPatterns;
		showToast("已应用夜王");
	}
	else
	{
		showToast("无符合条件的夜王，未应用");
	}
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
	temp.label = CHS("交互点");
	temp.visible = true;
	return temp;
}

VariationInfo Scene::getFilterStarter() const
{
	VariationInfo temp{};
	temp.icon = "launch";
	temp.label = CHS("启始点");
	temp.visible = true;
	return temp;
}

VariationInfo Scene::getPlayArea(int day) const
{
    VariationInfo temp{};
	temp.icon = "play_area";
	if (day == 1)
		temp.label = CHS("第一天");
	else if (day == 2)
		temp.label = CHS("第二天");
	temp.visible = true;
	temp.iconScale = 2.f;
	return temp;
}

void Scene::updateB1Overlay()
{
	for (auto entity : m_mapTileEntities)
	{
		if (!m_registry.valid(entity))
			continue;
		auto tagComp = m_registry.try_get<Tag>(entity);
		if (!tagComp || tagComp->name != "tile_b1")
			continue;
		auto *meshComp = m_registry.try_get<MeshComponent>(entity);
		if (!meshComp)
			continue;
		meshComp->visible = m_enableB1Overlay;
	}
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
	auto &mapSpot = m_registry.emplace<MapSpot>(entity);
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
	auto &mapSpot = m_registry.get<MapSpot>(entity);
	mapSpot.metadata = attachId;
	m_registry.emplace<Tag>(entity, "base spot");
	updateSpot(entity, info);
	return entity;
}

entt::entity Scene::addFilterSpot(const glm::vec2 &gridPos, int spotId, const VariationInfo &info)
{
	auto entity = addSpot(gridPos);
	auto &mapSpot = m_registry.get<MapSpot>(entity);
	mapSpot.metadata = spotId;
	auto &option = m_registry.get<RenderOptions>(entity);
	option.order = 3.f;
	m_registry.emplace<Tag>(entity, "filter spot");
	updateSpot(entity, info);
	return entity;
}

entt::entity Scene::addStarterSpot(const glm::vec2 &gridPos, int starterId, const VariationInfo &info)
{
	auto entity = addSpot(gridPos);
	auto &mapSpot = m_registry.get<MapSpot>(entity);
	mapSpot.metadata = starterId;
	auto &option = m_registry.get<RenderOptions>(entity);
	option.order = 4.f;
	m_registry.emplace<Tag>(entity, "starter spot");
	updateSpot(entity, info);
	return entity;
}

void Scene::updateSpot(entt::entity entity, const VariationInfo &info)
{
	ResourceManager *resMgr = ResourceManager::getInstance();
	Texture *texture = nullptr;
	if (!info.icon.empty() && resMgr)
	{
		// Use alias format: "spot_" + icon name
		std::string textureAlias = "spot_" + info.icon;
		texture = resMgr->loadTexture(textureAlias);
		if (auto *meshComp = m_registry.try_get<MeshComponent>(entity))
		{
			meshComp->textureName = textureAlias;
			meshComp->meshName = "quad";
			meshComp->shaderName = "texture";
		}
	}

	auto &transform = m_registry.get<Transform>(entity);
	if (texture && texture->isValid())
	{
		float scale = MapSpot::iconSize * info.iconScale;
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

	if (auto *mapSpot = m_registry.try_get<MapSpot>(entity))
	{
		mapSpot->label = info.getText();
		mapSpot->label += std::to_string(mapSpot->metadata);
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
	auto &gameData = GameData::getInstance();
	auto *patternInfo = gameData.getPattern(patternId);
	auto distList = gameData.getDists(patternId);

	for (const auto dist : distList)
	{
		auto spot = gameData.getAttach(dist->attachId);
		auto varInfo = gameData.getVariation(dist->patternId, dist->attachId);
		if (!spot || !varInfo)
			continue;
		auto pos = spot->normalize();

		addBaseSpot(pos, dist->attachId, *varInfo);
	}
	if (patternInfo)
	{
		auto day1Spot = getPlayArea(1);
		addBaseSpot(patternInfo->playArea1.normalize(), 1, day1Spot);
		auto day2Spot = getPlayArea(2);
		addBaseSpot(patternInfo->playArea2.normalize(), 2, day2Spot);
	}
	

	loadMapTiles(gameData.getPattern(patternId)->map, 0);

	m_currentPatternId = patternId;

	if (patternInfo)
	{
		// Update info panel with map details
		std::string htmlContent = "<h3>地图信息</h3>";
		htmlContent += "<p><strong>夜王：</strong>" + std::to_string(patternInfo->boss) + "</p>";
		htmlContent += "<p><strong>第一夜BOSS：</strong>" + std::to_string(patternInfo->bossId1) + "</p>";
		htmlContent += "<p><strong>第二夜BOSS：</strong>" + std::to_string(patternInfo->bossId2) + "</p>";
		if (patternInfo->extraBossId1 > 0)
			htmlContent += "<p><strong>第一夜额外BOSS：</strong>" + std::to_string(patternInfo->extraBossId1) + "</p>";
		if (patternInfo->extraBossId2 > 0)
			htmlContent += "<p><strong>第二夜额外BOSS：</strong>" + std::to_string(patternInfo->extraBossId2) + "</p>";
		// TODO: Events, Castle, Great Hollow, etc.
		setInfoPanelContent(htmlContent);
	}
}

void Scene::loadSpotsByMap(int map)
{
	// Clear existing spots
	clearSpots();
	auto &gameData = GameData::getInstance();
	auto staticSpots = gameData.getStaticSpotsByMap(map);
	for (auto spotId : staticSpots)
	{
		auto spot = gameData.getSpot(spotId);
		if (!spot)
			continue;
		auto pos = spot->normalize();
		auto tmp = getFilterSpot();
		addFilterSpot(pos, spotId, tmp);
	}

	auto starterSpots = gameData.getStarterSpotsByMap(map);
	for (auto starterId : starterSpots)
	{
		auto starter = gameData.getStarterSpot(starterId);
		if (!starter)
			continue;
		auto pos = starter->normalize();
		auto tmp = getFilterStarter();
		addStarterSpot(pos, starterId, tmp);
	}

	loadMapTiles(map, 0);

	m_currentPatternId = -1;

	// Update info panel with map details
	std::string htmlContent = "<h3>选择模式</h3>";
	htmlContent += "<p>点击下方地图 -> 选择目标地图</p>";
	htmlContent += "<p style=\"text-indent: 2em;\">点击初始点和交互点选择类型</p>";
	htmlContent += "<p style=\"text-indent: 2em;\">点击下方夜王按钮指定夜王</p>";
	htmlContent += "<p style=\"text-indent: 2em;\">建议优先选择初始点</p>";
	setInfoPanelContent(htmlContent);
}

#include "Scene.h"
#include "public.h"
#include "GameData.h"
#include "Renderer.h"
#include "components/Mesh.h"
#include "components/Transform.h"
#include "components/Map.h"
#include "components/RenderOptions.h"
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

Scene::Scene() : m_gameData(new GameDataDB)
{
}

Scene::~Scene()
{
}

void Scene::initialize()
{
	// Load game data from SQLite database
	if (!getGameData().open("nightreign/assets/datas/game_data.db"))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Application: Failed to load game data");
	}

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

	// Create default camera
	{
		auto& o = addObject("main camera");
		o.setTag("main camera");
		auto& camera = o.addComponent<Camera>();
		camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
		camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
		camera.updateMatrices();
	}

	// Add some sample spots on the map with Chinese labels
	filterMap(0);
}

void Scene::cleanup()
{
}

void Scene::update(float deltaTime)
{
	updateObjects();
}

std::vector<Entity*> Scene::Raycast(Camera& camera, int screenX, int screenY, int width, int height)
{
	std::vector<Entity*> hitsAtLocation;
	Ray ray(camera, screenX, screenY, width, height);
	auto interactables = findObjectsByComponent<Interactable>();
	for (auto* gameObject : interactables)
	{
		if (!gameObject) continue;

		Transform* transform = gameObject->getComponent<Transform>();
		float halfSize = (transform->scale.x * m_tileSize) * 0.5f;
		HitResult hitResult = ray.intersectSphere(transform->position, halfSize);

		if (hitResult.hit)
		{
			hitsAtLocation.push_back(gameObject);
		}
	}
	return hitsAtLocation;
}

void Scene::onSelectSpots(int screenX, int screenY, int width, int height)
{
#ifndef _DEBUG
	if (!m_filterMode)
		return;
#endif

	auto camera = getCamera();
	if (!camera) return;

	Ray ray(*camera, screenX, screenY, width, height);

	// Clear previous context menu data
	m_selectionData.clear();

	// Collect all spots at this click location
	auto hitsAtLocation = Raycast(*camera, screenX, screenY, width, height);
	if (hitsAtLocation.empty()) return;

	auto &gameData = getGameData();
	std::map<int, size_t> uniqueVars; // For deduplicating variations

	for (auto *o : hitsAtLocation)
	{
		const auto* mapSpot = o->getComponent<MapSpot>();
		const auto* transform = o->getComponent<Transform>();
		
		if (!mapSpot) continue;

		m_selectionData.entities.push_back(o->getEntity());

		if (o->hasTag("starter spot"))
		{
			m_selectionData.starterIds.push_back(mapSpot->metadata);
		}
		else if (o->hasTag("filter spot"))
		{
			// Add filter spot ID
			m_selectionData.attachIds.push_back(mapSpot->metadata);

			// Collect variations for this spot
			auto results = gameData.listVariations(mapSpot->metadata, m_filteredPatterns);
			for (const auto &res : results)
			{
				auto outPatterns = gameData.filterByVariation(
					m_filteredPatterns, mapSpot->metadata, res.getKey());

				// Deduplicate variations by key
				auto it = uniqueVars.find(res.getKey());
				if (it != uniqueVars.end() && it->second < m_selectionData.variations.size())
				{
					// Merge patterns into existing variation
					m_selectionData.variations[it->second].patterns.insert(
						outPatterns.begin(), outPatterns.end());
					continue;
				}

				// Add new variation
				VariationOption var;
				var.info = res;
				var.patterns.insert(outPatterns.begin(), outPatterns.end());
				m_selectionData.variations.push_back(var);
				uniqueVars[res.getKey()] = m_selectionData.variations.size() - 1;
			}
		}
		else if (o->hasTag("base spot"))
		{
			m_selectionData.attachIds.push_back(mapSpot->metadata);
		}

		// Store world position from the first hit
		if (m_selectionData.worldPosition == glm::vec2(0.0f))
		{
			m_selectionData.worldPosition = glm::vec2(transform->position.x, transform->position.y);
		}
	}

	// Show context menu if we have any data
	if (!m_selectionData.isEmpty())
	{
		m_contextMenuPos = glm::vec2(screenX, screenY);
		m_showContextMenu = true;
	}
}

void Scene::handleInput(const InputState &result, int width, int height)
{
	auto camera = getCamera();
	if (!camera)
		return;

	switch (result.type)
	{
	case InputState::eSingleTap:
	{
		// Single tap - trigger click
		onSelectSpots(static_cast<int>(result.pos.x), static_cast<int>(result.pos.y), width, height);
		break;
	}

	case InputState::eDoubleTap:
	{
		// Double tap - zoom in with pos as center
		if (camera->isOrthographic)
		{
			// Calculate world position at tap location before zoom
			Ray ray(*camera, static_cast<int>(result.pos.x), static_cast<int>(result.pos.y), width, height);
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
				Ray rayAfter(*camera, static_cast<int>(result.pos.x), static_cast<int>(result.pos.y), width, height);
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
			float worldDeltaScale = camera->orthoSize / static_cast<float>(height);
			
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
			Ray ray(*camera, static_cast<int>(result.pos.x), static_cast<int>(result.pos.y), width, height);
			auto hitResult = ray.intersectPlane(glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
			
			if (hitResult.hit)
			{
				glm::vec3 worldPosBefore = hitResult.point;
				
				// Apply zoom and pan adjustment from result.delta
				float zoomFactor = 1.0f / result.scale;
				camera->orthoSize *= zoomFactor;
				camera->orthoSize = glm::clamp(camera->orthoSize, m_minZoom, m_maxZoom);
				
				// Apply panning from multi-touch gesture
				float worldDeltaScale = camera->orthoSize / static_cast<float>(height);
				glm::vec3 right = glm::normalize(glm::cross(camera->target - camera->position, camera->up));
				glm::vec3 worldUp = camera->up;
				glm::vec3 panMovement = -right * result.delta.x * worldDeltaScale + 
				                        worldUp * result.delta.y * worldDeltaScale;
				
				camera->position += panMovement;
				camera->target += panMovement;
				
				// Recalculate world position at same screen location after zoom
				camera->updateMatrices();
				Ray rayAfter(*camera, static_cast<int>(result.pos.x), static_cast<int>(result.pos.y), width, height);
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
		onSelectSpots(static_cast<int>(result.pos.x), static_cast<int>(result.pos.y), width, height);
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
	if (!camera) return;
	
	// Render all entities with MeshComponent
	auto meshes = findObjectsByComponent<MeshComponent>();
	renderer->renderEntities(*camera, meshes);

	// Render spot labels with ImGui's font texture
	glm::vec4 black = glm::vec4(0.0f, 0.0f, 0.0f, 0.5f);
	glm::vec4 white = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	auto texts = findObjectsByComponent<TextComponent>();
	renderer->renderText(*camera, texts, black, white);
}

void Scene::drawUI()
{
	#ifdef _DEBUG
	// Scene statistics window
	ImGui::Begin("Scene View");
	ImGui::Text("ECS Statistics:");
	ImGui::Text("  Entities: %zu", getRegistry().size());
	ImGui::Text("  Alive: %zu", getRegistry().alive());

	ImGui::Separator();
	ImGui::Text("Map Info:");
	ImGui::Text("  Current Index: %d", m_currentMapIndex);
	ImGui::Text("  Current Layer: %d", m_currentLayer);
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
	static int patternInput = m_currentPatternId;
	ImGui::InputInt("##PatternID", &patternInput);
	if (ImGui::Button("Load Spots by Pattern"))
	{
		loadSpotsByPattern(patternInput);
	}

	ImGui::SeparatorText("Pattern Filter Mode");
	ImGui::Checkbox("Enable Filter Mode", &m_filterMode);

	if (m_filterMode)
	{
		ImGui::Text("1. Select Map:");
		auto names = getGameData().getMapNames();
		static int filterMapSelection = m_currentMapIndex;
		if (ImGui::Combo("##MapSelect", &filterMapSelection, names.data(), static_cast<int>(names.size())))
		{
			filterMap(filterMapSelection);
		}

		ImGui::Separator();
		ImGui::Text("2. Click spots to select:");
		
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

	static SelectionData cachedSelectionData;
	if (!m_selectionData.attachIds.empty())
		cachedSelectionData = m_selectionData;
	if (!cachedSelectionData.attachIds.empty())
	{
		ImGui::SeparatorText("Spot Viewer");
		std::vector<int> toCopy;
		ImGui::BeginChild("SelectedSpots", ImVec2(0, 150), true);

		for (auto id : cachedSelectionData.attachIds)
		{
			auto spot = getGameData().getSpot(id);
			ImGui::PushID(std::to_string(id).c_str());
			if (!spot)
				ImGui::Text("AttachId: %d (No point info)", id);
			else
				ImGui::Text("AttachId: %d (%d,%d:%.2f,%.2f(%.2f))", id,
					spot->point.getX(),
					spot->point.getZ(),
					spot->point.posX,
					spot->point.posZ,
					spot->point.height);
			ImGui::SameLine();
			if (ImGui::SmallButton("Copy"))
			{
				toCopy.push_back(id);
			}
			ImGui::PopID();
		}

		ImGui::EndChild();
		if (ImGui::Button("Copy All"))
			toCopy = cachedSelectionData.attachIds;

		std::string content;
		for (auto key : toCopy)
		{
			if (!content.empty()) {
				content.append("\n");
			}
			content.append(std::to_string(key));
		}
		if (!content.empty())
			SDL_SetClipboardText(content.c_str());
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
		auto &gameData = getGameData();

		// Display header
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), CHS("选项"));
		ImGui::Separator();

		// === Starter Options Section ===
		if (m_selectionData.hasStarters())
		{
			ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), CHS("初始点:"));
			ImGui::Indent();

			int validStarterCount = 0;
			for (int starterId : m_selectionData.starterIds)
			{
				auto starterSpot = getGameData().getStarter(starterId);
				if (!starterSpot) continue;

				// Calculate how many patterns this would leave
				auto availPatterns = getGameData().filterByStarter(m_filteredPatterns, starterId);

				// Skip if no patterns match
				if (availPatterns.empty()) continue;

				validStarterCount++;
				std::string label = CHS("应用此初始点");
#ifdef _DEBUG
				label += std::to_string(starterId);
				label += " (" + std::to_string(availPatterns.size()) + " patterns)";
#endif

				if (ImGui::MenuItem(label.c_str()))
				{
					// Apply starter filter
					m_filteredPatterns = availPatterns;

					// Mark all entities at this location as selected
					for (auto entity : m_selectionData.entities)
					{
						auto *o = findObject(entity);
						if (!o) continue;
						
						if (!o->hasTag("starter spot")) continue;

						auto *mapSpot = o->getComponent<MapSpot>();
						if (!mapSpot || mapSpot->metadata != starterId)
							continue;
						
						o->destroy();
					}

					// Auto-load if only one pattern remains
					if (m_filteredPatterns.size() == 1)
					{
						onFilterPatterns(*m_filteredPatterns.begin());
					}

					showToast("已应用初始点");

					// Close menu
					m_selectionData.clear();
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
			if (m_selectionData.hasFilters())
			{
				ImGui::Spacing();
			}
		}

		// === Filter/Variation Options Section ===
		if (m_selectionData.hasFilters())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), CHS("交互点:"));

			if (m_selectionData.variations.empty())
			{
				ImGui::Indent();
				ImGui::TextDisabled(CHS("无可用交互点"));
				ImGui::Unindent();
			}
			else
			{
				// Count valid variations (with patterns > 0)
				std::vector<size_t> validIndices;
				for (size_t i = 0; i < m_selectionData.variations.size(); ++i)
				{
					if (!m_selectionData.variations[i].patterns.empty())
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
							const auto &var = m_selectionData.variations[idx];

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
							ImVec2 cellSize{ 0,50 };
							if (ImGui::Selectable(menuLabel.c_str(), false, ImGuiSelectableFlags_None, cellSize))
							{
								std::set<int> outPatterns;
								for (auto entity : m_selectionData.entities)
								{
									auto o = findObject(entity);
									if (!o) continue;

									auto* mapSpot = o->getComponent<MapSpot>();
									if (!mapSpot) continue;

									if (!o->hasTag("filter spot")) continue;
									auto attachId = mapSpot->metadata;

									auto availPatterns = gameData.filterByVariation(m_filteredPatterns, attachId, var.info.getKey());
									if (availPatterns.empty()) continue;

									outPatterns.insert(availPatterns.begin(), availPatterns.end());
									updateSpotText(*o, var.info.label);
									updateSpotIcon(*o, var.info.icon, var.info.iconScale);
								}

								// Apply filter
								m_filteredPatterns = outPatterns;

								// Auto-load if only one pattern remains
								if (m_filteredPatterns.size() == 1)
								{
									onFilterPatterns(*m_filteredPatterns.begin());
								}

								// Close menu
								m_selectionData.clear();
								ImGui::CloseCurrentPopup();
								break;
							}

							cellIndex++;
						}

						ImGui::EndTable();
					}
				}
			}
		}

		// Show message if empty
		if (m_selectionData.isEmpty())
		{
			ImGui::TextDisabled(CHS("无可用选项"));
		}

		ImGui::EndPopup();
	}
	else
	{
		m_selectionData.clear();
	}
}

Camera *Scene::getCamera()
{
	return findComponent<Camera>();
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
			std::string textureName = MapTile::getAlias(mapIndex, x, y, layer, false);
			if (resMgr->queryTexture(textureName)) {
				// Load texture using alias
				resMgr->loadTexture(textureName);

				// Create tile entity
				auto& tileObject = retrieveObject(textureName);
				tileObject.setTag("tile");
				tileObject.addComponent<MeshComponent>("quad", "texture", textureName);
				tileObject.addComponent<RenderOptions>(BlendType::Standard, 0.f);
				tileObject.addComponent<MapTile>(mapIndex, x, y, layer, false);

				auto& transform = tileObject.addComponent<Transform>();
				transform.position = glm::vec3(offsetX + x * m_tileSize, offsetY + y * m_tileSize, 0.0f);
				transform.scale = glm::vec3(m_tileSize, m_tileSize, 1.0f);
			}

			// Generate B1 texture alias
			std::string b1TextureName = MapTile::getAlias(mapIndex, x, y, layer, true);
			if (resMgr->queryTexture(b1TextureName)) {
				// Try to load B1 texture using alias (may not exist for all tiles)
				Texture *b1Texture = resMgr->loadTexture(b1TextureName);

				// Create overlay entity
				auto& overlayObject = retrieveObject(b1TextureName);
				overlayObject.setEnabled(m_enableB1Overlay);
				overlayObject.setTag("tile @b1");
				overlayObject.addComponent<MeshComponent>("quad", "texture", b1TextureName, true);
				overlayObject.addComponent<RenderOptions>(BlendType::Standard, 1.f);
				overlayObject.addComponent<MapTile>(mapIndex, x, y, layer, true);

				auto& overlayTransform = overlayObject.addComponent<Transform>();
				overlayTransform.position = glm::vec3(offsetX + x * m_tileSize, offsetY + y * m_tileSize, 0.01f); // Slightly above base tile
				overlayTransform.scale = glm::vec3(m_tileSize, m_tileSize, 1.0f);

                m_overlayGrids.push_back(glm::ivec2(x, y));
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
	m_filteredPatterns.clear();
}

void Scene::filterMap(int map)
{
	m_filterMode = true;
	loadSpotsByMap(map);
	m_filteredPatterns = getGameData().filterByMap(map);

	auto mapNames = getGameData().getMapNames();
	if (map >= 0 && static_cast<size_t>(map) < mapNames.size()) {
		showToast(std::string("已切换至: ") + mapNames[map]);
	}
}

void Scene::filterNightlord(int nightlord)
{
	auto testPatterns = getGameData().filterByNightlord(m_filteredPatterns, nightlord);
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
	auto gameObjects = findObjectsByComponent<MapTile>();
	for (auto o : gameObjects) {
		o->destroy();
	}
}

bool Scene::isOverlayPoint(const MapPoint& point) const
{
	auto it = std::find_if(m_overlayGrids.begin(), m_overlayGrids.end(),
		[&point](const glm::ivec2& grid) {
			return grid.x == point.getX() && grid.y == point.getZ();
        });
    // no fall in overlay grids
	if (it == m_overlayGrids.end())
        return false;
    auto option = getGameData().getGridOption(m_currentMapIndex, point.getX(), point.getZ());
	if (!option)
		return false;
    return point.height < option->height;
}

void Scene::updatePanelInfo(const PatternView &patternInfo)
{
	auto& gameData = getGameData();
	// Update info panel with map details
	std::string htmlContent = "<h3>地图信息</h3>";
	htmlContent += "<p><strong>夜王: </strong>" + patternInfo.nightlordName + "</p>";
	htmlContent += "<p><strong>第一夜BOSS: </strong>" + patternInfo.day1BossName + "</p>";
	if (!patternInfo.day1ExtraBossName.empty())
	{
		htmlContent += "<p style=\"text-indent: 2em;\"><strong>额外BOSS: </strong>" + patternInfo.day1ExtraBossName + "</p>";
	}
	htmlContent += "<p><strong>第二夜BOSS: </strong>" + patternInfo.day2BossName + "</p>";
	if (!patternInfo.day2ExtraBossName.empty())
	{
		htmlContent += "<p style=\"text-indent: 2em;\"><strong>额外BOSS: </strong>" + patternInfo.day2ExtraBossName + "</p>";
	}
	
	if (!patternInfo.eventContent.empty())
	{
		htmlContent += "<p><strong>事件: </strong>" + patternInfo.eventContent + "</p>";
	}
	setInfoPanelContent(htmlContent);
}

VariationInfo Scene::getFilterSpot() const
{
	VariationInfo temp{};
	temp.icon = "undefined";
	temp.label = CHS("交互点");
	temp.showAll();
	return temp;
}

VariationInfo Scene::getFilterStarter() const
{
	VariationInfo temp{};
	temp.icon = "launch";
	temp.label = CHS("启始点");
	temp.showAll();
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
	temp.iconScale = 2.f;
	temp.showAll();
	return temp;
}

VariationInfo Scene::getConstant(const ConstantInfo& info) const
{
    VariationInfo temp{};
	temp.iconScale = 1.f;
    temp.label = info.label;
    temp.icon = info.icon;
    temp.iconScale = info.iconScale;
    temp.showAll();
    return temp;
}

VariationInfo Scene::getRottedPower() const
{
	VariationInfo temp{};
	temp.icon = "target";
	temp.showAll();
	return temp;
}

VariationInfo Scene::getGreatHollowSmallBoss(const GreatHollowBindingInfo& info) const
{
	VariationInfo temp{};
	temp.icon = info.icon;
	temp.label = info.label;
	temp.iconScale = info.iconScale;
	temp.visible = info.visible;
	return temp;
}

void Scene::updateB1Overlay()
{
	auto gameObjects = findObjectsByComponent<MapSpot>();
	{
		auto tiles = findObjectsByComponent<MapTile>();
		gameObjects.insert(gameObjects.end(), tiles.begin(), tiles.end());
	}
	
	for (auto o : gameObjects)
	{
		if (!o) continue;
		if (!o->hasTag("@b1")) continue;
		o->setEnabled(m_enableB1Overlay);
	}
}

Entity& Scene::addSpot(const std::string& name, const MapPoint &gridPos)
{
	float gridTotalWidth = m_gridWidth * m_tileSize;
	float gridTotalHeight = m_gridHeight * m_tileSize;
	float offsetX = -gridTotalWidth / 2.0f + m_tileSize / 2.0f;
	float offsetY = -gridTotalHeight / 2.0f + m_tileSize / 2.0f;

	// Convert grid position to world position
	auto normalizedPos = gridPos.normalize(m_textureTileSize);
	float worldX = offsetX + normalizedPos.x * m_tileSize;
	float worldY = offsetY + normalizedPos.y * m_tileSize;

	// Create spot entity
	auto& gameObject = addObject(name);
	gameObject.addComponent<MapSpot>().point = gridPos;

	auto& meshComp = gameObject.addComponent<MeshComponent>();
	meshComp.meshName = "quad";
	meshComp.shaderName = "texture";

	gameObject.addComponent<TextComponent>();

	gameObject.addComponent<RenderOptions>(BlendType::Standard, 2.f);

	auto &transform = gameObject.addComponent<Transform>();
	transform.position = glm::vec3(worldX, worldY, 0.1f); // Slightly above tiles

	return gameObject;
}

Entity& Scene::addBaseSpot(const MapPoint &gridPos, int attachId, const VariationInfo &info)
{
	std::string name = "base:";
	name += std::to_string(info.getKey()) + "-" + std::to_string(attachId);
	auto& gameObject = addSpot(name, gridPos);
	auto* mapSpot = gameObject.getComponent<MapSpot>();
	mapSpot->metadata = attachId;
	std::string tag = "base spot";
	
	auto* meshComp = gameObject.getComponent<MeshComponent>();

	auto* textComp = gameObject.getComponent<TextComponent>();
	textComp->visible = info.isShowText();
	meshComp->visible = info.isShowIcon();

	if (auto option = getGameData().getAttachOption(attachId))
	{
		textComp->offset = option->offset;
		textComp->direction = static_cast<TextComponent::Direction>(option->direction);

		meshComp->visible = meshComp->visible && option->showIcon;
	}
	if (isOverlayPoint(gridPos))
	{
        tag += " @b1";
    }

	gameObject.setTag(tag);
#ifdef _DEBUG
	gameObject.addComponent<Interactable>();
#endif
	updateSpotIcon(gameObject, info.icon, info.iconScale);
	updateSpotText(gameObject, info.getText());
	return gameObject;
}

Entity& Scene::addFilterSpot(const MapPoint &gridPos, int spotId, const VariationInfo &info)
{
	std::string name = "filter:";
	name += std::to_string(info.getKey()) + "-" + std::to_string(spotId);
	auto& gameObject = addSpot(name, gridPos);
	auto* mapSpot = gameObject.getComponent<MapSpot>();

	mapSpot->metadata = spotId;
	gameObject.getComponent<RenderOptions>()->order = 3.f;
	gameObject.setTag("filter spot");
	gameObject.addComponent<Interactable>();
	updateSpotIcon(gameObject, info.icon, info.iconScale);
	updateSpotText(gameObject, info.getText());
	return gameObject;
}

Entity& Scene::addStarterSpot(const MapPoint &gridPos, int starterId, const VariationInfo &info)
{
	std::string name = "starter:";
	name += std::to_string(info.getKey()) + "-" + std::to_string(starterId);
	auto& gameObject = addSpot(name, gridPos);
	auto* mapSpot = gameObject.getComponent<MapSpot>();

	mapSpot->metadata = starterId;
	gameObject.getComponent<RenderOptions>()->order = 4.f;;
	gameObject.setTag("starter spot");
	gameObject.addComponent<Interactable>();
	updateSpotIcon(gameObject, info.icon, info.iconScale);
	updateSpotText(gameObject, info.getText());
	return gameObject;
}

void Scene::updateSpotIcon(Entity& gameObject, const std::string& iconName, float iconScale)
{
	ResourceManager *resMgr = ResourceManager::getInstance();
	Texture *texture = nullptr;
	// Use alias format: "spot_" + icon name
	std::string alias = "spot_" + iconName;
	if (resMgr->queryTexture(alias))
	{
		texture = resMgr->loadTexture(alias);
		auto* meshComp = gameObject.getComponent<MeshComponent>();
		meshComp->textureName = alias;
	}
	else
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Scene: '%s' using invalid texture '%s'",
			gameObject.getName().c_str(), alias.c_str());
	}

	auto* transform = gameObject.getComponent<Transform>();
	if (texture && texture->isValid())
	{
		float scale = MapSpot::iconSize * iconScale;
		float aspectRatio = static_cast<float>(texture->getWidth()) / static_cast<float>(texture->getHeight());
		if (aspectRatio < 1.0f && aspectRatio > 0.0f)
			transform->scale = glm::vec3(scale, scale / aspectRatio, 1.0f);
		else
			transform->scale = glm::vec3(scale * aspectRatio, scale, 1.0f);
	}
	else
	{
		transform->scale = glm::vec3(0, 0, 1.0f);
	}
}

void Scene::updateSpotText(Entity& gameObject, const std::string& text)
{
	auto* textComp = gameObject.getComponent<TextComponent>();
	textComp->text = text;
}

void Scene::clearSpots()
{
	auto gameObjects = findObjectsByComponent<MapSpot>();
	for (auto o : gameObjects) {
		o->destroy();
	}
}

void Scene::loadSpotsByPattern(int patternId)
{
	if (patternId < 0) return;
	auto patternView = getGameData().getPatternView(patternId);
	if (!patternView) return;

	// Clear existing spots
	clearSpots();
	
	// load map tiles for this pattern
	loadMapTiles(patternView->mapId, 0);

    // Set current pattern ID
	m_currentPatternId = patternId;

    // Add spots based on distribution
	for (const auto& sc : patternView->spots)
	{
		if (sc.smallBase.flag == SpotFlag_None)
			continue;

		VariationInfo varInfo{};
		varInfo.icon = sc.smallBase.iconAlias;
		varInfo.iconScale = 1.f;
		varInfo.label = sc.smallBase.majorName;
		varInfo.sublabel = sc.smallBase.minorName;
		varInfo.variationId = sc.smallBase.smallBaseId;
		varInfo.variationType = sc.smallBase.variationId;
		varInfo.visible = static_cast<int>(sc.smallBase.flag);
		addBaseSpot(sc.attachPoint.point, sc.attachPoint.attachId, varInfo);
	}
	
    // Add play area spots
	auto day1Spot = getPlayArea(1);
	day1Spot.label += "\n" + patternView->day1BossName;
	if (!patternView->day1ExtraBossName.empty())
		day1Spot.label += "\n" + patternView->day1ExtraBossName;
	addBaseSpot(patternView->day1PlayArea, 1, day1Spot);
	auto day2Spot = getPlayArea(2);
	day2Spot.label += "\n" + patternView->day2BossName;
	if (!patternView->day2ExtraBossName.empty())
		day2Spot.label += "\n" + patternView->day2ExtraBossName;
	addBaseSpot(patternView->day2PlayArea, 2, day2Spot);

    // Reset overlay state when loading a new pattern
	updateB1Overlay();

	updatePanelInfo(patternView.value());
}

void Scene::loadSpotsByMap(int map)
{
	if (map < 0) return;
	auto& gameData = getGameData();

	// Clear existing spots
	clearSpots();

	// load map tiles for this map (use layer 0 as default)
	loadMapTiles(map, 0);

    // Reset pattern and filters since we're just browsing the map
	m_currentPatternId = -1;

	auto mapView = gameData.getMapView(map);
    // Add all spots for this map for filtering
	for (auto& attachPoint : mapView.attachPoints)
	{
		auto option = gameData.getSpotOption(attachPoint.attachId);
		if (option && option->disable_filter)
			continue;
		auto tmp = getFilterSpot();
		addFilterSpot(attachPoint.point, attachPoint.attachId, tmp);
	}

    // Add starter spots for this map for filtering
	for (auto& starter : mapView.starters)
	{
		auto tmp = getFilterStarter();
		addStarterSpot(starter.point, starter.starterId, tmp);
	}

	// Update info panel with map details
	std::string htmlContent = "<h3>选择模式</h3>";
	htmlContent += "<p>点击下方地图 -> 选择目标地图</p>";
	htmlContent += "<p style=\"text-indent: 2em;\">点击初始点和交互点选择类型</p>";
	htmlContent += "<p style=\"text-indent: 2em;\">点击下方夜王按钮指定夜王</p>";
	htmlContent += "<p style=\"text-indent: 2em;\">建议优先选择初始点</p>";
	setInfoPanelContent(htmlContent);
}

EntityManager::EntityManager() : m_registry(new entt::registry) {
}

EntityManager::~EntityManager() {
	clearObjects();
	m_registry->clear();
}

Entity& EntityManager::addObject(const std::string& name)
{
	std::string newName = name;
	auto it = m_name2Entities.find(name);
	int tryTimes = 0;
	while (it != m_name2Entities.end())
	{
		newName = name + " clone " + std::to_string(tryTimes + 1);
		it = m_name2Entities.find(newName);
		tryTimes++;
	}
	auto newObject = m_entities.emplace_back(new Entity(m_registry, newName));
	m_name2Entities[newObject->getName()] = newObject;
	m_entt2Entities[newObject->getEntity()] = newObject;
	return *newObject;
}

Entity& EntityManager::retrieveObject(const std::string& name)
{
	auto it = m_name2Entities.find(name);
	if (it == m_name2Entities.end()) {
		return addObject(name);
	}
	else
	{
		it->second->reuse();
		return *it->second;
	}
}

bool EntityManager::removeObject(Entity*& object) {
	if (object && object->deleted()) {
		m_name2Entities.erase(object->getName());
		m_entt2Entities.erase(object->getEntity());
		delete object;
		object = nullptr;
		return true;
	}
	return false;
}

// cleanup
void EntityManager::clearObjects() {
	std::for_each(m_entities.begin(), m_entities.end(), [this](Entity* o) { o->destroy(); });
	updateObjects();
}

// update
void EntityManager::updateObjects() {
	auto newEnd = std::partition(m_entities.begin(), m_entities.end(),
		[this](Entity*& o) { return !removeObject(o); });

	m_entities.erase(newEnd, m_entities.end());
}

Entity* EntityManager::findObject(const std::string& name) {
	auto it = m_name2Entities.find(name);
	if (it != m_name2Entities.end())
		return it->second;
	return nullptr;
}

Entity* EntityManager::findObject(entt::entity entity) {
	auto it = m_entt2Entities.find(entity);
	if (it != m_entt2Entities.end())
		return it->second;
	return nullptr;
}

Entity::Entity(std::shared_ptr<entt::registry> registry, const std::string& name)
	: m_name(name) {
	if (registry) {
		m_entity = registry->create();
		m_registry = registry;
	}
}

Entity::~Entity() {
	if (valid()) {
		m_registry->destroy(m_entity);
		m_entity = entt::null;
	}
}

#include "SceneManager.h"
#include "Scene.h"
#include <SDL_log.h>

SceneManager::SceneManager()
{
}

SceneManager::~SceneManager()
{
	cleanup();
}

void SceneManager::initialize()
{
	SDL_Log("SceneManager initialized");
}

void SceneManager::cleanup()
{
	// Clean up all scenes
	for (auto& pair : m_scenes) {
		if (pair.second) {
			pair.second->cleanup();
			delete pair.second;
		}
	}
	m_scenes.clear();
	m_active_scene = nullptr;
	m_active_scene_name.clear();
	
	SDL_Log("SceneManager cleanup");
}

void SceneManager::update(float deltaTime)
{
	if (m_active_scene) {
		m_active_scene->update(deltaTime);
	}
}

void SceneManager::draw()
{
	if (m_active_scene) {
		m_active_scene->draw();
	}
}

void SceneManager::addScene(const std::string& name, Scene* scene)
{
	if (!scene) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot add null scene: %s", name.c_str());
		return;
	}

	// Remove existing scene with same name if it exists
	removeScene(name);

	m_scenes[name] = scene;
	scene->initialize();
	
	SDL_Log("Scene added: %s", name.c_str());

	// If no active scene, set this as active
	if (!m_active_scene) {
		setActiveScene(name);
	}
}

void SceneManager::removeScene(const std::string& name)
{
	auto it = m_scenes.find(name);
	if (it != m_scenes.end()) {
		// If removing active scene, clear it
		if (it->second == m_active_scene) {
			m_active_scene = nullptr;
			m_active_scene_name.clear();
		}

		it->second->cleanup();
		delete it->second;
		m_scenes.erase(it);
		
		SDL_Log("Scene removed: %s", name.c_str());
	}
}

void SceneManager::setActiveScene(const std::string& name)
{
	auto it = m_scenes.find(name);
	if (it != m_scenes.end()) {
		m_active_scene = it->second;
		m_active_scene_name = name;
		SDL_Log("Active scene set to: %s", name.c_str());
	} else {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Scene not found: %s", name.c_str());
	}
}

Scene* SceneManager::getScene(const std::string& name)
{
	auto it = m_scenes.find(name);
	return (it != m_scenes.end()) ? it->second : nullptr;
}

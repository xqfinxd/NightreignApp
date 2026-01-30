#pragma once
#include "public.h"
#include <string>
#include <unordered_map>

class Scene;
class Renderer;

class SceneManager
{
public:
	SceneManager();
	~SceneManager();

	void initialize();
	void cleanup();

	void update(float deltaTime);
	void draw(Renderer *renderer);

	// Scene management
	void addScene(const std::string& name, class Scene* scene);
	void removeScene(const std::string& name);
	void setActiveScene(const std::string& name);
	Scene* getActiveScene() const { return m_active_scene; }
	Scene* getScene(const std::string& name);

private:
	std::unordered_map<std::string, Scene*> m_scenes;
	Scene* m_active_scene = nullptr;
	std::string m_active_scene_name;
};

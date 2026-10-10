
#include "Scene.h"

GameObject* Scene::CreateObject(const std::string& name)
{
    auto object = std::make_unique<GameObject>(nextId++, name);

    GameObject* result = object.get();
    objects.push_back(std::move(object));

    return result;
}

bool Scene::DeleteObject(int id)
{
    for (auto it = objects.begin(); it != objects.end(); ++it)
    {
        if (*it && (*it)->id == id)
        {
            objects.erase(it);
            return true;
        }
    }

    return false;
}

std::vector<std::unique_ptr<GameObject>>& Scene::GetObjects()
{
    return objects;
}

const std::vector<std::unique_ptr<GameObject>>& Scene::GetObjects() const
{
    return objects;
}
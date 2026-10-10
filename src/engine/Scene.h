
#pragma once

#include "GameObject.h"

#include <memory>
#include <vector>
#include <string>

class Scene
{
private:
    std::vector<std::unique_ptr<GameObject>> objects;
    int nextId = 1;

public:
    GameObject* CreateObject(const std::string& name);
    bool DeleteObject(int id);

    std::vector<std::unique_ptr<GameObject>>& GetObjects();
    const std::vector<std::unique_ptr<GameObject>>& GetObjects() const;
};
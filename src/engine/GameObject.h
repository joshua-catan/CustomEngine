
#pragma once

#include <string>

struct Vector3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

class GameObject
{
public:
    int id = 0;
    std::string name;

    Vector3 position;
    Vector3 rotation;
    Vector3 scale = { 1.0f, 1.0f, 1.0f };

    GameObject(int objectId, const std::string& objectName)
        : id(objectId), name(objectName)
    {
    }
};
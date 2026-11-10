
#pragma once

#include "GameObject.h"
#include <glm/glm.hpp>

class Renderer
{
private:
    unsigned int shaderProgram = 0;
    unsigned int VAO = 0;
    unsigned int VBO = 0;

    unsigned int gizmoVAO = 0;
    unsigned int gizmoVBO = 0;

    unsigned int framebuffer = 0;
    unsigned int colorTexture = 0;
    unsigned int depthBuffer = 0;

    int framebufferWidth = 0;
    int framebufferHeight = 0;

    int modelLocation = -1;
    int viewLocation = -1;
    int projectionLocation = -1;
    int colorLocation = -1;

    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};

    bool CreateFramebuffer(int width, int height);
    bool CreateShader();

    void DrawGizmoLines(
        const float* vertices,
        int vertexCount
    );

public:
    bool Initialize();

    void BeginScene(
        int width,
        int height,
        const Vector3& cameraPosition,
        float cameraYaw,
        float cameraPitch
    );

    void DrawCube(
        float x, float y, float z,
        float rotationX, float rotationY, float rotationZ,
        float scaleX, float scaleY, float scaleZ
    );

    void DrawGizmo(
        const Vector3& position,
        int tool,
        float size = 1.5f
    );

    bool ProjectToScreen(
        const Vector3& worldPosition,
        float& screenX,
        float& screenY,
        float& depth
    ) const;

    unsigned int GetSceneTexture() const;

    void Shutdown();
};
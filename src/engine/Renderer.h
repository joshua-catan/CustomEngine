
#pragma once

class Renderer
{
private:
    unsigned int shaderProgram = 0;
    unsigned int VAO = 0;
    unsigned int VBO = 0;

    unsigned int framebuffer = 0;
    unsigned int colorTexture = 0;
    unsigned int depthBuffer = 0;

    int framebufferWidth = 0;
    int framebufferHeight = 0;

    int modelLocation = -1;
    int viewLocation = -1;
    int projectionLocation = -1;

public:
    bool Initialize();

    void BeginScene(int width, int height);
    unsigned int GetSceneTexture() const;

    void DrawCube(
        float x, float y, float z,
        float rotationX, float rotationY, float rotationZ,
        float scaleX, float scaleY, float scaleZ
    );

    void Shutdown();
};
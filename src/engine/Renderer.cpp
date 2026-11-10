
#include "Renderer.h"

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>

static const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
uniform vec3 uColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(uColor, 1.0);
}
)";

bool Renderer::CreateShader()
{
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    int success = 0;

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char info[1024] = {};
        glGetShaderInfoLog(vertexShader, sizeof(info), nullptr, info);
        std::cerr << "Vertex shader error:\n" << info << '\n';
        glDeleteShader(vertexShader);
        return false;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char info[1024] = {};
        glGetShaderInfoLog(fragmentShader, sizeof(info), nullptr, info);
        std::cerr << "Fragment shader error:\n" << info << '\n';
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (!success)
    {
        char info[1024] = {};
        glGetProgramInfoLog(shaderProgram, sizeof(info), nullptr, info);
        std::cerr << "Shader link error:\n" << info << '\n';
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        return false;
    }

    modelLocation = glGetUniformLocation(shaderProgram, "uModel");
    viewLocation = glGetUniformLocation(shaderProgram, "uView");
    projectionLocation = glGetUniformLocation(shaderProgram, "uProjection");
    colorLocation = glGetUniformLocation(shaderProgram, "uColor");

    return true;
}

bool Renderer::CreateFramebuffer(int width, int height)
{
    if (width < 1 || height < 1)
        return false;

    if (framebuffer != 0 &&
        framebufferWidth == width &&
        framebufferHeight == height)
    {
        return true;
    }

    if (framebuffer != 0)
    {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &colorTexture);
        glDeleteRenderbuffers(1, &depthBuffer);

        framebuffer = 0;
        colorTexture = 0;
        depthBuffer = 0;
    }

    framebufferWidth = width;
    framebufferHeight = height;

    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        colorTexture,
        0
    );

    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);

    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH24_STENCIL8,
        width,
        height
    );

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        depthBuffer
    );

    bool complete =
        glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
        GL_FRAMEBUFFER_COMPLETE;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (!complete)
        std::cerr << "Scene framebuffer is incomplete.\n";

    return complete;
}

bool Renderer::Initialize()
{
    if (!CreateShader())
        return false;

    const float cubeVertices[] = {
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
         0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,

        -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,

        -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,

         0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
         0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,

        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
         0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,

        -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(cubeVertices),
        cubeVertices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        3 * sizeof(float), nullptr
    );

    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &gizmoVAO);
    glGenBuffers(1, &gizmoVBO);

    glBindVertexArray(gizmoVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gizmoVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        0,
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        3 * sizeof(float), nullptr
    );

    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glEnable(GL_DEPTH_TEST);

    return true;
}

void Renderer::BeginScene(
    int width,
    int height,
    const Vector3& cameraPosition,
    float cameraYaw,
    float cameraPitch
)
{
    width = std::max(width, 1);
    height = std::max(height, 1);

    if (!CreateFramebuffer(width, height))
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.12f, 0.13f, 0.15f, 1.0f);

    glClear(
        GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT |
        GL_STENCIL_BUFFER_BIT
    );

    glm::vec3 position(
        cameraPosition.x,
        cameraPosition.y,
        cameraPosition.z
    );

    float yaw = glm::radians(cameraYaw);
    float pitch = glm::radians(cameraPitch);

    glm::vec3 front(
        std::cos(yaw) * std::cos(pitch),
        std::sin(pitch),
        std::sin(yaw) * std::cos(pitch)
    );

    front = glm::normalize(front);

    viewMatrix = glm::lookAt(
        position,
        position + front,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    projectionMatrix = glm::perspective(
        glm::radians(60.0f),
        static_cast<float>(width) / static_cast<float>(height),
        0.05f,
        1000.0f
    );

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(
        viewLocation,
        1,
        GL_FALSE,
        glm::value_ptr(viewMatrix)
    );

    glUniformMatrix4fv(
        projectionLocation,
        1,
        GL_FALSE,
        glm::value_ptr(projectionMatrix)
    );
}

void Renderer::DrawCube(
    float x, float y, float z,
    float rotationX, float rotationY, float rotationZ,
    float scaleX, float scaleY, float scaleZ
)
{
    glm::mat4 model(1.0f);

    model = glm::translate(model, glm::vec3(x, y, z));

    model = glm::rotate(
        model, glm::radians(rotationX), glm::vec3(1, 0, 0)
    );

    model = glm::rotate(
        model, glm::radians(rotationY), glm::vec3(0, 1, 0)
    );

    model = glm::rotate(
        model, glm::radians(rotationZ), glm::vec3(0, 0, 1)
    );

    model = glm::scale(model, glm::vec3(scaleX, scaleY, scaleZ));

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(
        modelLocation,
        1,
        GL_FALSE,
        glm::value_ptr(model)
    );

    glUniform3f(colorLocation, 0.72f, 0.75f, 0.80f);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Renderer::DrawGizmoLines(
    const float* vertices,
    int vertexCount
)
{
    if (!vertices || vertexCount <= 0)
        return;

    glUseProgram(shaderProgram);

    glm::mat4 model(1.0f);

    glUniformMatrix4fv(
        modelLocation,
        1,
        GL_FALSE,
        glm::value_ptr(model)
    );

    glBindVertexArray(gizmoVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gizmoVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            vertexCount * 3 * sizeof(float)
        ),
        vertices,
        GL_DYNAMIC_DRAW
    );

    glDrawArrays(GL_LINES, 0, vertexCount);
}

void Renderer::DrawGizmo(
    const Vector3& position,
    int tool,
    float size
)
{
    if (framebufferWidth <= 0 || framebufferHeight <= 0)
        return;

    glm::vec3 origin(
        position.x,
        position.y,
        position.z
    );

    glm::vec4 clip =
        projectionMatrix *
        viewMatrix *
        glm::vec4(origin, 1.0f);

    if (clip.w <= 0.05f)
        return;

    size = glm::clamp(size, 0.1f, 100.0f);

    std::vector<float> vertices;

    auto addLine = [&](const glm::vec3& a, const glm::vec3& b)
    {
        vertices.push_back(a.x);
        vertices.push_back(a.y);
        vertices.push_back(a.z);

        vertices.push_back(b.x);
        vertices.push_back(b.y);
        vertices.push_back(b.z);
    };

    const glm::vec3 axes[3] = {
        glm::vec3(1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 1)
    };

    const glm::vec3 colors[3] = {
        glm::vec3(1.0f, 0.16f, 0.13f),
        glm::vec3(0.20f, 1.0f, 0.25f),
        glm::vec3(0.18f, 0.48f, 1.0f)
    };

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(
        viewLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix)
    );

    glUniformMatrix4fv(
        projectionLocation, 1, GL_FALSE, glm::value_ptr(projectionMatrix)
    );

    glDisable(GL_DEPTH_TEST);
    glLineWidth(3.0f);

    if (tool == 0)
    {
        for (int axis = 0; axis < 3; ++axis)
        {
            glm::vec3 direction = axes[axis];
            glm::vec3 tip = origin + direction * size;
            glm::vec3 base = origin + direction * (size * 0.78f);

            glm::vec3 helper =
                std::abs(direction.y) > 0.9f
                ? glm::vec3(0, 0, 1)
                : glm::vec3(0, 1, 0);

            glm::vec3 side = glm::normalize(
                glm::cross(direction, helper)
            );

            glm::vec3 otherSide = glm::normalize(
                glm::cross(direction, side)
            );

            vertices.clear();

            addLine(origin, tip);
            addLine(base, base + side * size * 0.10f);
            addLine(base, base - side * size * 0.10f);
            addLine(base, base + otherSide * size * 0.10f);
            addLine(base, base - otherSide * size * 0.10f);

            glUniform3fv(colorLocation, 1, &colors[axis].x);

            DrawGizmoLines(
                vertices.data(),
                static_cast<int>(vertices.size() / 3)
            );
        }
    }
    else if (tool == 1)
    {
        for (int axis = 0; axis < 3; ++axis)
        {
            vertices.clear();

            glm::vec3 u = axes[(axis + 1) % 3];
            glm::vec3 v = axes[(axis + 2) % 3];

            const int segments = 96;

            for (int i = 0; i < segments; ++i)
            {
                float a0 =
                    (static_cast<float>(i) / segments) *
                    glm::two_pi<float>();

                float a1 =
                    (static_cast<float>(i + 1) / segments) *
                    glm::two_pi<float>();

                glm::vec3 p0 =
                    origin +
                    (u * std::cos(a0) + v * std::sin(a0)) * size;

                glm::vec3 p1 =
                    origin +
                    (u * std::cos(a1) + v * std::sin(a1)) * size;

                addLine(p0, p1);
            }

            glUniform3fv(colorLocation, 1, &colors[axis].x);

            DrawGizmoLines(
                vertices.data(),
                static_cast<int>(vertices.size() / 3)
            );
        }
    }
    else
    {
        for (int axis = 0; axis < 3; ++axis)
        {
            glm::vec3 direction = axes[axis];
            glm::vec3 end = origin + direction * size;

            glm::vec3 helper =
                std::abs(direction.y) > 0.9f
                ? glm::vec3(0, 0, 1)
                : glm::vec3(0, 1, 0);

            glm::vec3 side1 = glm::normalize(
                glm::cross(direction, helper)
            );

            glm::vec3 side2 = glm::normalize(
                glm::cross(direction, side1)
            );

            vertices.clear();

            addLine(origin, end);

            addLine(
                end - side1 * size * 0.08f,
                end + side1 * size * 0.08f
            );

            addLine(
                end - side2 * size * 0.08f,
                end + side2 * size * 0.08f
            );

            glUniform3fv(colorLocation, 1, &colors[axis].x);

            DrawGizmoLines(
                vertices.data(),
                static_cast<int>(vertices.size() / 3)
            );
        }
    }

    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(0);
}

bool Renderer::ProjectToScreen(
    const Vector3& worldPosition,
    float& screenX,
    float& screenY,
    float& depth
) const
{
    glm::vec4 clip =
        projectionMatrix *
        viewMatrix *
        glm::vec4(
            worldPosition.x,
            worldPosition.y,
            worldPosition.z,
            1.0f
        );

    if (clip.w <= 0.0f)
        return false;

    glm::vec3 ndc = glm::vec3(clip) / clip.w;

    screenX = (ndc.x * 0.5f + 0.5f) * framebufferWidth;

    screenY = (1.0f - (ndc.y * 0.5f + 0.5f)) * framebufferHeight;

    depth = ndc.z;

    return depth >= -1.0f && depth <= 1.0f;
}

unsigned int Renderer::GetSceneTexture() const
{
    return colorTexture;
}

void Renderer::Shutdown()
{
    if (VBO)
        glDeleteBuffers(1, &VBO);

    if (VAO)
        glDeleteVertexArrays(1, &VAO);

    if (gizmoVBO)
        glDeleteBuffers(1, &gizmoVBO);

    if (gizmoVAO)
        glDeleteVertexArrays(1, &gizmoVAO);

    if (framebuffer)
        glDeleteFramebuffers(1, &framebuffer);

    if (colorTexture)
        glDeleteTextures(1, &colorTexture);

    if (depthBuffer)
        glDeleteRenderbuffers(1, &depthBuffer);

    if (shaderProgram)
        glDeleteProgram(shaderProgram);

    VBO = VAO = 0;
    gizmoVBO = gizmoVAO = 0;
    framebuffer = colorTexture = depthBuffer = 0;
    shaderProgram = 0;

    framebufferWidth = 0;
    framebufferHeight = 0;
}
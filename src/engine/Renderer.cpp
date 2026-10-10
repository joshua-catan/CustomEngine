
#include "Renderer.h"

#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdio>

static const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(0.35, 0.65, 1.0, 1.0);
}
)";

static void IdentityMatrix(float* m)
{
    for (int i = 0; i < 16; ++i)
        m[i] = 0.0f;

    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void MultiplyMatrix(const float* a, const float* b, float* result)
{
    float temp[16] = {};

    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int k = 0; k < 4; ++k)
            {
                temp[column * 4 + row] +=
                    a[k * 4 + row] * b[column * 4 + k];
            }
        }
    }

    for (int i = 0; i < 16; ++i)
        result[i] = temp[i];
}

static void TranslateMatrix(float* m, float x, float y, float z)
{
    IdentityMatrix(m);
    m[12] = x;
    m[13] = y;
    m[14] = z;
}

static void ScaleMatrix(float* m, float x, float y, float z)
{
    IdentityMatrix(m);
    m[0] = x;
    m[5] = y;
    m[10] = z;
}

static void RotateX(float* m, float angle)
{
    IdentityMatrix(m);

    float c = cosf(angle);
    float s = sinf(angle);

    m[5] = c;
    m[6] = s;
    m[9] = -s;
    m[10] = c;
}

static void RotateY(float* m, float angle)
{
    IdentityMatrix(m);

    float c = cosf(angle);
    float s = sinf(angle);

    m[0] = c;
    m[2] = -s;
    m[8] = s;
    m[10] = c;
}

static void RotateZ(float* m, float angle)
{
    IdentityMatrix(m);

    float c = cosf(angle);
    float s = sinf(angle);

    m[0] = c;
    m[1] = s;
    m[4] = -s;
    m[5] = c;
}

static void PerspectiveMatrix(
    float* m,
    float fov,
    float aspect,
    float nearPlane,
    float farPlane)
{
    for (int i = 0; i < 16; ++i)
        m[i] = 0.0f;

    if (aspect <= 0.0f)
        aspect = 1.0f;

    float tanHalfFov = tanf(fov * 0.5f);

    m[0] = 1.0f / (aspect * tanHalfFov);
    m[5] = 1.0f / tanHalfFov;
    m[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    m[11] = -1.0f;
    m[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
}

static unsigned int CompileShader(GLenum type, const char* source)
{
    unsigned int shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success != GL_TRUE)
    {
        char infoLog[1024] = {};
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);

        std::fprintf(stderr, "Shader compilation error:\n%s\n", infoLog);

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

bool Renderer::Initialize()
{
    unsigned int vertexShader =
        CompileShader(GL_VERTEX_SHADER, vertexShaderSource);

    if (vertexShader == 0)
        return false;

    unsigned int fragmentShader =
        CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    if (fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        return false;
    }

    shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success = GL_FALSE;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (success != GL_TRUE)
    {
        char infoLog[1024] = {};
        glGetProgramInfoLog(shaderProgram, sizeof(infoLog), nullptr, infoLog);

        std::fprintf(stderr, "Shader linking error:\n%s\n", infoLog);

        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        return false;
    }

    const float vertices[] = {
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,0.5f,-0.5f,
         0.5f,0.5f,-0.5f, -0.5f,0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f,0.5f,   0.5f,-0.5f,0.5f,   0.5f,0.5f,0.5f,
         0.5f,0.5f,0.5f,  -0.5f,0.5f,0.5f,  -0.5f,-0.5f,0.5f,
        -0.5f,0.5f,0.5f,  -0.5f,0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f,-0.5f,-0.5f,-0.5f,0.5f, -0.5f,0.5f,0.5f,
         0.5f,0.5f,0.5f,   0.5f,0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
         0.5f,-0.5f,-0.5f, 0.5f,-0.5f,0.5f,  0.5f,0.5f,0.5f,
        -0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f,-0.5f,0.5f,
         0.5f,-0.5f,0.5f, -0.5f,-0.5f,0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,0.5f,-0.5f, 0.5f,0.5f,-0.5f,  0.5f,0.5f,0.5f,
         0.5f,0.5f,0.5f, -0.5f,0.5f,0.5f,  -0.5f,0.5f,-0.5f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr
    );

    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    modelLocation = glGetUniformLocation(shaderProgram, "model");
    viewLocation = glGetUniformLocation(shaderProgram, "view");
    projectionLocation = glGetUniformLocation(shaderProgram, "projection");

    glGenFramebuffers(1, &framebuffer);

    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenRenderbuffers(1, &depthBuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        colorTexture,
        0
    );

    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);

    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH24_STENCIL8,
        1,
        1
    );

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        depthBuffer
    );

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_DEPTH_TEST);

    return true;
}

void Renderer::BeginScene(int width, int height)
{
    if (width <= 0 || height <= 0 || shaderProgram == 0)
        return;

    if (width != framebufferWidth || height != framebufferHeight)
    {
        framebufferWidth = width;
        framebufferHeight = height;

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

        glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);

        glRenderbufferStorage(
            GL_RENDERBUFFER,
            GL_DEPTH24_STENCIL8,
            width,
            height
        );
    }

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);

    glClearColor(0.12f, 0.13f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::fprintf(stderr, "Scene framebuffer is incomplete.\n");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    float view[16];
    TranslateMatrix(view, 0.0f, 0.0f, -5.0f);

    float projection[16];

    PerspectiveMatrix(
        projection,
        45.0f * 3.1415926535f / 180.0f,
        static_cast<float>(width) / static_cast<float>(height),
        0.1f,
        100.0f
    );

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, view);
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projection);
}

unsigned int Renderer::GetSceneTexture() const
{
    return colorTexture;
}

void Renderer::DrawCube(
    float x, float y, float z,
    float rotationX, float rotationY, float rotationZ,
    float scaleX, float scaleY, float scaleZ)
{
    if (shaderProgram == 0 || VAO == 0)
        return;

    float translation[16];
    float rx[16], ry[16], rz[16], scale[16];
    float temp1[16], temp2[16], temp3[16], model[16];

    TranslateMatrix(translation, x, y, z);

    RotateX(rx, rotationX * 3.1415926535f / 180.0f);
    RotateY(ry, rotationY * 3.1415926535f / 180.0f);
    RotateZ(rz, rotationZ * 3.1415926535f / 180.0f);

    ScaleMatrix(scale, scaleX, scaleY, scaleZ);

    MultiplyMatrix(ry, rx, temp1);
    MultiplyMatrix(rz, temp1, temp2);
    MultiplyMatrix(temp2, scale, temp3);
    MultiplyMatrix(translation, temp3, model);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, model);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void Renderer::Shutdown()
{
    if (VBO != 0)
        glDeleteBuffers(1, &VBO);

    if (VAO != 0)
        glDeleteVertexArrays(1, &VAO);

    if (shaderProgram != 0)
        glDeleteProgram(shaderProgram);

    if (depthBuffer != 0)
        glDeleteRenderbuffers(1, &depthBuffer);

    if (colorTexture != 0)
        glDeleteTextures(1, &colorTexture);

    if (framebuffer != 0)
        glDeleteFramebuffers(1, &framebuffer);

    VBO = 0;
    VAO = 0;
    shaderProgram = 0;
    depthBuffer = 0;
    colorTexture = 0;
    framebuffer = 0;
}

#define GLFW_INCLUDE_NONE

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cstdint>
#include <cstdio>
#include <iostream>

#include "engine/Scene.h"
#include "engine/Renderer.h"

#include "external/imgui/imgui.h"
#include "external/imgui/backends/imgui_impl_glfw.h"
#include "external/imgui/backends/imgui_impl_opengl3.h"

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW.\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        1280, 800, "mestEngine", nullptr, nullptr
    );

    if (!window)
    {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glGetError();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 3.0f;
    style.FrameRounding = 3.0f;
    style.WindowBorderSize = 1.0f;

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true) ||
        !ImGui_ImplOpenGL3_Init("#version 330"))
    {
        std::cerr << "Failed to initialize ImGui.\n";
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    Scene scene;
    Renderer renderer;

    if (!renderer.Initialize())
    {
        std::cerr << "Failed to initialize renderer.\n";
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    GameObject* cube = scene.CreateObject("Cube");
    cube->position = {0.0f, 0.0f, 0.0f};

    scene.CreateObject("Light");

    int selectedId = cube->id;
    bool wasDragging = false;
    ImVec2 previousMousePosition(0.0f, 0.0f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();

        int screenWidth;
        int screenHeight;
        glfwGetWindowSize(window, &screenWidth, &screenHeight);

        float leftWidth = 230.0f;
        float rightWidth = 270.0f;
        float consoleHeight = 140.0f;
        float topHeight = static_cast<float>(screenHeight) - consoleHeight;

        if (topHeight < 200.0f)
            topHeight = 200.0f;

        if (screenWidth < 700)
        {
            leftWidth = 170.0f;
            rightWidth = 190.0f;
        }

        float sceneWidth =
            static_cast<float>(screenWidth) - leftWidth - rightWidth;

        if (sceneWidth < 100.0f)
            sceneWidth = 100.0f;

        ImGuiWindowFlags fixedFlags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse;

        ImGui::SetNextWindowPos(
            ImVec2(0.0f, 0.0f), ImGuiCond_Always
        );
        ImGui::SetNextWindowSize(
            ImVec2(leftWidth, topHeight), ImGuiCond_Always
        );
        ImGui::Begin("Hierarchy", nullptr, fixedFlags);

        for (const auto& object : scene.GetObjects())
        {
            if (!object)
                continue;

            ImGui::PushID(object->id);

            if (ImGui::Selectable(
                    object->name.c_str(),
                    selectedId == object->id))
            {
                selectedId = object->id;
            }

            ImGui::PopID();
        }

        if (ImGui::Button("Create Cube"))
        {
            GameObject* newObject = scene.CreateObject("Cube");
            selectedId = newObject->id;
        }

        if (ImGui::Button("Create Light"))
        {
            GameObject* newObject = scene.CreateObject("Light");
            selectedId = newObject->id;
        }

        if (ImGui::Button("Delete Selected"))
        {
            if (scene.DeleteObject(selectedId))
            {
                selectedId = -1;

                const auto& objects = scene.GetObjects();

                if (!objects.empty())
                    selectedId = objects.back()->id;
            }
        }

        ImGui::End();

        GameObject* selectedObject = nullptr;

        for (const auto& object : scene.GetObjects())
        {
            if (object && object->id == selectedId)
            {
                selectedObject = object.get();
                break;
            }
        }

        ImGui::SetNextWindowPos(
            ImVec2(
                static_cast<float>(screenWidth) - rightWidth,
                0.0f
            ),
            ImGuiCond_Always
        );
        ImGui::SetNextWindowSize(
            ImVec2(rightWidth, topHeight), ImGuiCond_Always
        );
        ImGui::Begin("Inspector", nullptr, fixedFlags);

        if (selectedObject)
        {
            char nameBuffer[128];

            std::snprintf(
                nameBuffer,
                sizeof(nameBuffer),
                "%s",
                selectedObject->name.c_str()
            );

            if (ImGui::InputText(
                    "Name", nameBuffer, sizeof(nameBuffer)))
            {
                selectedObject->name = nameBuffer;
            }

            ImGui::Separator();
            ImGui::Text("Transform");

            ImGui::DragFloat3(
                "Position",
                &selectedObject->position.x,
                0.1f
            );

            ImGui::DragFloat3(
                "Rotation",
                &selectedObject->rotation.x,
                1.0f
            );

            ImGui::DragFloat3(
                "Scale",
                &selectedObject->scale.x,
                0.1f,
                0.01f,
                100.0f
            );
        }
        else
        {
            ImGui::Text("No object selected.");
        }

        ImGui::End();

        ImGui::SetNextWindowPos(
            ImVec2(leftWidth, 0.0f), ImGuiCond_Always
        );
        ImGui::SetNextWindowSize(
            ImVec2(sceneWidth, topHeight), ImGuiCond_Always
        );
        ImGui::Begin("Scene", nullptr, fixedFlags);

        ImGui::Text("Select an object and drag in the scene to rotate it.");

        ImVec2 available = ImGui::GetContentRegionAvail();

        int sceneWidthPixels = static_cast<int>(available.x);
        int sceneHeightPixels = static_cast<int>(available.y);

        if (sceneWidthPixels < 1)
            sceneWidthPixels = 1;

        if (sceneHeightPixels < 1)
            sceneHeightPixels = 1;

        renderer.BeginScene(sceneWidthPixels, sceneHeightPixels);

        for (const auto& object : scene.GetObjects())
        {
            if (!object)
                continue;

            renderer.DrawCube(
                object->position.x,
                object->position.y,
                object->position.z,
                object->rotation.x,
                object->rotation.y,
                object->rotation.z,
                object->scale.x,
                object->scale.y,
                object->scale.z
            );
        }

        ImVec2 imageSize(
            static_cast<float>(sceneWidthPixels),
            static_cast<float>(sceneHeightPixels)
        );

        ImGui::Image(
            (ImTextureID)(intptr_t)renderer.GetSceneTexture(),
            imageSize,
            ImVec2(0.0f, 1.0f),
            ImVec2(1.0f, 0.0f)
        );

        bool imageHovered = ImGui::IsItemHovered();

        if (imageHovered &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            wasDragging = true;
            previousMousePosition = io.MousePos;
        }

        if (wasDragging &&
            ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
            selectedObject)
        {
            float deltaX =
                io.MousePos.x - previousMousePosition.x;

            float deltaY =
                io.MousePos.y - previousMousePosition.y;

            selectedObject->rotation.x += deltaY * 0.5f;
            selectedObject->rotation.y += deltaX * 0.5f;

            previousMousePosition = io.MousePos;
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            wasDragging = false;

        ImGui::End();

        ImGui::SetNextWindowPos(
            ImVec2(
                0.0f,
                static_cast<float>(screenHeight) - consoleHeight
            ),
            ImGuiCond_Always
        );
        ImGui::SetNextWindowSize(
            ImVec2(
                static_cast<float>(screenWidth),
                consoleHeight
            ),
            ImGuiCond_Always
        );
        ImGui::Begin("Console", nullptr, fixedFlags);

        ImGui::Text("mestEngine is running.");
        ImGui::Text(
            "Game Objects: %d",
            static_cast<int>(scene.GetObjects().size())
        );

        ImGui::End();

        ImGui::Render();

        int displayWidth;
        int displayHeight;

        glfwGetFramebufferSize(
            window, &displayWidth, &displayHeight
        );

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    renderer.Shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
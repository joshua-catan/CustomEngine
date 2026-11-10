#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "engine/Scene.h"
#include "engine/Renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

enum class TransformTool
{
    Move = 0,
    Rotate = 1,
    Scale = 2
};

enum class PivotMode
{
    Pivot,
    Center
};

static Scene scene;
static Renderer renderer;

static GameObject* selectedObject = nullptr;

static TransformTool currentTool = TransformTool::Move;
static PivotMode pivotMode = PivotMode::Pivot;

static Vector3 cameraPosition{0.0f, 1.5f, 7.0f};
static float cameraYaw = -90.0f;
static float cameraPitch = -10.0f;
static float cameraSpeed = 5.0f;
static float mouseSensitivity = 0.12f;

static bool previousW = false;
static bool previousE = false;
static bool previousR = false;

static bool draggingGizmo = false;
static int draggingAxis = -1;

static Vector3 dragStartPosition{};
static Vector3 dragStartRotation{};
static Vector3 dragStartScale{};

static double dragStartMouseX = 0.0;
static double dragStartMouseY = 0.0;

static float dragStartAngle = 0.0f;
static float dragPreviousAngle = 0.0f;
static float dragAccumulatedAngle = 0.0f;

static double previousMouseX = 0.0;
static double previousMouseY = 0.0;
static bool firstMouse = true;

static bool sceneViewportHovered = false;
static bool sceneViewportFocused = false;

static bool IsRightMouseDown(GLFWwindow* window)
{
    return glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
}

static void SetTool(TransformTool tool)
{
    currentTool = tool;
    draggingGizmo = false;
    draggingAxis = -1;
}

static void HandleCamera(GLFWwindow* window, float deltaTime)
{
    if (!IsRightMouseDown(window))
    {
        firstMouse = true;
        return;
    }

    double mouseX;
    double mouseY;

    glfwGetCursorPos(window, &mouseX, &mouseY);

    if (firstMouse)
    {
        previousMouseX = mouseX;
        previousMouseY = mouseY;
        firstMouse = false;
    }

    float mouseDeltaX = static_cast<float>(mouseX - previousMouseX);
    float mouseDeltaY = static_cast<float>(previousMouseY - mouseY);

    previousMouseX = mouseX;
    previousMouseY = mouseY;

    cameraYaw += mouseDeltaX * mouseSensitivity;
    cameraPitch += mouseDeltaY * mouseSensitivity;

    cameraPitch = std::clamp(cameraPitch, -89.0f, 89.0f);

    const float radians = 3.14159265359f / 180.0f;

    Vector3 forward{
        std::cos(cameraYaw * radians) * std::cos(cameraPitch * radians),
        std::sin(cameraPitch * radians),
        std::sin(cameraYaw * radians) * std::cos(cameraPitch * radians)
    };

    const float length = std::sqrt(
        forward.x * forward.x +
        forward.y * forward.y +
        forward.z * forward.z
    );

    if (length > 0.0001f)
    {
        forward.x /= length;
        forward.y /= length;
        forward.z /= length;
    }

    Vector3 right{
        -std::sin(cameraYaw * radians),
        0.0f,
        std::cos(cameraYaw * radians)
    };

    const float movement = cameraSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        cameraPosition.x += forward.x * movement;
        cameraPosition.y += forward.y * movement;
        cameraPosition.z += forward.z * movement;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        cameraPosition.x -= forward.x * movement;
        cameraPosition.y -= forward.y * movement;
        cameraPosition.z -= forward.z * movement;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        cameraPosition.x -= right.x * movement;
        cameraPosition.z -= right.z * movement;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        cameraPosition.x += right.x * movement;
        cameraPosition.z += right.z * movement;
    }

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        cameraPosition.y += movement;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        cameraPosition.y -= movement;
    }
}

static void HandleShortcuts(GLFWwindow* window)
{
    ImGuiIO& io = ImGui::GetIO();

    bool wDown = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool eDown = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
    bool rDown = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;

    bool wPressed = wDown && !previousW;
    bool ePressed = eDown && !previousE;
    bool rPressed = rDown && !previousR;

    previousW = wDown;
    previousE = eDown;
    previousR = rDown;

    if (io.WantTextInput)
        return;

    if (IsRightMouseDown(window))
        return;

    if (wPressed)
        SetTool(TransformTool::Move);

    if (ePressed)
        SetTool(TransformTool::Rotate);

    if (rPressed)
        SetTool(TransformTool::Scale);
}

static Vector3 GetObjectCenter(const GameObject* object)
{
    if (!object)
        return {};

    if (pivotMode == PivotMode::Pivot)
        return object->position;

    return object->position;
}

static bool ProjectPoint(
    const Vector3& position,
    float& screenX,
    float& screenY,
    float& depth)
{
    return renderer.ProjectToScreen(
        position,
        screenX,
        screenY,
        depth
    );
}

static bool GetProjectedObjectBounds(
    const GameObject* object,
    float viewportWidth,
    float viewportHeight,
    float& minX,
    float& minY,
    float& maxX,
    float& maxY)
{
    if (!object)
        return false;

    minX = viewportWidth;
    minY = viewportHeight;
    maxX = 0.0f;
    maxY = 0.0f;

    bool foundPoint = false;

    for (int x = -1; x <= 1; x += 2)
    {
        for (int y = -1; y <= 1; y += 2)
        {
            for (int z = -1; z <= 1; z += 2)
            {
                Vector3 corner{
                    object->position.x + x * object->scale.x * 0.5f,
                    object->position.y + y * object->scale.y * 0.5f,
                    object->position.z + z * object->scale.z * 0.5f
                };

                float screenX;
                float screenY;
                float depth;

                if (!ProjectPoint(corner, screenX, screenY, depth))
                    continue;

                minX = std::min(minX, screenX);
                minY = std::min(minY, screenY);
                maxX = std::max(maxX, screenX);
                maxY = std::max(maxY, screenY);

                foundPoint = true;
            }
        }
    }

    return foundPoint;
}

static GameObject* PickObject(
    float mouseX,
    float mouseY,
    float viewportWidth,
    float viewportHeight)
{
    GameObject* closestObject = nullptr;
    float closestDepth = 1000000.0f;

    for (auto& objectPointer : scene.GetObjects())
    {
        GameObject* object = objectPointer.get();

        float minX;
        float minY;
        float maxX;
        float maxY;

        if (!GetProjectedObjectBounds(
            object,
            viewportWidth,
            viewportHeight,
            minX,
            minY,
            maxX,
            maxY))
        {
            continue;
        }

        if (mouseX < minX || mouseX > maxX ||
            mouseY < minY || mouseY > maxY)
        {
            continue;
        }

        float projectedX;
        float projectedY;
        float depth;

        if (!ProjectPoint(
            object->position,
            projectedX,
            projectedY,
            depth))
        {
            continue;
        }

        if (depth < closestDepth)
        {
            closestDepth = depth;
            closestObject = object;
        }
    }

    return closestObject;
}

static float DistanceToSegment(
    float px,
    float py,
    float ax,
    float ay,
    float bx,
    float by)
{
    float dx = bx - ax;
    float dy = by - ay;

    float lengthSquared = dx * dx + dy * dy;

    if (lengthSquared < 0.0001f)
    {
        float ex = px - ax;
        float ey = py - ay;

        return std::sqrt(ex * ex + ey * ey);
    }

    float t = ((px - ax) * dx + (py - ay) * dy) / lengthSquared;
    t = std::clamp(t, 0.0f, 1.0f);

    float closestX = ax + t * dx;
    float closestY = ay + t * dy;

    float ex = px - closestX;
    float ey = py - closestY;

    return std::sqrt(ex * ex + ey * ey);
}

static int HitTestGizmo(
    float mouseX,
    float mouseY,
    float gizmoSize)
{
    if (!selectedObject)
        return -1;

    Vector3 center = GetObjectCenter(selectedObject);

    float centerX;
    float centerY;
    float centerDepth;

    if (!ProjectPoint(center, centerX, centerY, centerDepth))
        return -1;

    const float axisLength = gizmoSize;

    Vector3 endpoints[3] = {
        {center.x + axisLength, center.y, center.z},
        {center.x, center.y + axisLength, center.z},
        {center.x, center.y, center.z + axisLength}
    };

    int bestAxis = -1;
    float bestDistance = 100000.0f;

    if (currentTool == TransformTool::Rotate)
    {
        const int segments = 96;
        const float radius = axisLength * 0.8f;

        for (int axis = 0; axis < 3; ++axis)
        {
            float previousX = 0.0f;
            float previousY = 0.0f;
            bool hasPrevious = false;

            for (int i = 0; i <= segments; ++i)
            {
                float angle =
                    6.28318530718f *
                    static_cast<float>(i) /
                    static_cast<float>(segments);

                Vector3 point = center;

                if (axis == 0)
                {
                    point.y += std::cos(angle) * radius;
                    point.z += std::sin(angle) * radius;
                }
                else if (axis == 1)
                {
                    point.x += std::cos(angle) * radius;
                    point.z += std::sin(angle) * radius;
                }
                else
                {
                    point.x += std::cos(angle) * radius;
                    point.y += std::sin(angle) * radius;
                }

                float screenX;
                float screenY;
                float depth;

                if (!ProjectPoint(point, screenX, screenY, depth))
                {
                    hasPrevious = false;
                    continue;
                }

                if (hasPrevious)
                {
                    float distance = DistanceToSegment(
                        mouseX,
                        mouseY,
                        previousX,
                        previousY,
                        screenX,
                        screenY
                    );

                    if (distance < bestDistance)
                    {
                        bestDistance = distance;
                        bestAxis = axis;
                    }
                }

                previousX = screenX;
                previousY = screenY;
                hasPrevious = true;
            }
        }

        return bestDistance < 12.0f ? bestAxis : -1;
    }

    for (int axis = 0; axis < 3; ++axis)
    {
        float endX;
        float endY;
        float endDepth;

        if (!ProjectPoint(
            endpoints[axis],
            endX,
            endY,
            endDepth))
        {
            continue;
        }

        float distance = DistanceToSegment(
            mouseX,
            mouseY,
            centerX,
            centerY,
            endX,
            endY
        );

        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestAxis = axis;
        }
    }

    return bestDistance < 12.0f ? bestAxis : -1;
}

static float GetMouseAngle(
    float mouseX,
    float mouseY,
    float centerX,
    float centerY)
{
    return std::atan2(
        mouseY - centerY,
        mouseX - centerX
    );
}

static void BeginGizmoDrag(
    int axis,
    float mouseX,
    float mouseY)
{
    if (!selectedObject || axis < 0 || axis > 2)
        return;

    draggingGizmo = true;
    draggingAxis = axis;

    dragStartPosition = selectedObject->position;
    dragStartRotation = selectedObject->rotation;
    dragStartScale = selectedObject->scale;

    dragStartMouseX = mouseX;
    dragStartMouseY = mouseY;

    dragAccumulatedAngle = 0.0f;

    float centerX;
    float centerY;
    float depth;

    Vector3 center = GetObjectCenter(selectedObject);

    if (ProjectPoint(center, centerX, centerY, depth))
    {
        dragStartAngle = GetMouseAngle(
            mouseX,
            mouseY,
            centerX,
            centerY
        );

        dragPreviousAngle = dragStartAngle;
    }
}

static void ApplyGizmoDrag(
    float mouseX,
    float mouseY,
    float gizmoSize)
{
    if (!selectedObject || !draggingGizmo || draggingAxis < 0)
        return;

    if (currentTool == TransformTool::Rotate)
    {
        float centerX;
        float centerY;
        float depth;

        Vector3 center = GetObjectCenter(selectedObject);

        if (!ProjectPoint(center, centerX, centerY, depth))
            return;

        float currentAngle = GetMouseAngle(
            mouseX,
            mouseY,
            centerX,
            centerY
        );

        float delta = currentAngle - dragPreviousAngle;

        if (delta > 3.14159265359f)
            delta -= 6.28318530718f;

        if (delta < -3.14159265359f)
            delta += 6.28318530718f;

        dragAccumulatedAngle += delta;
        dragPreviousAngle = currentAngle;

        float degrees = dragAccumulatedAngle * 57.2957795131f;

        if (draggingAxis == 0)
            selectedObject->rotation.x = dragStartRotation.x + degrees;
        else if (draggingAxis == 1)
            selectedObject->rotation.y = dragStartRotation.y + degrees;
        else
            selectedObject->rotation.z = dragStartRotation.z + degrees;

        return;
    }

    Vector3 center = GetObjectCenter(selectedObject);

    Vector3 endpoint = center;

    if (draggingAxis == 0)
        endpoint.x += gizmoSize;
    else if (draggingAxis == 1)
        endpoint.y += gizmoSize;
    else
        endpoint.z += gizmoSize;

    float centerX;
    float centerY;
    float centerDepth;

    float endX;
    float endY;
    float endDepth;

    if (!ProjectPoint(center, centerX, centerY, centerDepth))
        return;

    if (!ProjectPoint(endpoint, endX, endY, endDepth))
        return;

    float axisX = endX - centerX;
    float axisY = endY - centerY;

    float screenLengthSquared = axisX * axisX + axisY * axisY;

    if (screenLengthSquared < 1.0f)
        return;

    float mouseDeltaX =
        mouseX - static_cast<float>(dragStartMouseX);

    float mouseDeltaY =
        mouseY - static_cast<float>(dragStartMouseY);

    float projectedDelta =
        (mouseDeltaX * axisX + mouseDeltaY * axisY) /
        screenLengthSquared;

    float worldDelta = projectedDelta * gizmoSize;

    if (currentTool == TransformTool::Move)
    {
        selectedObject->position = dragStartPosition;

        if (draggingAxis == 0)
            selectedObject->position.x += worldDelta;
        else if (draggingAxis == 1)
            selectedObject->position.y += worldDelta;
        else
            selectedObject->position.z += worldDelta;
    }
    else if (currentTool == TransformTool::Scale)
    {
        selectedObject->scale = dragStartScale;

        float factor = 1.0f + worldDelta;

        if (draggingAxis == 0)
            selectedObject->scale.x = std::max(0.05f, dragStartScale.x * factor);
        else if (draggingAxis == 1)
            selectedObject->scale.y = std::max(0.05f, dragStartScale.y * factor);
        else
            selectedObject->scale.z = std::max(0.05f, dragStartScale.z * factor);
    }
}

static void DrawToolbar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Object"))
            {
                GameObject* object = scene.CreateObject("GameObject");

                if (object)
                    selectedObject = object;
            }

            if (ImGui::MenuItem("Exit"))
                glfwSetWindowShouldClose(
                    glfwGetCurrentContext(),
                    GLFW_TRUE
                );

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GameObject"))
        {
            if (ImGui::MenuItem("Create Cube"))
            {
                GameObject* object = scene.CreateObject("Cube");

                if (object)
                    selectedObject = object;
            }

            if (ImGui::MenuItem("Delete Selected", nullptr, false,
                selectedObject != nullptr))
            {
                int selectedId = selectedObject->id;

                scene.DeleteObject(selectedId);
                selectedObject = nullptr;
                draggingGizmo = false;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Move", "W"))
                SetTool(TransformTool::Move);

            if (ImGui::MenuItem("Rotate", "E"))
                SetTool(TransformTool::Rotate);

            if (ImGui::MenuItem("Scale", "R"))
                SetTool(TransformTool::Scale);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Toolbar", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::Button("Move (W)"))
        SetTool(TransformTool::Move);

    ImGui::SameLine();

    if (ImGui::Button("Rotate (E)"))
        SetTool(TransformTool::Rotate);

    ImGui::SameLine();

    if (ImGui::Button("Scale (R)"))
        SetTool(TransformTool::Scale);

    ImGui::SameLine();

    if (ImGui::Button("Pivot"))
        pivotMode = PivotMode::Pivot;

    ImGui::SameLine();

    if (ImGui::Button("Center"))
        pivotMode = PivotMode::Center;

    ImGui::End();
}

static void DrawHierarchy()
{
    ImGui::Begin("Hierarchy");

    if (ImGui::Button("+ Create GameObject"))
    {
        GameObject* object = scene.CreateObject("GameObject");

        if (object)
            selectedObject = object;
    }

    ImGui::Separator();

    for (auto& objectPointer : scene.GetObjects())
    {
        GameObject* object = objectPointer.get();

        char label[256];

        std::snprintf(
            label,
            sizeof(label),
            "%s##%d",
            object->name.c_str(),
            object->id
        );

        if (ImGui::Selectable(
            label,
            selectedObject == object))
        {
            selectedObject = object;
            draggingGizmo = false;
        }
    }

    ImGui::End();
}

static void DrawVector3Control(
    const char* label,
    Vector3& value,
    float resetValue)
{
    ImGui::PushID(label);

    ImGui::TextUnformatted(label);

    ImGui::PushItemWidth(-1);

    float values[3] = {
        value.x,
        value.y,
        value.z
    };

    if (ImGui::DragFloat3("##value", values, 0.1f))
    {
        value.x = values[0];
        value.y = values[1];
        value.z = values[2];
    }

    if (ImGui::Button("Reset"))
    {
        value.x = resetValue;
        value.y = resetValue;
        value.z = resetValue;
    }

    ImGui::PopItemWidth();
    ImGui::PopID();
}

static void DrawInspector()
{
    ImGui::Begin("Inspector");

    if (!selectedObject)
    {
        ImGui::TextDisabled("No GameObject selected.");
        ImGui::End();
        return;
    }

    char nameBuffer[256] = {};

    std::snprintf(
        nameBuffer,
        sizeof(nameBuffer),
        "%s",
        selectedObject->name.c_str()
    );

    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
        selectedObject->name = nameBuffer;

    ImGui::Text("ID: %d", selectedObject->id);

    ImGui::Separator();

    DrawVector3Control(
        "Position",
        selectedObject->position,
        0.0f
    );

    ImGui::Separator();

    DrawVector3Control(
        "Rotation",
        selectedObject->rotation,
        0.0f
    );

    ImGui::Separator();

    DrawVector3Control(
        "Scale",
        selectedObject->scale,
        1.0f
    );

    ImGui::Separator();

    if (ImGui::Button("Delete GameObject"))
    {
        int selectedId = selectedObject->id;

        scene.DeleteObject(selectedId);
        selectedObject = nullptr;
        draggingGizmo = false;
    }

    ImGui::End();
}

static void DrawSceneViewport(GLFWwindow* window)
{
    ImGui::Begin("Scene");

    ImGui::TextDisabled(
        "Right mouse + WASD: move camera | R: up | Q: down"
    );

    ImGui::TextDisabled(
        "W: Move | E: Rotate | R: Scale outside camera mode"
    );

    ImVec2 available = ImGui::GetContentRegionAvail();

    if (available.x < 1.0f)
        available.x = 1.0f;

    if (available.y < 1.0f)
        available.y = 1.0f;

    ImVec2 viewportStart = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton(
        "SceneViewport",
        available,
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonRight
    );

    sceneViewportHovered = ImGui::IsItemHovered();
    sceneViewportFocused = ImGui::IsItemFocused();

    float viewportWidth = available.x;
    float viewportHeight = available.y;

    int framebufferWidth = static_cast<int>(viewportWidth);
    int framebufferHeight = static_cast<int>(viewportHeight);

    renderer.BeginScene(
        framebufferWidth,
        framebufferHeight,
        cameraPosition,
        cameraYaw,
        cameraPitch
    );

    for (auto& objectPointer : scene.GetObjects())
    {
        GameObject* object = objectPointer.get();

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

    const float gizmoSize = 1.5f;

    if (selectedObject)
    {
        renderer.DrawGizmo(
            GetObjectCenter(selectedObject),
            static_cast<int>(currentTool),
            gizmoSize
        );
    }

    unsigned int sceneTexture = renderer.GetSceneTexture();

    ImGui::SetCursorScreenPos(viewportStart);

    ImGui::Image(
        (ImTextureID)(intptr_t)sceneTexture,
        available,
        ImVec2(0, 1),
        ImVec2(1, 0)
    );

    ImGui::SetCursorScreenPos(viewportStart);

    ImGui::InvisibleButton(
        "SceneMouseInput",
        available,
        ImGuiButtonFlags_MouseButtonLeft
    );

    bool mouseInsideViewport =
        ImGui::IsItemHovered() || sceneViewportHovered;

    ImVec2 mousePosition = ImGui::GetMousePos();

    float localMouseX = mousePosition.x - viewportStart.x;
    float localMouseY = mousePosition.y - viewportStart.y;

    if (mouseInsideViewport &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        int axis = HitTestGizmo(
            localMouseX,
            localMouseY,
            gizmoSize
        );

        if (selectedObject && axis >= 0)
        {
            BeginGizmoDrag(
                axis,
                localMouseX,
                localMouseY
            );
        }
        else
        {
            selectedObject = PickObject(
                localMouseX,
                localMouseY,
                viewportWidth,
                viewportHeight
            );

            draggingGizmo = false;
            draggingAxis = -1;
        }
    }

    if (draggingGizmo &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        ApplyGizmoDrag(
            localMouseX,
            localMouseY,
            gizmoSize
        );
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        draggingGizmo = false;
        draggingAxis = -1;
    }

    if (mouseInsideViewport &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        glfwSetInputMode(
            window,
            GLFW_CURSOR,
            GLFW_CURSOR_DISABLED
        );
        firstMouse = true;
    }

    if (!IsRightMouseDown(window))
    {
        glfwSetInputMode(
            window,
            GLFW_CURSOR,
            GLFW_CURSOR_NORMAL
        );
    }

    ImGui::End();
}

static void DrawConsole()
{
    ImGui::Begin("Console");

    ImGui::Text("mestEngine");
    ImGui::Separator();

    ImGui::Text("Renderer: OpenGL");
    ImGui::Text("Scene objects: %d",
        static_cast<int>(scene.GetObjects().size()));

    ImGui::Text(
        "Current tool: %s",
        currentTool == TransformTool::Move
            ? "Move"
            : currentTool == TransformTool::Rotate
                ? "Rotate"
                : "Scale"
    );

    ImGui::Text(
        "Camera: %.2f, %.2f, %.2f",
        cameraPosition.x,
        cameraPosition.y,
        cameraPosition.z
    );

    ImGui::End();
}

int main()
{
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        1280,
        800,
        "mestEngine",
        nullptr,
        nullptr
    );

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

glGetError();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    if (!renderer.Initialize())
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    scene.CreateObject("Cube");

    float previousTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - previousTime;
        previousTime = currentTime;

        deltaTime = std::min(deltaTime, 0.05f);

        HandleCamera(window, deltaTime);

        int displayWidth;
        int displayHeight;

        glfwGetFramebufferSize(
            window,
            &displayWidth,
            &displayHeight
        );

        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        HandleShortcuts(window);

        DrawToolbar();
        DrawHierarchy();
        DrawInspector();
        DrawSceneViewport(window);
        DrawConsole();

        ImGui::Render();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
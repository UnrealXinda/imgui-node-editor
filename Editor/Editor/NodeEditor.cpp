#include "NodeEditorInternal.h"
#include <string>
#include <fstream>

namespace ed = ax::NodeEditor;


extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(const char* string);

static void LogV(const char* fmt, va_list args)
{
    const int buffer_size = 1024;
    static char buffer[1024];

    int w = vsnprintf(buffer, buffer_size - 1, fmt, args);
    buffer[buffer_size - 1] = 0;

    ImGui::LogText("\nNode Editor: %s", buffer);

    OutputDebugStringA("NodeEditor: ");
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
}

void ed::Log(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogV(fmt, args);
    va_end(args);
}


namespace ax {
namespace NodeEditor {
Context* s_Editor = nullptr;
} // namespace node_editor
} // namespace ax

ed::Context::Context():
    Nodes(),
    CurrentNode(nullptr),
    CurrentNodeIsNew(false),
    CurrentNodeStage(NodeStage::Invalid),
    ActiveNode(nullptr),
    DragOffset(),
    IsInitialized(false),
    Settings()
{
}

ed::Context::~Context()
{
    if (IsInitialized)
        SaveSettings();

    for (auto node : Nodes)
        delete node;

    Nodes.clear();
}


ed::Node* ed::Context::FindNode(int id)
{
    for (auto node : s_Editor->Nodes)
        if (node->ID == id)
            return node;

    return nullptr;
}

ed::Node* ed::Context::CreateNode(int id)
{
    assert(nullptr == FindNode(id));
    s_Editor->Nodes.push_back(new Node(id));
    auto node = s_Editor->Nodes.back();

    auto settings = FindNodeSettings(id);
    if (!settings)
    {
        settings = AddNodeSettings(id);
        node->Bounds.location = to_point(ImGui::GetCursorScreenPos());
    }
    else
        node->Bounds.location = settings->Location;

    return node;
}

void ed::Context::DestroyNode(Node* node)
{
    if (!node) return;
    auto& nodes = s_Editor->Nodes;
    auto nodeIt = std::find(nodes.begin(), nodes.end(), node);
    assert(nodeIt != nodes.end());
    nodes.erase(nodeIt);
    delete node;
}

void ed::Context::SetCurrentNode(Node* node, bool isNew/* = false*/)
{
    CurrentNode      = node;
    CurrentNodeIsNew = isNew;
}

void ed::Context::SetActiveNode(Node* node)
{
    ActiveNode = node;
}

bool ed::Context::SetNodeStage(NodeStage stage)
{
    if (stage == CurrentNodeStage)
        return false;

    auto oldStage = CurrentNodeStage;
    CurrentNodeStage = stage;

    ImVec2 cursor;
    switch (oldStage)
    {
        case NodeStage::Begin:
            break;

        case NodeStage::Header:
            ImGui::EndHorizontal();
            ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImColor(255, 0, 0, 32));

            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 2),
                ImVec2(ImGui::GetItemRectMax().x, ImGui::GetItemRectMax().y + 2), ImColor(255, 255, 255));

            ImGui::Spring(0, 8);

            break;

        case NodeStage::Content:
            break;

        case NodeStage::Input:
            ImGui::Spring(1, 0);
            ImGui::EndVertical();
            //ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImColor(0, 255, 0, 32));
            break;

        case NodeStage::Output:
            ImGui::Spring(1,0);
            ImGui::EndVertical();
            //ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImColor(0, 255, 0, 32));
            break;

        case NodeStage::End:
            break;
    }

    switch (stage)
    {
        case NodeStage::Begin:
            ImGui::BeginVertical(CurrentNode->ID);
            break;

        case NodeStage::Header:
            ImGui::BeginHorizontal("header");
            break;

        case NodeStage::Content:
            ImGui::BeginHorizontal("content");
            break;

        case NodeStage::Input:
            ImGui::BeginVertical("input", ImVec2(0,0), 0.0f);
            break;

        case NodeStage::Output:
            if (oldStage == NodeStage::Input)
                ImGui::Spring(1);
            else
                ImGui::Spring(1,0);
            ImGui::BeginVertical("output", ImVec2(0, 0), 1.0f);
            break;

        case NodeStage::End:
            ImGui::EndHorizontal();
            //ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImColor(0, 0, 255, 32));
            //ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImColor(0, 0, 255, 64));

            ImGui::EndVertical();
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin() - ImVec2(2,2), ImGui::GetItemRectMax() + ImVec2(2, 2), ImColor(255, 255, 255));
            break;
    }

    return true;
}

ed::NodeSettings* ed::Context::FindNodeSettings(int id)
{
    for (auto& nodeSettings : Settings.Nodes)
        if (nodeSettings.ID == id)
            return &nodeSettings;

    return nullptr;
}

ed::NodeSettings* ed::Context::AddNodeSettings(int id)
{
    Settings.Nodes.push_back(NodeSettings(id));
    return &Settings.Nodes.back();
}

void ed::Context::LoadSettings()
{
    namespace json = picojson;

    if (Settings.Path.empty())
        return;

    std::ifstream settingsFile(Settings.Path);
    if (!settingsFile)
        return;

    json::value settingsValue;
    auto error = json::parse(settingsValue, settingsFile);
    if (!error.empty() || settingsValue.is<json::null>())
        return;

    if (!settingsValue.is<json::object>())
        return;

    auto& settingsObject = settingsValue.get<json::object>();
    auto& nodesValue     = settingsValue.get("nodes");

    if (nodesValue.is<json::object>())
    {
        for (auto& node : nodesValue.get<json::object>())
        {
            auto id       = static_cast<int>(strtoll(node.first.c_str(), nullptr, 10));
            auto settings = FindNodeSettings(id);
            if (!settings)
                settings = AddNodeSettings(id);

            auto& nodeData = node.second;

            auto locationValue = nodeData.get("location");
            if (locationValue.is<json::object>())
            {
                auto xValue = locationValue.get("x");
                auto yValue = locationValue.get("y");

                if (xValue.is<double>() && yValue.is<double>())
                {
                    settings->Location.x = static_cast<int>(xValue.get<double>());
                    settings->Location.y = static_cast<int>(yValue.get<double>());
                }
            }
        }
    }
}

void ed::Context::SaveSettings()
{
    namespace json = picojson;

    if (Settings.Path.empty())
        return;

    std::ofstream settingsFile(Settings.Path);
    if (!settingsFile)
        return;

    // update settings data
    for (auto& node : Nodes)
    {
        auto settings = FindNodeSettings(node->ID);
        settings->Location = node->Bounds.location;
    }

    json::object nodes;
    for (auto& node : Settings.Nodes)
    {
        json::object locationData;
        locationData["x"] = json::value(static_cast<double>(node.Location.x));
        locationData["y"] = json::value(static_cast<double>(node.Location.y));

        json::object nodeData;
        nodeData["location"] = json::value(std::move(locationData));

        nodes[std::to_string(node.ID)] = json::value(std::move(nodeData));
    }

    json::object settings;
    settings["nodes"] = json::value(std::move(nodes));

    json::value settingsValue(std::move(settings));

    settingsFile << settingsValue.serialize(true);
}

void ed::Context::MarkSettingsDirty()
{
    Settings.Dirty = true;
}

ed::Context* ed::CreateEditor()
{
    return new Context();
}

void ed::DestroyEditor(Context* ctx)
{
    if (GetCurrentEditor() == ctx)
        SetCurrentEditor(nullptr);

    delete ctx;
}

void ed::SetCurrentEditor(Context* ctx)
{
    s_Editor = ctx;
}

ed::Context* ed::GetCurrentEditor()
{
    return s_Editor;
}

void ed::Begin(const char* id)
{
    if (!s_Editor->IsInitialized)
    {
        s_Editor->LoadSettings();
        s_Editor->IsInitialized = true;
    }

    //ImGui::LogToClipboard();
    //Log("---- begin ----");

    ImGui::BeginChild(id, ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
}

void ed::End()
{
    ImGui::EndChild();

    if (s_Editor->Settings.Dirty)
    {
        s_Editor->Settings.Dirty = false;
        s_Editor->SaveSettings();
    }

    //Log("---- end ----");
    //ImGui::LogFinish();
}

void ed::BeginNode(int id)
{
    assert(nullptr == s_Editor->CurrentNode);

    auto isNewNode = false;

    auto node = s_Editor->FindNode(id);
    if (!node)
    {
        node = s_Editor->CreateNode(id);
        s_Editor->CurrentNodeIsNew = true;

        // Make new node and everything under it invisible
        // for first time it is around to determine initial
        // dimensions without showing a mess to user.
        // Next iterations use cached layout.
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.0f);

        isNewNode = true;
    }

    ImGui::PushID(id);

    s_Editor->SetCurrentNode(node, isNewNode);

    // Position node on screen
    if (s_Editor->ActiveNode == node)
        ImGui::SetCursorScreenPos(to_imvec(node->Bounds.location) + s_Editor->DragOffset); // drag, find a better way
    else
        ImGui::SetCursorScreenPos(to_imvec(node->Bounds.location));

    s_Editor->SetNodeStage(NodeStage::Begin);
}

void ed::EndNode()
{
    assert(nullptr != s_Editor->CurrentNode);

    s_Editor->SetNodeStage(NodeStage::End);

    auto nodeRect = get_item_bounds();

    if (s_Editor->CurrentNode->Bounds != nodeRect)
    {
        s_Editor->MarkSettingsDirty();

        // TODO: update layout
        //ImGui::Text((std::string("Changed: ") + std::to_string(s_Editor->CurrentNode->ID)).c_str());
    }

    ImGui::PopID();

    if (s_Editor->CurrentNodeIsNew)
        ImGui::PopStyleVar();

    ImGui::SetCursorScreenPos(to_imvec(nodeRect.location));
    ImGui::InvisibleButton(std::to_string(s_Editor->CurrentNode->ID).c_str(), to_imvec(nodeRect.size));

    if (!s_Editor->CurrentNodeIsNew)
    {
        if (ImGui::IsItemActive())
        {
            s_Editor->SetActiveNode(s_Editor->CurrentNode);
            s_Editor->DragOffset = ImGui::GetMouseDragDelta(0, 0.0f);
        }
        else if (s_Editor->ActiveNode == s_Editor->CurrentNode)
        {
            s_Editor->ActiveNode->Bounds.location += to_point(s_Editor->DragOffset);
            s_Editor->SetActiveNode(nullptr);
        }
    }

    s_Editor->SetCurrentNode(nullptr);

    s_Editor->SetNodeStage(NodeStage::Invalid);
}

void ed::BeginHeader()
{
    assert(nullptr != s_Editor->CurrentNode);

    s_Editor->SetNodeStage(NodeStage::Header);
}

void ed::EndHeader()
{
    assert(nullptr != s_Editor->CurrentNode);

    s_Editor->SetNodeStage(NodeStage::Content);
}

void ed::BeginInput(int id)
{
    assert(nullptr != s_Editor->CurrentNode);

    if (s_Editor->CurrentNodeStage == NodeStage::Begin)
        s_Editor->SetNodeStage(NodeStage::Content);

    s_Editor->SetNodeStage(NodeStage::Input);

}

void ed::EndInput()
{
    ImGui::Spring(0);
}

void ed::BeginOutput(int id)
{
    assert(nullptr != s_Editor->CurrentNode);

    if (s_Editor->CurrentNodeStage == NodeStage::Begin)
        s_Editor->SetNodeStage(NodeStage::Content);

    if (s_Editor->CurrentNodeStage == NodeStage::Begin)
        s_Editor->SetNodeStage(NodeStage::Input);

    //ImGui::SameLine();
    //ed::Spring();
    //ImGui::SameLine();

    s_Editor->SetNodeStage(NodeStage::Output);
}

void ed::EndOutput()
{
    ImGui::Spring(0);
}

void ed::Link(int id, int startNodeId, int endNodeId, const ImVec4& color/* = ImVec4(1, 1, 1, 1)*/)
{
}




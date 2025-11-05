// Machine Summary Block
// {"file":"engine/platform/desktop/panels/scene_tree_panel.cpp","purpose":"Renders the ImGui scene tree panel for the desktop editor.","exports":["panels::RenderSceneTree"],"depends_on":["panels/scene_tree_panel.h","imgui.h"],"notes":["desktop_imgui_panel"]}
// Human Summary
// Desktop-oriented ImGui scene tree that wires selection and command dispatch into the UI bridge.
#include "panels/scene_tree_panel.h"

#include "imgui.h"

#include <unordered_set>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>

namespace panels {

void RenderSceneTree(const UIState& state, const std::function<void(const UICommandData&)>& onCommand)
{
    ImGuiIO& io = ImGui::GetIO();
    const float leftW = 300.0f;
    ImGui::SetNextWindowPos(ImVec2(16.0f, 45.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(leftW, io.DisplaySize.y - 45.0f - 16.0f), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    if (!ImGui::Begin("Scene", nullptr, flags)) { ImGui::End(); return; }

    // Search/filter
    static char filterBuf[128] = "";
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##scene_filter", "filter (name substring)", filterBuf, sizeof(filterBuf));
    const bool hasFilter = filterBuf[0] != '\0';

    ImGui::Separator();

    // Data model
    const int N = (int)state.objectNames.size();
    std::vector<int> parent(N, -1);
    if ((int)state.objectParentIndex.size() == N) parent = state.objectParentIndex;

    // Children table
    std::vector<std::vector<int>> children(N);
    for (int i = 0; i < N; ++i) {
        int p = parent[i];
        if (p >= 0 && p < N && p != i) children[p].push_back(i);
    }
    // Roots
    std::vector<int> roots; roots.reserve(N);
    for (int i = 0; i < N; ++i) {
        if (parent[i] < 0 || parent[i] >= N || parent[i] == i) roots.push_back(i);
    }

    // Persist expand state across frames
    static std::unordered_set<int> expanded;
    auto isExpanded = [&](int id) -> bool { return expanded.count(id) != 0; };
    auto toggleExpanded = [&](int id) { if (isExpanded(id)) expanded.erase(id); else expanded.insert(id); };

    // Box-drawing with ASCII fallback
    const char* VERT = u8"???   ";
    const char* TEE  = u8"????????? ";
    const char* ELB  = u8"????????? ";
    const char* SPACE= "    ";
    const char* EXPANDED = u8"???";
    const char* COLLAPSED = u8"???";
    bool unicode = true;
    if (!ImGui::GetFont()->FindGlyphNoFallback((ImWchar)0x2502)) unicode = false; // ???
    if (!unicode) { 
        VERT = "|   "; 
        TEE = "|-- "; 
        ELB = "`-- "; 
        EXPANDED = "-";
        COLLAPSED = "+";
    }

    // Utils
    auto matchesFilter = [&](const std::string& s)->bool {
        if (!hasFilter) return true;
        std::string a = s, b = std::string(filterBuf);
        std::transform(a.begin(), a.end(), a.begin(), ::tolower);
        std::transform(b.begin(), b.end(), b.begin(), ::tolower);
        return a.find(b) != std::string::npos;
    };

    struct Row { int id; std::string prefix; bool lastSibling; int depth; };
    std::vector<Row> rows; rows.reserve(N);

    std::function<void(int,const std::vector<bool>&,int)> pushRow =
        [&](int id, const std::vector<bool>& drawVert, int depth)
    {
        std::string prefix;
        for (int d = 0; d < depth; ++d) prefix += drawVert[d] ? VERT : SPACE;
        rows.push_back({id, prefix, false, depth});
    };

    std::function<void(int,std::vector<bool>&,int)> expandDFS =
        [&](int nid, std::vector<bool>& vmask, int depth)
    {
        if (!isExpanded(nid)) return;
        const auto& ch = children[nid];
        for (int c = 0; c < (int)ch.size(); ++c) {
            int cid = ch[c];
            std::vector<bool> nextMask = vmask;
            if (depth >= (int)nextMask.size()) nextMask.resize(depth + 1, false);
            nextMask[depth] = (c != (int)ch.size() - 1);
            pushRow(cid, nextMask, depth + 1);
            expandDFS(cid, nextMask, depth + 1);
        }
    };

    // Seed rows with roots and expanded descendants
    for (int r = 0; r < (int)roots.size(); ++r) {
        int root = roots[r];
        std::vector<bool> mask; // depth 0
        pushRow(root, mask, 0);
        expandDFS(root, mask, 0);
    }
    // Mark lastSibling
    for (size_t i = 0; i < rows.size(); ++i) {
        bool last = true;
        if (i + 1 < rows.size()) last = rows[i + 1].depth <= rows[i].depth;
        rows[i].lastSibling = last;
    }

    const bool windowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    if (ImGui::BeginChild("##scene_tree_scroll", ImVec2(0, 0), true, ImGuiWindowFlags_NavFlattened)) {
        static int renamingId = -1;
        static char renameBuf[256] = "";

        for (size_t i = 0; i < rows.size(); ++i) {
            const Row& row = rows[i];
            int id = row.id;
            const bool hasChildren = (id >= 0 && id < N) && !children[id].empty();
            const bool open = isExpanded(id);
            const bool selected = (id == state.selectedObjectIndex);
            const std::string& nm = state.objectNames[id];

            // Filter: show branches if any descendant matches
            if (!matchesFilter(nm)) {
                bool anyChildMatches = false;
                std::function<void(int)> checkKids = [&](int nid){
                    for (int c : children[nid]) {
                        if (matchesFilter(state.objectNames[c])) { anyChildMatches = true; return; }
                        checkKids(c);
                        if (anyChildMatches) return;
                    }
                };
                if (hasChildren) checkKids(id);
                if (!anyChildMatches) continue;
            }

            ImGui::PushID(id);

            std::string branch = row.prefix;
            branch += (row.depth == 0) ? "" : (row.lastSibling ? ELB : TEE);
            ImGui::TextUnformatted(branch.c_str());
            ImGui::SameLine(0.0f, 0.0f);

            if (hasChildren) {
                if (ImGui::SmallButton(open ? EXPANDED : COLLAPSED)) { toggleExpanded(id); }
                ImGui::SameLine();
            } else {
                ImGui::Dummy(ImVec2(ImGui::GetFrameHeight()*0.75f, 0));
                ImGui::SameLine();
            }

            if (renamingId == id) {
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("##rename", renameBuf, sizeof(renameBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    UICommandData c; c.command = UICommand::RenameObject; c.intParam = id; c.stringParam = std::string(renameBuf);
                    if (onCommand) onCommand(c);
                    renamingId = -1;
                }
                if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0)) renamingId = -1;
            } else {
                if (ImGui::Selectable(nm.c_str(), selected)) {
                    UICommandData c; c.command = UICommand::SelectObject; c.intParam = id; if (onCommand) onCommand(c);
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && hasChildren) {
                    toggleExpanded(id);
                }
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Select")) { UICommandData c; c.command = UICommand::SelectObject; c.intParam = id; if (onCommand) onCommand(c); }
                    if (ImGui::MenuItem("Rename", "F2")) { renamingId = id; std::snprintf(renameBuf, sizeof(renameBuf), "%s", nm.c_str()); }
                    if (ImGui::MenuItem("Duplicate", "Ctrl+D")) { UICommandData c; c.command = UICommand::DuplicateObject; c.intParam = id; if (onCommand) onCommand(c); }
                    if (ImGui::MenuItem("Delete", "Del")) { UICommandData c; c.command = UICommand::RemoveObject; c.intParam = id; if (onCommand) onCommand(c); }
                    ImGui::Separator();
                    if (ImGui::MenuItem(open ? "Collapse" : "Expand")) { toggleExpanded(id); }
                    ImGui::EndPopup();
                }
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("SCENE_NODE_ID", &id, sizeof(int));
                ImGui::Text("Move '%s'", nm.c_str());
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("SCENE_NODE_ID")) {
                    int dragged = *(const int*)p->Data;
                    if (dragged != id) {
                        UICommandData c; 
                        c.command = UICommand::ReparentObject;
                        c.intParam = dragged;   // child
                        
                        // Check if dragged object is already a child of target (toggle parenting)
                        int draggedParent = (dragged >= 0 && dragged < (int)parent.size()) ? parent[dragged] : -1;
                        if (draggedParent == id) {
                            // Dragging child to existing parent -> unparent (move to root)
                            c.intParam2 = -1;
                        } else {
                            // Normal reparenting
                            c.intParam2 = id;       // new parent
                        }
                        
                        if (onCommand) onCommand(c);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopID();
        }

        // Keyboard shortcuts scoped to window
        if (windowFocused) {
            // Delete selected object
            if (ImGui::Shortcut(ImGuiKey_Delete, ImGuiInputFlags_Repeat)) {
                if (state.selectedObjectIndex >= 0 && state.selectedObjectIndex < N) {
                    UICommandData c; c.command = UICommand::RemoveObject; c.intParam = state.selectedObjectIndex; if (onCommand) onCommand(c);
                }
            }
            // Duplicate selected (Ctrl+D)
            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D)) {
                if (state.selectedObjectIndex >= 0 && state.selectedObjectIndex < N) {
                    UICommandData c; c.command = UICommand::DuplicateObject; c.intParam = state.selectedObjectIndex; if (onCommand) onCommand(c);
                }
            }
            // Rename (F2)
            if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
                int sel = state.selectedObjectIndex; if (sel >= 0 && sel < N) { std::snprintf(renameBuf, sizeof(renameBuf), "%s", state.objectNames[sel].c_str()); renamingId = sel; }
            }
            // Expand/Collapse with arrows
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
                int sel = state.selectedObjectIndex; if (sel >= 0 && sel < N && !children[sel].empty()) expanded.insert(sel);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
                int sel = state.selectedObjectIndex; if (sel >= 0 && sel < N) expanded.erase(sel);
            }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

} // namespace panels

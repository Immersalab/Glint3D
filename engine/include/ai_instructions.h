#pragma once
#include <string>
#include <sstream>

// Centralized AI instruction builders for NL->JSON and planning.
// Header-only (inline) so it works with both CMake and VS projects
// without needing to add an extra .cpp to every project file.

inline std::string build_instructions() {
    return std::string(R"(You convert natural language into a strict JSON command batch for a 3D app.
Output ONLY JSON. No prose, no markdown.
Schema: an object or array of objects with fields:
{ "op": one of [load_model, duplicate, add_light], ... }
load_model: { op, path, name?, transform?{ position:[x,y,z], scale:[x,y,z], rotation:[x,y,z] } }
duplicate:  { op, source, name?, transform?{ position:[dx,dy,dz], scale? } }
add_light:  { op, type: 'point'|'directional', position?, direction?, color?, intensity? }
Vectors are arrays of 3 numbers.
Examples:
User: load cow.obj and move it right 2
[{"op":"load_model","path":"cow.obj","name":"Cow1","transform":{"position":[2,0,0]}}]
User: duplicate Cow1 left 1 and add a light above
[{"op":"duplicate","source":"Cow1","name":"Cow2","transform":{"position":[-1,0,0]}},{"op":"add_light","type":"point","position":[0,5,0]}]
)");
}

inline std::string build_planner_instructions() {
    return std::string(R"(You are a scene assistant for a 3D app.
Given USER instruction and SCENE JSON, output a plan as imperative commands, one per line.
No explanations, no markdown, only commands.
Use object names from the scene where helpful.
Supported commands (free-form, natural but structured):
- place <name> [in front of me <d>] | [at x y z] [scale sx sy sz]
- place <count> <name> objects [scale sx sy sz | long | flat | tall | wide] [arrange into three walls and one floor]
- add light [at x y z] [color r g b] [intensity v]
- create material <name> [color r g b] [specular r g b] [ambient r g b] [shininess s] [roughness r] [metallic m]
- assign material <mat> to <object>
- fullscreen
Examples:
USER: put the cow 3 meters ahead and add a soft light above
place cow in front of me 3
add light at 0 5 0 intensity 0.6
USER: make a wood material and apply it to Cow1
create material wood color 0.6 0.4 0.2 roughness 0.8
assign material wood to Cow1
)");
}

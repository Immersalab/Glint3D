// Machine Summary Block
// {"file":"apps/qem_simplifier/include/glint_qem_tool/backend_contracts.h","purpose":"Defines ABI-conscious backend metadata and capability structs intended to inform a future plugin contract.","exports":["ApiVersion","BackendKind","CapabilityFlags","BackendInfo"],"depends_on":["<cstdint>"],"notes":["pod_friendly_shapes","future_plugin_abi_seed"]}
// Human Summary
// POD-style metadata/contracts used to keep the in-process backend interfaces aligned with a future runtime plugin ABI.

#pragma once

#include <cstdint>

namespace glint_qem_tool {

struct ApiVersion {
    std::uint16_t major = 0;
    std::uint16_t minor = 1;
    std::uint16_t patch = 0;
};

enum class BackendKind : std::uint32_t {
    kSimplifier = 1,
    kRenderer = 2
};

enum CapabilityFlags : std::uint64_t {
    kCapabilityNone = 0,
    kCapabilityDeterministic = 1ull << 0,
    kCapabilityCollapseTrace = 1ull << 1,
    kCapabilityDryRun = 1ull << 2,
    kCapabilityExternalProcess = 1ull << 3,
    kCapabilityRenderPreview = 1ull << 4
};

struct BackendInfo {
    ApiVersion api_version{};
    BackendKind kind = BackendKind::kSimplifier;
    std::uint64_t capability_flags = kCapabilityNone;
    const char* backend_id = "";
    const char* display_name = "";
    const char* implementation_version = "";
};

inline bool HasCapability(const BackendInfo& info, std::uint64_t flag) {
    return (info.capability_flags & flag) != 0;
}

} // namespace glint_qem_tool

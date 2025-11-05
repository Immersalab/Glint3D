// Machine Summary Block
// {"file":"tools/user_paths_probe.cpp","purpose":"CLI probe to exercise user path helpers under different modes.","depends_on":["user_paths.h","<filesystem>","<iostream>","<fstream>"],"notes":["outputs_JSON_for_automation","invokes_core_library_without_UI"]}
// Human Summary
// Diagnostic utility that validates user path resolution and portable mode behavior.
#include "user_paths.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct DirSnapshot {
    std::string kind;
    fs::path path;
    bool exists = false;
    bool startsWithRuntime = false;
};

struct FileSnapshot {
    std::string name;
    fs::path path;
    bool exists = false;
    fs::path parent;
};

#if defined(_WIN32)
void setEnv(const std::string& key, const std::string& value)
{
    if (value.empty()) {
        _putenv_s(key.c_str(), "");
    } else {
        _putenv_s(key.c_str(), value.c_str());
    }
}
#else
void setEnv(const std::string& key, const std::string& value)
{
    if (value.empty()) {
        unsetenv(key.c_str());
    } else {
        setenv(key.c_str(), value.c_str(), 1);
    }
}
#endif

void clearPortableArtifacts()
{
    fs::path runtimeRoot = fs::current_path() / "runtime";
    fs::remove(runtimeRoot / ".portable");
    if (fs::exists(runtimeRoot)) {
        std::error_code ec;
        fs::remove_all(runtimeRoot, ec);
    }
}

DirSnapshot makeDirSnapshot(const std::string& kind, const fs::path& path)
{
    DirSnapshot snapshot;
    snapshot.kind = kind;
    snapshot.path = path;
    snapshot.exists = fs::exists(path);

    std::string str = path.generic_string();
    snapshot.startsWithRuntime = str.rfind("runtime", 0) == 0 || str.rfind("./runtime", 0) == 0;
    return snapshot;
}

FileSnapshot makeFileSnapshot(const std::string& name, const fs::path& path, bool createFile)
{
    FileSnapshot snapshot;
    snapshot.name = name;
    snapshot.path = path;
    snapshot.parent = path.parent_path();

    if (createFile) {
        std::ofstream ofs(path, std::ios::out | std::ios::trunc);
        if (ofs.is_open()) {
            ofs << "probe:" << name << "\n";
            ofs.close();
        }
    }

    snapshot.exists = fs::exists(path);
    return snapshot;
}

void printJson(
    const std::string& scenario,
    bool portableMode,
    bool consistent,
    const std::vector<DirSnapshot>& dirs,
    const std::vector<FileSnapshot>& files,
    const std::vector<std::string>& notes)
{
    std::cout << "{";
    std::cout << "\"scenario\":\"" << scenario << "\",";
    std::cout << "\"portable\":" << (portableMode ? "true" : "false") << ",";
    std::cout << "\"consistent_calls\":" << (consistent ? "true" : "false") << ",";
    std::cout << "\"dirs\":[";
    for (size_t i = 0; i < dirs.size(); ++i) {
        const auto& d = dirs[i];
        if (i > 0) std::cout << ",";
        std::cout << "{";
        std::cout << "\"kind\":\"" << d.kind << "\",";
        std::cout << "\"path\":\"" << d.path.generic_string() << "\",";
        std::cout << "\"exists\":" << (d.exists ? "true" : "false") << ",";
        std::cout << "\"starts_with_runtime\":" << (d.startsWithRuntime ? "true" : "false");
        std::cout << "}";
    }
    std::cout << "],";
    std::cout << "\"files\":[";
    for (size_t i = 0; i < files.size(); ++i) {
        const auto& f = files[i];
        if (i > 0) std::cout << ",";
        std::cout << "{";
        std::cout << "\"name\":\"" << f.name << "\",";
        std::cout << "\"path\":\"" << f.path.generic_string() << "\",";
        std::cout << "\"parent\":\"" << f.parent.generic_string() << "\",";
        std::cout << "\"exists\":" << (f.exists ? "true" : "false");
        std::cout << "}";
    }
    std::cout << "],";
    std::cout << "\"notes\":[";
    for (size_t i = 0; i < notes.size(); ++i) {
        if (i > 0) std::cout << ",";
        std::cout << "\"" << notes[i] << "\"";
    }
    std::cout << "]";
    std::cout << "}\n";
}

void runScenario(const std::string& scenario)
{
    bool createFiles = (scenario == "env_portable" || scenario == "file_portable");
    std::vector<std::string> notes;

    if (scenario == "env_portable") {
        clearPortableArtifacts();
        setEnv("GLINT_PORTABLE", "1");
        notes.push_back("GLINT_PORTABLE=1");
    } else if (scenario == "file_portable") {
        clearPortableArtifacts();
        setEnv("GLINT_PORTABLE", "");
        fs::create_directories("runtime");
        std::ofstream marker("runtime/.portable");
        if (marker.is_open()) {
            marker << "portable\n";
            marker.close();
        }
        notes.push_back("runtime/.portable marker created");
    } else {
        clearPortableArtifacts();
        setEnv("GLINT_PORTABLE", "");
        notes.push_back("portable mode disabled");
    }

    fs::path dataDir = glint::getUserDataDir();
    fs::path configDir = glint::getConfigDir();
    fs::path cacheDir = glint::getCacheDir();

    bool consistent = true;
    consistent &= (dataDir == glint::getUserDataDir());
    consistent &= (configDir == glint::getConfigDir());
    consistent &= (cacheDir == glint::getCacheDir());

    DirSnapshot dataSnap = makeDirSnapshot("data", dataDir);
    DirSnapshot configSnap = makeDirSnapshot("config", configDir);
    DirSnapshot cacheSnap = makeDirSnapshot("cache", cacheDir);

    auto historyPath = glint::getDataPath("history.txt");
    auto recentPath = glint::getDataPath("recent.txt");
    auto imguiPath = glint::getConfigPath("imgui.ini");
    auto cachePath = glint::getCachePath("tmp/probe.bin");

    std::vector<FileSnapshot> files;
    files.push_back(makeFileSnapshot("history.txt", historyPath, createFiles));
    files.push_back(makeFileSnapshot("recent.txt", recentPath, createFiles));
    files.push_back(makeFileSnapshot("imgui.ini", imguiPath, createFiles));
    files.push_back(makeFileSnapshot("tmp/probe.bin", cachePath, createFiles));

    bool portableMode = glint::isPortableMode();

    printJson(
        scenario,
        portableMode,
        consistent,
        {dataSnap, configSnap, cacheSnap},
        files,
        notes);
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: user_paths_probe <default|env_portable|file_portable>\n";
        return 1;
    }

    try {
        runScenario(argv[1]);
    } catch (const std::exception& ex) {
        std::cerr << "{\"scenario\":\"" << argv[1] << "\",\"error\":\"" << ex.what() << "\"}\n";
        return 2;
    }

    return 0;
}

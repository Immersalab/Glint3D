#pragma once

#include <string>

// Minimal AI bridges over local Ollama HTTP API.
// - NLToJSONBridge: legacy translator to a JSON micro-DSL (kept for compatibility)
// - AIPlanner: returns natural-language imperative steps that our app can execute directly
// Default provider targets a local Ollama server at http://127.0.0.1:11434.
// No external libs required; uses WinHTTP.

struct AIConfig {
    std::string endpoint = "http://127.0.0.1:11434"; // Ollama default
    std::string model = "llama3.2";      // small, fast model
};

class NLToJSONBridge {
public:
    explicit NLToJSONBridge(const AIConfig& cfg = {}) : m_cfg(cfg) {}

    void setConfig(const AIConfig& cfg) { m_cfg = cfg; }
    const AIConfig& config() const { return m_cfg; }

    // Returns true on success, and fills outJson with the micro-DSL JSON.
    // On failure, returns false and sets error.
    bool translate(const std::string& natural, std::string& outJson, std::string& error);

private:
    AIConfig m_cfg;
};

// Produces natural-language imperative commands (one per line) based on
// the user's instruction and the current scene JSON provided as context.
// The commands are designed to be executed by our local NL executor.
class AIPlanner {
public:
    explicit AIPlanner(const AIConfig& cfg = {}) : m_cfg(cfg) {}
    void setConfig(const AIConfig& cfg) { m_cfg = cfg; }
    const AIConfig& config() const { return m_cfg; }

    // Returns true on success. outPlan contains one or more lines like:
    //   place cow in front of me 2
    //   add light at 0 5 5 color 1 1 1 intensity 1.2
    //   create material wood color 0.6 0.4 0.2
    bool plan(const std::string& natural,
              const std::string& sceneJson,
              std::string& outPlan,
              std::string& error);

private:
    AIConfig m_cfg;
};

// Machine Summary Block
// {"file":"engine/core/application/cli_parser.h","purpose":"Declares CLI parsing utilities and logging helpers for the legacy application path.","exports":["CLIExitCode","LogLevel","CLIOptions","CLIParser","Logger"],"depends_on":["render_settings.h","<string>","<vector>"],"notes":["legacy_cli_parser","exit_code_contract","logging_utilities"]}
// Human Summary
// Provides option parsing and logging infrastructure used by the legacy command-line entry point.

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "render_settings.h"

enum class CLIExitCode : int {
    Success = 0,
    SchemaValidationError = 2,
    FileNotFound = 3,
    RuntimeError = 4,
    UnknownFlag = 5,
    DependencyError = 6,
    DeterminismError = 7
};

enum class LogLevel {
    Quiet,
    Warn,
    Info,
    Debug
};

struct CLIOptions {
    bool showHelp = false;
    bool showVersion = false;
    bool headlessMode = false;
    bool enableDenoise = false;
    bool forceRaytrace = false;
    bool strictSchema = false;
    
    std::string opsFile;
    std::string outputFile;
    std::string schemaVersion = "v1.3";
    std::string assetRoot;
    LogLevel logLevel = LogLevel::Info;
    
    int outputWidth = 1024;
    int outputHeight = 1024;
    int reflectionSpp = 8; // Default reflection samples per pixel
    
    // Render settings
    RenderSettings renderSettings;
};

struct CLIParseResult {
    CLIExitCode exitCode = CLIExitCode::Success;
    std::string errorMessage;
    CLIOptions options;
};

class CLIParser {
public:
    static CLIParseResult parse(int argc, char** argv);
    static void printHelp();
    static void printVersion();
    static const char* exitCodeToString(CLIExitCode code);
    
private:
    static bool isValidFlag(const std::string& flag);
    static bool isValidLogLevel(const std::string& level);
    static bool isValidSchemaVersion(const std::string& version);
    static bool isValidSeed(const std::string& seed);
    static bool isValidExposure(const std::string& exposure);
    static bool isValidGamma(const std::string& gamma);
    static LogLevel parseLogLevel(const std::string& level);
    static uint32_t parseSeed(const std::string& seed);
    static float parseExposure(const std::string& exposure);
    static float parseGamma(const std::string& gamma);
    static std::vector<std::string> getValidFlags();
};

class Logger {
public:
    static void setLevel(LogLevel level);
    static LogLevel getLevel();
    
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    
private:
    static LogLevel s_currentLevel;
    static void log(LogLevel level, const std::string& prefix, const std::string& message);
    static std::string getCurrentTimestamp();
};

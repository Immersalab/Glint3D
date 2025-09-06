#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

enum class CLIExitCode : int {
    Success = 0,
    SchemaValidationError = 2,
    FileNotFound = 3,
    RuntimeError = 4,
    UnknownFlag = 5
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
    static LogLevel parseLogLevel(const std::string& level);
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
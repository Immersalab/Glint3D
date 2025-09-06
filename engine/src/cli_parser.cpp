#include "cli_parser.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include "help_text.h"

LogLevel Logger::s_currentLevel = LogLevel::Info;

CLIParseResult CLIParser::parse(int argc, char** argv)
{
    CLIParseResult result;
    
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    
    // First pass: check for unknown flags
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i].rfind("--", 0) == 0) { // starts with "--"
            if (!isValidFlag(args[i])) {
                result.exitCode = CLIExitCode::UnknownFlag;
                result.errorMessage = "Unknown flag: " + args[i];
                return result;
            }
        }
    }
    
    auto hasFlag = [&](const std::string& flag) -> bool {
        return std::find(args.begin(), args.end(), flag) != args.end();
    };
    
    auto getValue = [&](const std::string& flag, const std::string& defaultValue = "") -> std::string {
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == flag) {
                if (i + 1 >= args.size() || args[i + 1].rfind("--", 0) == 0) {
                    return defaultValue;
                }
                return args[i + 1];
            }
        }
        return defaultValue;
    };
    
    auto getIntValue = [&](const std::string& flag, int defaultValue) -> int {
        std::string val = getValue(flag);
        if (val.empty()) return defaultValue;
        try {
            return std::stoi(val);
        } catch (...) {
            return defaultValue;
        }
    };
    
    // Parse flags
    result.options.showHelp = hasFlag("--help");
    result.options.showVersion = hasFlag("--version");
    result.options.enableDenoise = hasFlag("--denoise");
    result.options.forceRaytrace = hasFlag("--raytrace");
    result.options.strictSchema = hasFlag("--strict-schema");
    
    // Parse values
    result.options.opsFile = getValue("--ops");
    result.options.outputFile = getValue("--render");
    result.options.assetRoot = getValue("--asset-root");
    result.options.outputWidth = getIntValue("--w", 1024);
    result.options.outputHeight = getIntValue("--h", 1024);

    // Validate presence of values for flags that require them
    if (hasFlag("--ops") && result.options.opsFile.empty()) {
        result.exitCode = CLIExitCode::UnknownFlag;
        result.errorMessage = "Missing value for --ops (expected a file path)";
        return result;
    }
    
    if (hasFlag("--asset-root") && result.options.assetRoot.empty()) {
        result.exitCode = CLIExitCode::UnknownFlag;
        result.errorMessage = "Missing value for --asset-root (expected a directory path)";
        return result;
    }
    
    // Parse schema version
    std::string schemaVersionStr = getValue("--schema-version", "v1.3");
    if (hasFlag("--schema-version") && schemaVersionStr.empty()) {
        result.exitCode = CLIExitCode::UnknownFlag;
        result.errorMessage = "Missing value for --schema-version (expected e.g. v1.3)";
        return result;
    }
    if (!isValidSchemaVersion(schemaVersionStr)) {
        result.exitCode = CLIExitCode::UnknownFlag;
        result.errorMessage = "Invalid schema version: " + schemaVersionStr + " (supported: v1.3)";
        return result;
    }
    result.options.schemaVersion = schemaVersionStr;
    
    // Parse log level
    std::string logLevelStr = getValue("--log", "info");
    if (hasFlag("--log") && logLevelStr.empty()) {
        result.exitCode = CLIExitCode::UnknownFlag;
        result.errorMessage = "Missing value for --log (expected quiet|warn|info|debug)";
        return result;
    }
    if (!isValidLogLevel(logLevelStr)) {
        result.exitCode = CLIExitCode::UnknownFlag;
        result.errorMessage = "Invalid log level: " + logLevelStr + " (supported: quiet, warn, info, debug)";
        return result;
    }
    result.options.logLevel = parseLogLevel(logLevelStr);
    
    // Determine headless mode
    result.options.headlessMode = hasFlag("--ops") || hasFlag("--render");
    
    // Validate file existence for ops file
    if (!result.options.opsFile.empty()) {
        if (!std::filesystem::exists(result.options.opsFile)) {
            result.exitCode = CLIExitCode::FileNotFound;
            result.errorMessage = "Operations file not found: " + result.options.opsFile;
            return result;
        }
    }
    
    // Validate asset root directory
    if (!result.options.assetRoot.empty()) {
        if (!std::filesystem::exists(result.options.assetRoot)) {
            result.exitCode = CLIExitCode::FileNotFound;
            result.errorMessage = "Asset root directory not found: " + result.options.assetRoot;
            return result;
        }
        if (!std::filesystem::is_directory(result.options.assetRoot)) {
            result.exitCode = CLIExitCode::UnknownFlag;
            result.errorMessage = "Asset root path is not a directory: " + result.options.assetRoot;
            return result;
        }
    }
    
    // Validate dimensions
    if (result.options.outputWidth <= 0 || result.options.outputHeight <= 0) {
        result.exitCode = CLIExitCode::UnknownFlag;
        result.errorMessage = "Output dimensions must be positive integers";
        return result;
    }
    
    return result;
}

void CLIParser::printHelp()
{
    print_cli_help();
}

void CLIParser::printVersion()
{
    printf("0.3.0\n");
}

const char* CLIParser::exitCodeToString(CLIExitCode code)
{
    switch (code) {
        case CLIExitCode::Success: return "Success";
        case CLIExitCode::SchemaValidationError: return "Schema validation error";
        case CLIExitCode::FileNotFound: return "File not found";
        case CLIExitCode::RuntimeError: return "Runtime error";
        case CLIExitCode::UnknownFlag: return "Unknown flag";
        default: return "Unknown error";
    }
}

bool CLIParser::isValidFlag(const std::string& flag)
{
    static const std::vector<std::string> validFlags = getValidFlags();
    return std::find(validFlags.begin(), validFlags.end(), flag) != validFlags.end();
}

bool CLIParser::isValidLogLevel(const std::string& level)
{
    return level == "quiet" || level == "warn" || level == "info" || level == "debug";
}

bool CLIParser::isValidSchemaVersion(const std::string& version)
{
    return version == "v1.3";
}

LogLevel CLIParser::parseLogLevel(const std::string& level)
{
    if (level == "quiet") return LogLevel::Quiet;
    if (level == "warn") return LogLevel::Warn;
    if (level == "info") return LogLevel::Info;
    if (level == "debug") return LogLevel::Debug;
    return LogLevel::Info;
}

std::vector<std::string> CLIParser::getValidFlags()
{
    return {
        "--help",
        "--version",
        "--ops",
        "--render",
        "--asset-root",
        "--w",
        "--h",
        "--denoise",
        "--raytrace",
        "--strict-schema",
        "--schema-version",
        "--log"
    };
}

// Logger implementation
void Logger::setLevel(LogLevel level)
{
    s_currentLevel = level;
}

LogLevel Logger::getLevel()
{
    return s_currentLevel;
}

void Logger::debug(const std::string& message)
{
    if (s_currentLevel >= LogLevel::Debug) {
        log(LogLevel::Debug, "[DEBUG]", message);
    }
}

void Logger::info(const std::string& message)
{
    if (s_currentLevel >= LogLevel::Info) {
        log(LogLevel::Info, "[INFO]", message);
    }
}

void Logger::warn(const std::string& message)
{
    if (s_currentLevel >= LogLevel::Warn) {
        log(LogLevel::Warn, "[WARN]", message);
    }
}

void Logger::error(const std::string& message)
{
    log(LogLevel::Debug, "[ERROR]", message);
}

void Logger::log(LogLevel level, const std::string& prefix, const std::string& message)
{
    if (s_currentLevel == LogLevel::Quiet) return;
    
    std::string timestamp = getCurrentTimestamp();
    std::fprintf(stderr, "%s %s %s\n", timestamp.c_str(), prefix.c_str(), message.c_str());
}

std::string Logger::getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
#ifdef _WIN32
    struct tm timeinfo;
    localtime_s(&timeinfo, &time_t);
    ss << std::put_time(&timeinfo, "%H:%M:%S");
#else
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
#endif
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

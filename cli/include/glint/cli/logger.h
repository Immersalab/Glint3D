// Machine Summary Block
// {"file":"cli/include/glint/cli/logger.h","purpose":"Declares centralized logging infrastructure for the Glint CLI platform.","exports":["glint::cli::LogLevel","glint::cli::Logger","glint::cli::LogConfig"],"depends_on":["<string>","<chrono>"],"notes":["severity_levels","timestamp_support","configurable_output","thread_safe"]}
// Human Summary
// Centralized logging system with configurable verbosity, timestamps, and color support for CLI platform.

#pragma once

#include <string>
#include <chrono>
#include <memory>

/**
 * @file logger.h
 * @brief Centralized logging infrastructure for Glint CLI.
 */

namespace glint::cli {

/// @brief Logging severity levels.
enum class LogLevel {
    Quiet = 0,   ///< No output (errors only)
    Warn = 1,    ///< Warnings and errors
    Info = 2,    ///< Informational messages (default)
    Debug = 3    ///< Verbose debugging output
};

/// @brief Logging configuration options.
struct LogConfig {
    LogLevel level = LogLevel::Info;     ///< Minimum log level to display
    bool timestamps = false;             ///< Include timestamps in output
    bool color = true;                   ///< Use ANSI color codes (auto-detect TTY)
    bool jsonMode = false;               ///< Output as NDJSON events instead of text

    /// @brief Auto-detect color support based on terminal capabilities.
    static bool detectColorSupport();
};

/**
 * @brief Centralized logger for CLI platform.
 *
 * Thread-safe logging with configurable verbosity, timestamps, and formatting.
 * Supports both human-readable text output and machine-readable NDJSON.
 *
 * **Usage:**
 * ```cpp
 * Logger::setConfig(config);
 * Logger::info("Processing file: {}", filename);
 * Logger::warn("Deprecated flag used: --old-flag");
 * Logger::error("Failed to load scene: {}", error);
 * ```
 */
class Logger {
public:
    /**
     * @brief Set global logger configuration.
     * @param config Logging configuration.
     */
    static void setConfig(const LogConfig& config);

    /**
     * @brief Get current logger configuration.
     * @return Current configuration.
     */
    static LogConfig getConfig();

    /**
     * @brief Set minimum log level.
     * @param level Minimum severity level to display.
     */
    static void setLevel(LogLevel level);

    /**
     * @brief Get current log level.
     * @return Current minimum severity level.
     */
    static LogLevel getLevel();

    /**
     * @brief Log debug message.
     * @param message Message string.
     */
    static void debug(const std::string& message);

    /**
     * @brief Log informational message.
     * @param message Message string.
     */
    static void info(const std::string& message);

    /**
     * @brief Log warning message.
     * @param message Message string.
     */
    static void warn(const std::string& message);

    /**
     * @brief Log error message.
     * @param message Message string.
     */
    static void error(const std::string& message);

    /**
     * @brief Parse log level from string.
     * @param levelStr String representation (quiet, warn, info, debug).
     * @return Corresponding log level, or Info if unrecognized.
     */
    static LogLevel parseLevel(const std::string& levelStr);

    /**
     * @brief Convert log level to string.
     * @param level Log level.
     * @return String representation.
     */
    static std::string levelToString(LogLevel level);

private:
    /**
     * @brief Internal logging implementation.
     * @param level Message severity level.
     * @param prefix Human-readable prefix (e.g., "INFO", "WARN").
     * @param message Message content.
     * @param colorCode ANSI color code (empty if color disabled).
     */
    static void log(LogLevel level, const std::string& prefix,
                   const std::string& message, const std::string& colorCode);

    /**
     * @brief Get current timestamp string.
     * @return ISO 8601 timestamp with milliseconds.
     */
    static std::string getCurrentTimestamp();

    static LogConfig s_config;  ///< Global logger configuration
};

} // namespace glint::cli

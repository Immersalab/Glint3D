// Machine Summary Block
// {"file":"cli/src/logger.cpp","purpose":"Implements centralized logging infrastructure for the Glint CLI platform.","depends_on":["glint/cli/logger.h","<iostream>","<iomanip>","<sstream>","<ctime>","<mutex>"],"notes":["thread_safe","ansi_colors","iso8601_timestamps","ndjson_support"]}
// Human Summary
// Thread-safe logging implementation with ANSI color support, timestamps, and NDJSON output mode for machine consumption.

#include "glint/cli/logger.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <mutex>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

namespace glint::cli {

namespace {

// ANSI color codes
constexpr const char* COLOR_RESET = "\033[0m";
constexpr const char* COLOR_RED = "\033[31m";
constexpr const char* COLOR_YELLOW = "\033[33m";
constexpr const char* COLOR_BLUE = "\033[34m";
constexpr const char* COLOR_GRAY = "\033[90m";

// Global mutex for thread-safe logging
std::mutex g_logMutex;

} // namespace

// Static member initialization
LogConfig Logger::s_config;

bool LogConfig::detectColorSupport() {
#ifdef _WIN32
    // Enable ANSI color support on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        return false;
    }

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) {
        return false;
    }

    // Check if stdout is a TTY
    return ISATTY(FILENO(stdout)) != 0;
#else
    // On Unix, check if stdout is a TTY and TERM is set
    if (ISATTY(FILENO(stdout)) == 0) {
        return false;
    }

    const char* term = std::getenv("TERM");
    if (!term || std::string(term) == "dumb") {
        return false;
    }

    return true;
#endif
}

void Logger::setConfig(const LogConfig& config) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    s_config = config;
}

LogConfig Logger::getConfig() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    return s_config;
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    s_config.level = level;
}

LogLevel Logger::getLevel() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    return s_config.level;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::Debug, "DEBUG", message, COLOR_GRAY);
}

void Logger::info(const std::string& message) {
    log(LogLevel::Info, "INFO", message, COLOR_BLUE);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::Warn, "WARN", message, COLOR_YELLOW);
}

void Logger::error(const std::string& message) {
    log(LogLevel::Quiet, "ERROR", message, COLOR_RED);
}

LogLevel Logger::parseLevel(const std::string& levelStr) {
    std::string lower = levelStr;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "quiet") return LogLevel::Quiet;
    if (lower == "warn" || lower == "warning") return LogLevel::Warn;
    if (lower == "info") return LogLevel::Info;
    if (lower == "debug") return LogLevel::Debug;

    return LogLevel::Info; // Default
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Quiet: return "quiet";
        case LogLevel::Warn: return "warn";
        case LogLevel::Info: return "info";
        case LogLevel::Debug: return "debug";
        default: return "info";
    }
}

void Logger::log(LogLevel level, const std::string& prefix,
                const std::string& message, const std::string& colorCode) {
    std::lock_guard<std::mutex> lock(g_logMutex);

    // Check if message should be displayed based on current log level
    if (level > s_config.level) {
        return;
    }

    // NDJSON mode
    if (s_config.jsonMode) {
        std::ostringstream oss;
        oss << "{\"event\":\"";

        // Convert prefix to event type
        if (prefix == "ERROR") {
            oss << "error";
        } else if (prefix == "WARN") {
            oss << "warning";
        } else if (prefix == "INFO") {
            oss << "info";
        } else {
            oss << "debug";
        }

        oss << "\",\"timestamp\":\"" << getCurrentTimestamp() << "\"";
        oss << ",\"message\":\"";

        // Escape special characters in message
        for (char c : message) {
            if (c == '"') {
                oss << "\\\"";
            } else if (c == '\\') {
                oss << "\\\\";
            } else if (c == '\n') {
                oss << "\\n";
            } else if (c == '\t') {
                oss << "\\t";
            } else {
                oss << c;
            }
        }

        oss << "\"}\n";
        std::cout << oss.str() << std::flush;
        return;
    }

    // Human-readable mode
    std::ostringstream oss;

    // Timestamp
    if (s_config.timestamps) {
        oss << "[" << getCurrentTimestamp() << "] ";
    }

    // Color prefix
    if (s_config.color) {
        oss << colorCode << "[" << prefix << "]" << COLOR_RESET << " ";
    } else {
        oss << "[" << prefix << "] ";
    }

    // Message
    oss << message << "\n";

    // Output to stderr for errors/warnings, stdout for info/debug
    if (level <= LogLevel::Warn) {
        std::cerr << oss.str() << std::flush;
    } else {
        std::cout << oss.str() << std::flush;
    }
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tmSnapshot{};

#ifdef _WIN32
    gmtime_s(&tmSnapshot, &time);
#else
    gmtime_r(&time, &tmSnapshot);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmSnapshot, "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << "Z";
    return oss.str();
}

} // namespace glint::cli

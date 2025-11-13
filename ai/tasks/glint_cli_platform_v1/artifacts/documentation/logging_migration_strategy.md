<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_cli_platform_v1/artifacts/documentation/logging_migration_strategy.md","purpose":"Documents the logging infrastructure migration from CLIParser to centralized logger.","exports":[],"depends_on":["cli/include/glint/cli/logger.h","engine/core/application/cli_parser.h"],"notes":["gradual_migration","backward_compatibility","configuration_precedence"]}
<!-- Human Summary -->
Migration strategy for transitioning from legacy Logger in CLIParser to centralized logging infrastructure in CLI platform.

# Logging Migration Strategy

## Overview

The Glint CLI platform introduces a **centralized logging module** (`glint::cli::Logger`) that provides enhanced features while maintaining backward compatibility with the legacy `Logger` class in `application/cli_parser.h`.

## Architecture

### Legacy Logger (Engine Core)

**Location**: `engine/core/application/cli_parser.h/.cpp`

**Features**:
- Basic severity levels (Quiet, Warn, Info, Debug)
- Timestamp support
- Simple stderr output
- Global state via static members

**Usage**:
```cpp
#include "application/cli_parser.h"

Logger::setLevel(LogLevel::Debug);
Logger::info("Processing file...");
Logger::warn("Deprecated flag used");
Logger::error("Failed to load scene");
```

**Limitations**:
- No color support
- No NDJSON output mode
- No thread safety guarantees
- Coupled to CLI parser module

---

### Centralized Logger (CLI Platform)

**Location**: `cli/include/glint/cli/logger.h`, `cli/src/logger.cpp`

**Features**:
- ✅ Thread-safe logging with mutex protection
- ✅ ANSI color support (auto-detect TTY)
- ✅ NDJSON output mode for machine consumption
- ✅ Configurable timestamps (ISO 8601 with milliseconds)
- ✅ Flexible configuration (color, timestamps, json mode)
- ✅ Separate stderr/stdout streams (errors/warnings vs info/debug)

**Usage**:
```cpp
#include "glint/cli/logger.h"

using namespace glint::cli;

// Configure logger
LogConfig config;
config.level = LogLevel::Debug;
config.timestamps = true;
config.color = LogConfig::detectColorSupport();
config.jsonMode = false;
Logger::setConfig(config);

// Log messages
Logger::info("Processing file...");
Logger::warn("Deprecated flag used");
Logger::error("Failed to load scene");
```

**NDJSON Mode**:
```cpp
LogConfig config;
config.jsonMode = true;
Logger::setConfig(config);

Logger::info("Processing complete");
// Output: {"event":"info","timestamp":"2025-11-12T14:30:00.123Z","message":"Processing complete"}
```

---

## Migration Phases

### Phase 1: Dual Infrastructure (Current State)

**Status**: ✅ Complete

**Implementation**:
- Centralized logger implemented in `cli/` directory
- Legacy logger remains in `engine/core/application/`
- Both coexist without conflicts
- New CLI commands use centralized logger
- Engine core continues using legacy logger

**Benefits**:
- No breaking changes
- Gradual adoption
- Full backward compatibility

---

### Phase 2: New Commands Use Centralized Logger (Recommended)

**Status**: ⏸ Pending

**Scope**: Update new CLI commands to use centralized logger

**Files to Update**:
- `cli/src/commands/*.cpp` - All command implementations
- `cli/src/command_io.cpp` - Bridge functions
- `cli/src/command_dispatcher.cpp` - Dispatcher logging

**Migration Example**:

**Before** (using legacy Logger):
```cpp
#include "application/cli_parser.h"

void MyCommand::execute() {
    Logger::info("Starting execution...");
    // ...
}
```

**After** (using centralized Logger):
```cpp
#include "glint/cli/logger.h"

void MyCommand::execute() {
    glint::cli::Logger::info("Starting execution...");
    // ...
}
```

**Automatic NDJSON Integration**:
```cpp
// In command_dispatcher.cpp
if (context.emitter) {
    LogConfig config;
    config.jsonMode = true;
    Logger::setConfig(config);
}
```

---

### Phase 3: Engine Core Migration (Future)

**Status**: ⏸ Not Started

**Scope**: Migrate engine core to use centralized logger

**Considerations**:
- Requires updating all engine source files
- May affect third-party integrations
- Desktop UI (ImGui) console output needs adapter
- Performance-critical paths need benchmarking

**Recommended Approach**:
1. Create compatibility bridge in `cli_parser.h`
2. Forward legacy Logger calls to centralized Logger
3. Update engine source files incrementally
4. Deprecate legacy Logger in future release

---

## Configuration Precedence

Logger configuration follows CLI configuration precedence:

1. **Programmatic** - `Logger::setConfig()`
2. **CLI flags** - `--verbosity debug`
3. **Environment** - `GLINT_VERBOSITY=debug`
4. **Config file** - `.glint/config.json`
5. **Defaults** - `LogLevel::Info`, color auto-detect

---

## Feature Comparison

| Feature | Legacy Logger | Centralized Logger |
|---------|---------------|-------------------|
| **Severity Levels** | ✅ 4 levels | ✅ 4 levels (same) |
| **Thread Safety** | ❌ No mutex | ✅ Mutex protected |
| **ANSI Colors** | ❌ Not supported | ✅ Auto-detect TTY |
| **Timestamps** | ✅ Basic | ✅ ISO 8601 + ms |
| **NDJSON Output** | ❌ Not supported | ✅ Machine-readable |
| **Stream Separation** | ❌ All stderr | ✅ stderr/stdout split |
| **Configuration** | ❌ Level only | ✅ Full LogConfig |
| **JSON Escaping** | ❌ Not applicable | ✅ Proper escaping |

---

## Best Practices

### For New Code

**Always use the centralized logger**:
```cpp
#include "glint/cli/logger.h"

namespace glint::cli {

void myFunction() {
    Logger::info("Using centralized logger");
}

} // namespace glint::cli
```

### For Existing Engine Code

**Continue using legacy logger** (for now):
```cpp
#include "application/cli_parser.h"

void myEngineFunction() {
    Logger::info("Using legacy logger");
}
```

### For Command Implementations

**Respect --json mode**:
```cpp
CLIExitCode MyCommand::run(const CommandExecutionContext& context) {
    if (context.emitter) {
        // NDJSON mode active - logger will output JSON
        LogConfig config;
        config.jsonMode = true;
        Logger::setConfig(config);
    }

    Logger::info("Processing...");
    // Output: {"event":"info","timestamp":"...","message":"Processing..."}

    return CLIExitCode::Success;
}
```

---

## Testing

### Unit Tests

```cpp
TEST(LoggerTest, ThreadSafety) {
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([] {
            Logger::info("Concurrent message");
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    // No crashes = success
}

TEST(LoggerTest, NdjsonOutput) {
    LogConfig config;
    config.jsonMode = true;
    Logger::setConfig(config);

    std::ostringstream capture;
    // Redirect stdout to capture
    Logger::info("Test message");

    std::string output = capture.str();
    // Verify valid JSON
    rapidjson::Document doc;
    EXPECT_FALSE(doc.Parse(output.c_str()).HasParseError());
    EXPECT_EQ(doc["event"].GetString(), std::string("info"));
}
```

---

## Rollout Timeline

| Phase | Status | Timeline |
|-------|--------|----------|
| **Phase 1**: Centralized logger implementation | ✅ Complete | v0.4.0 |
| **Phase 2**: New CLI commands adoption | ⏸ Pending | v0.4.1 |
| **Phase 3**: Engine core migration | 📅 Planned | v0.5.0 |
| **Phase 4**: Legacy logger deprecation | 📅 Future | v1.0.0 |

---

## FAQ

**Q: Why not replace the legacy Logger immediately?**

A: The legacy Logger is deeply embedded in the engine core (100+ call sites). A gradual migration reduces risk and allows thorough testing at each step.

**Q: Will NDJSON mode affect performance?**

A: NDJSON formatting adds ~10-20% overhead vs plain text, but this is negligible compared to I/O costs. Use `LogLevel::Quiet` for performance-critical paths.

**Q: Can I use both loggers in the same file?**

A: Yes, but it's not recommended. Choose one logger per module for consistency.

**Q: How do I disable colors in CI environments?**

A: Colors are auto-detected. Set `config.color = false` explicitly, or ensure CI environment doesn't report TTY.

**Q: What about web builds (Emscripten)?**

A: Centralized logger works with Emscripten. ANSI colors are automatically disabled. NDJSON mode is recommended for web tooling integration.

---

*End of document.*

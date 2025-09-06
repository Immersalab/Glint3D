# Glint3D Test Suite

This directory contains all tests for the Glint3D engine, organized by type and purpose.

## Directory Structure

```
tests/
├── README.md                   # This file
├── unit/                      # Unit tests (C++)
│   ├── camera_preset_test.cpp
│   ├── path_security_test.cpp  
│   └── ...
├── integration/               # Integration tests
│   ├── json_ops/             # JSON operations tests
│   │   ├── basic/            # Basic functionality tests
│   │   ├── lighting/         # Lighting system tests
│   │   ├── camera/           # Camera system tests
│   │   └── materials/        # Material system tests
│   ├── cli/                  # Command-line interface tests
│   └── rendering/            # End-to-end rendering tests
├── security/                 # Security tests
│   ├── path_traversal/       # Path traversal vulnerability tests
│   └── ...
├── golden/                   # Golden image testing
│   ├── scenes/               # Test scenes for golden image generation
│   ├── references/           # Reference golden images
│   └── tools/                # Golden image comparison tools
├── data/                     # Test assets and fixtures
│   ├── models/               # Test 3D models
│   ├── textures/             # Test textures  
│   ├── scenes/               # Test scene files
│   └── configs/              # Test configuration files
├── scripts/                  # Test automation scripts
│   ├── run_unit_tests.sh
│   ├── run_integration_tests.sh
│   ├── validate_build.sh
│   └── ...
└── results/                  # Test output and artifacts (gitignored)
    ├── coverage/
    ├── reports/
    └── logs/
```

## Test Categories

### Unit Tests (`unit/`)
- **Purpose**: Test individual components in isolation
- **Language**: C++
- **Framework**: Custom assertions (can be extended with Google Test later)
- **Run**: Compiled and executed directly
- **Coverage**: Core engine functionality, utilities, data structures

### Integration Tests (`integration/`)
- **Purpose**: Test component interactions and workflows
- **Language**: JSON (test cases) + validation scripts
- **Run**: Via engine binary with test JSON files
- **Coverage**: JSON operations, CLI workflows, system integration

### Security Tests (`security/`)
- **Purpose**: Validate security controls and prevent vulnerabilities
- **Language**: Mixed (C++, shell scripts, JSON)
- **Coverage**: Path traversal, input validation, access controls

### Golden Image Tests (`golden/`)
- **Purpose**: Visual regression testing via image comparison
- **Language**: Python (comparison tools) + JSON (scenes)
- **Coverage**: Rendering consistency, visual quality assurance

## Running Tests

### All Tests
```bash
# Run complete test suite
make test
# or
scripts/run_all_tests.sh
```

### By Category
```bash
# Unit tests
scripts/run_unit_tests.sh

# Integration tests  
scripts/run_integration_tests.sh

# Security tests
scripts/run_security_tests.sh

# Golden image tests
scripts/run_golden_tests.sh
```

### Individual Tests
```bash
# Single unit test
./builds/desktop/cmake/unit_tests camera_preset_test

# Single integration test
./builds/desktop/cmake/glint --ops tests/integration/json_ops/basic/load_object.json

# Security test
tests/security/path_traversal/test_traversal_attacks.sh
```

## Adding New Tests

### Unit Test
1. Create `tests/unit/my_component_test.cpp`
2. Follow existing patterns for assertions
3. Add to CMake build if using build system

### Integration Test  
1. Create test JSON in appropriate `tests/integration/` subdirectory
2. Add validation script if needed
3. Document expected behavior

### Golden Image Test
1. Add scene to `tests/golden/scenes/`
2. Generate reference image: `python tools/generate_goldens.py`
3. Add to CI golden validation job

## Test Data Management

### Assets (`tests/data/`)
- Keep test assets minimal (small models, low-res textures)
- Use procedural/generated content where possible
- Document asset sources and licensing

### Fixtures
- Reusable test configurations and scenes
- Common test patterns and templates
- Shared utilities and helpers

## CI Integration

Tests are automatically run in CI:
- **PR Validation**: Unit + integration + security tests
- **Golden Image**: Visual regression testing on Linux
- **Cross-Platform**: Windows + Linux validation
- **Artifacts**: Test reports, coverage, failure diagnostics uploaded

## Test Standards

### Naming Conventions
- Unit tests: `component_test.cpp`
- Integration tests: `feature_test.json` or `workflow_test.json`
- Scripts: `action_verb_object.sh` (e.g., `validate_path_security.sh`)

### Documentation
- Each test should have clear purpose and expected outcomes
- Complex tests need inline documentation
- Failure modes and diagnostics should be documented

### Reliability
- Tests must be deterministic and reproducible
- No external dependencies (network, specific files)
- Clean up after execution (temp files, state changes)

## Maintenance

### Regular Tasks
- Update golden images when rendering changes intentionally
- Review and prune obsolete tests
- Update test data when formats change
- Monitor test execution time and optimize slow tests

### When Adding Features
- Add corresponding unit tests for new components
- Create integration tests for new workflows
- Update golden tests if visual changes occur
- Add security tests for new input handling
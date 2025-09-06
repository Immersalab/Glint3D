# Asset Root Security Feature

## Overview

The `--asset-root` flag provides directory traversal protection for Glint3D by restricting file access to a specified directory tree. This feature prevents malicious JSON operations files from accessing files outside of the designated asset directory.

## Problem Statement

JSON operations can load files using the `load` operation, render to files using `render_image`, and load skyboxes using `set_background`. Without restrictions, these operations could be exploited to:

- Access system files (e.g., `/etc/passwd`, `C:\Windows\System32\config\SAM`)
- Read sensitive application files or configuration
- Write output files to arbitrary locations
- Perform directory traversal attacks using `../` sequences

## Security Implementation

### Path Validation Process

1. **Input Validation**: Check for empty paths and invalid characters
2. **Traversal Detection**: Reject paths containing `..` sequences
3. **Path Normalization**: Resolve relative paths and normalize separators
4. **Boundary Checking**: Ensure resolved paths remain within the asset root
5. **Case-Insensitive Comparison**: Handle filesystem case sensitivity properly

### Protected Operations

The following JSON operations are protected by asset root validation:

- `load`: Asset file loading (models, textures)
- `render_image`: Output file path validation
- `set_background`: Skybox file loading

### Error Handling

When path validation fails, operations return clear error messages:

```
load: Path contains directory traversal attempts (..)
render_image: Path resolves outside the asset root directory
set_background: Asset root directory has not been configured
```

## Usage

### Command Line

```bash
# Set asset root to restrict file access
glint --asset-root /path/to/assets --ops operations.json --render output.png

# Without asset root (backward compatible, no restrictions)
glint --ops operations.json --render output.png
```

### Example Directory Structure

```
project/
├── assets/                 # Asset root directory
│   ├── models/
│   │   ├── character.obj
│   │   └── environment.fbx
│   ├── textures/
│   │   ├── diffuse.png
│   │   └── normal.png
│   └── skyboxes/
│       └── sunset.hdr
├── operations.json
└── output/
```

### Valid Operations Example

With `--asset-root ./assets`, these operations are allowed:

```json
{
  "ops": [
    {
      "op": "load",
      "path": "models/character.obj"
    },
    {
      "op": "load", 
      "path": "textures/diffuse.png"
    },
    {
      "op": "set_background",
      "skybox": "skyboxes/sunset.hdr"
    }
  ]
}
```

### Blocked Operations Example

These operations would be rejected:

```json
{
  "ops": [
    {
      "op": "load",
      "path": "../../../etc/passwd"
    },
    {
      "op": "load",
      "path": "..\\..\\Windows\\System32\\config\\SAM"
    },
    {
      "op": "render_image",
      "path": "/tmp/malicious_output.png"
    }
  ]
}
```

## Testing

### Unit Tests

Run the comprehensive test suite:

```bash
# Build and run path security tests
g++ -std=c++17 -I./engine/include tests/unit/path_security_test.cpp engine/src/path_security.cpp -o test_path_security
./test_path_security
```

### Integration Tests

Test the full application with security controls:

```bash
# Run the security validation script
bash tests/scripts/run_security_tests.sh
```

### Negative Test Cases

The `tests/security/path_traversal/` directory contains various path traversal attack tests that should all fail when `--asset-root` is set.

## Backward Compatibility

When `--asset-root` is **not** specified:
- All paths are allowed (existing behavior)
- No validation is performed
- Existing JSON operations files continue to work unchanged

When `--asset-root` **is** specified:
- Only paths within the asset root directory are allowed
- Path traversal attempts are blocked
- Clear error messages guide users to fix invalid paths

## Implementation Details

### Core Components

1. **PathSecurity namespace** (`path_security.h/cpp`):
   - `setAssetRoot()`: Configure the root directory
   - `validatePath()`: Check if a path is allowed
   - `resolvePath()`: Normalize and resolve paths safely

2. **CLI Integration** (`cli_parser.h/cpp`):
   - Added `--asset-root` argument parsing
   - Directory existence validation
   - Integration with main application

3. **JSON Ops Integration** (`json_ops.cpp`):
   - Path validation for file operations
   - Clear error reporting
   - Backward compatibility preservation

### Security Considerations

- **Double-encoded attacks**: Paths are validated before URL/encoding processing
- **Case sensitivity**: Comparisons handle filesystem differences
- **Symlink protection**: Path resolution follows logical paths, not physical links
- **Unicode handling**: Proper UTF-8 path handling on all platforms
- **Race conditions**: Validation and access are atomic where possible

## Error Codes

The application returns appropriate exit codes for security failures:

- `3`: File not found (including asset root directory)
- `4`: Runtime error (including path validation failures)
- `5`: Invalid arguments (including malformed asset root path)

## Best Practices

### For Users

1. **Always specify --asset-root** when processing untrusted JSON files
2. **Use absolute paths** for asset root to avoid confusion
3. **Organize assets** in a dedicated directory structure
4. **Test operations** with the security flag before production use

### For Developers

1. **Never bypass** path validation in file operations
2. **Log security violations** appropriately
3. **Test both positive and negative cases** for path validation
4. **Document file access patterns** in operation specifications

### For System Administrators

1. **Restrict asset root directories** to necessary files only
2. **Monitor for validation failures** in logs
3. **Use least-privilege principles** for file system permissions
4. **Regular security audits** of asset directory contents

## Future Enhancements

Potential improvements to the security system:

- **Allowlist file extensions**: Only permit specific file types
- **Size limits**: Restrict maximum file sizes for operations
- **Rate limiting**: Prevent excessive file access attempts
- **Audit logging**: Detailed security event logging
- **Network restrictions**: Block network-accessible paths
- **Sandboxing**: Additional OS-level isolation

## Support

For questions about the asset root security feature:

1. Check this documentation first
2. Run the test suite to verify behavior
3. Review the source code in `engine/src/path_security.cpp`
4. Open an issue with detailed reproduction steps
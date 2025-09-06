# Application Icon Setup

This document describes how the Glint3D application icon is integrated into the Windows executable.

## Overview

The Glint3D logo is embedded into the Windows executable using Windows resources. When users see the executable in Windows Explorer or the taskbar, they'll see the Glint3D diamond logo instead of the default application icon.

## Files

- **Source Logo**: `engine/assets/img/Glint3DIcon.png` - Original PNG logo (64x64)
- **Windows Icon**: `engine/resources/glint3d.ico` - Converted ICO format for Windows
- **Resource Script**: `engine/resources/glint3d.rc` - Windows resource definition
- **Converter Tool**: `tools/create_basic_ico.py` - Python script to convert PNG to ICO

## Process

1. **Logo Source**: The original logo is a clean geometric diamond design in PNG format
2. **ICO Conversion**: The PNG is converted to ICO format using the Python tool
3. **Resource Integration**: The ICO file is referenced in the Windows resource script
4. **Build Integration**: CMake includes the resource file in the build when using MSVC

## Windows Resource File (glint3d.rc)

```rc
#include <windows.h>

// Application icon with resource ID 101
101 ICON "glint3d.ico"

// Version information
VS_VERSION_INFO VERSIONINFO
FILEVERSION     0,3,0,0
PRODUCTVERSION  0,3,0,0
// ... version details
```

## CMake Integration

```cmake
if (WIN32)
    target_link_libraries(glint PRIVATE gdi32 user32 shell32)
    
    # Add Windows resource file for application icon and version info
    if (MSVC)
        target_sources(glint PRIVATE engine/resources/glint3d.rc)
        set_property(SOURCE engine/resources/glint3d.rc PROPERTY LANGUAGE RC)
    endif()
endif()
```

## Regenerating the Icon

If you need to update the icon:

1. Replace `engine/assets/img/Glint3DIcon.png` with your new logo
2. Run the converter: `cd tools && python create_basic_ico.py`
3. Rebuild the project: `cmake --build builds/desktop/cmake --config Release`

## Verification

To verify the icon is working:

1. Look at `builds/desktop/cmake/Release/glint.exe` in Windows Explorer
2. The icon should appear as the Glint3D diamond logo
3. When running the application, the taskbar should show the custom icon

## Technical Notes

- The ICO file contains the PNG data embedded directly (modern ICO format)
- Windows resource compiler (RC.exe) processes the .rc file during build
- The resource ID 101 is the standard application icon identifier
- Version information is also embedded for proper Windows integration

## Troubleshooting

If the icon doesn't appear:
1. Ensure you're using MSVC compiler (required for Windows resources)
2. Check that `glint3d.ico` exists in `engine/resources/`
3. Verify the build log shows the resource file being processed
4. Try clearing Windows icon cache (or restart Windows Explorer)

## Future Improvements

- Support multiple icon sizes (16x16, 32x32, 48x48, 64x64) in single ICO
- Add high-DPI icon variants
- Include icon in macOS builds using proper bundle resources
- Add installer icon for distribution packages
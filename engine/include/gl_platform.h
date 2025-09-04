// Lightweight GL include shim to support desktop (GLAD) and Emscripten (WebGL2)
#pragma once

#if defined(__EMSCRIPTEN__)
  // WebGL2 / GLES 3.0
  #ifndef GL_GLEXT_PROTOTYPES
  #define GL_GLEXT_PROTOTYPES
  #endif
  #include <GLES3/gl3.h>
  #include <GLES2/gl2ext.h>
#else
  #include <glad/glad.h>
#endif


<!-- Machine Summary Block -->
{"file":"third_party/managed/openimagedenoise/README.md","purpose":"Documents optional OpenImageDenoise drop-in structure.","exports":[],"depends_on":["third_party/managed/openimagedenoise/include","third_party/managed/openimagedenoise/lib","third_party/managed/openimagedenoise/bin"],"notes":["local_override_optional","preferred_vcpkg_source"]}
<!-- Human Summary -->
If OpenImageDenoise is not provided by your package manager or vcpkg, drop headers into `include/`, libraries into `lib/`, and runtime binaries into `bin/` so the build scripts and CMake fallback can find them.

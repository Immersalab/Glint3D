<!-- Machine Summary Block -->
{"file":"third_party/README.md","purpose":"Explains third-party dependency layout expectations.","exports":[],"depends_on":["third_party/vendored","third_party/managed"],"notes":["no_binaries_committed","vendored_layout_active","managed_optional_dependencies"]}
<!-- Human Summary -->
Third-party tree differentiates bundled source libraries from externally managed packages. Vendored headers now live under `third_party/vendored/`, while `third_party/managed/` provides optional drop-in space (e.g., GLFW, Assimp, OpenImageDenoise) when system packages are unavailable.

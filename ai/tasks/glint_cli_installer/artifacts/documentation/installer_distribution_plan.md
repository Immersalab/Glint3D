<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_cli_installer/artifacts/documentation/installer_distribution_plan.md","purpose":"Defines OS-specific packaging and PATH configuration steps for the Glint CLI installer.","exports":["windows_plan","posix_plan","validation_matrix"],"depends_on":["tools/build-and-run.bat","tools/build-and-run.sh","setup_glint_env.bat"],"notes":["path_modification_mandatory","superseded_reference_kept"]}
<!-- Human Summary -->
Historical installer blueprint describing how each platform bundles the CLI, adds the `glint` wrapper, and ensures PATH is updated so the keyword is globally invocable.

# Glint CLI Installer Distribution Plan (Superseded Reference)

Even though `glint_cli_platform_v1` replaced this task, we retain the original installer expectations for traceability. The critical requirement is that every distribution path installs the `glint` keyword by placing the wrapper in `bin/` **and** updating `PATH` automatically (or prompting the user with deterministic commands).

## Target Platforms
- **Windows (PowerShell/CMD)**
- **macOS / Linux (Bash/Zsh)**

## Windows Flow (`tools\build-and-run.bat`, `setup_glint_env.bat`)
1. **Build artifacts** – `tools\build-and-run.bat` produces `builds\desktop\cmake\<Config>\glint.exe`.  
2. **Wrapper emission** – the script creates `bin\glint.bat`, which locates the newest `glint.exe`, `cd`s to the repo root, and forwards all args.  
3. **PATH update (MANDATORY)**  
   - MSI/installer should append `<install_root>\bin` to the machine or user `Path` environment variable using `setx` or installer APIs.  
   - CLI setup scripts must also echo deterministic guidance:
     ```
     set PATH=%PATH%;<install_root>\bin
     setx PATH "%PATH%;<install_root>\bin"
     ```
   - Validation requires opening a *new* shell and confirming `where glint` resolves to the installed path.  
4. **Optional extras** – register tab-completion scripts under `%ProgramData%\glint\completions\` once PATH succeeds.

## macOS / Linux Flow (`tools/build-and-run.sh`)
1. Build via `tools/build-and-run.sh <config>` to produce `builds/desktop/cmake/<config>/glint`.  
2. Emit the POSIX wrapper (`bin/glint`) that mirrors the Windows search order.  
3. Append `export PATH="$PATH:<install_root>/bin"` into the appropriate shell profile (`~/.bashrc`, `~/.zshrc`, etc.). If the installer cannot edit profiles automatically, it must drop a snippet plus instructions and block completion until the user sources it.  
4. Provide optional shell completions in `/usr/local/share/glint/completions/` (or `$XDG_DATA_HOME/glint/completions`) once PATH is active.

## Validation Checklist
| Step | Command | Expected Result |
|------|---------|-----------------|
| PATH probe | `glint --version` from a fresh shell | Prints semantic version banner without requiring `cd` into repo. |
| Wrapper inspection | `type glint` (POSIX) / `glint /?` (Windows) | Confirms wrapper is resolving the built binary. |
| Smoke command | `glint render --help` | Non-zero exit code only on invalid usage; help text renders. |

Failure to update PATH is a release blocker: the installer must either modify PATH automatically or halt with explicit remediation steps before claiming success.

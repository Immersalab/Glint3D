# Output Directory Structure Design

**Date**: 2025-11-02  
**Purpose**: Define organization for generated artifacts separate from source code

## Target Structure

```
output/
|- README.md
|- renders/   # User render outputs (PNG, EXR)
|- exports/   # Scene/model exports (JSON, glTF)
`- cache/     # Shader cache, thumbnails, BVH
```

## Migration Notes

1. Create `output/{renders,exports,cache}`.
2. Move legacy `renders/` assets into `output/renders/`.
3. Update `.gitignore` and documentation to reference `output/renders/`.
4. Update code paths (render defaults, CLI help, UI prompts) to use `output/renders/`.
5. Remove the old `renders/` directory once all references are updated.

See `architecture_migration_plan.md` Phase 2 for detailed steps.

# Output Directory Structure Design

**Date**: 2025-11-02
**Purpose**: Define organization for generated artifacts separate from source code

## Proposed Structure

```
output/
├── README.md
├── renders/          # User render outputs (PNG, EXR)
├── exports/          # Scene/model exports (JSON, glTF)
└── cache/            # Shader cache, thumbnails, BVH
```

## Migration from renders/

1. Create output/{renders,exports,cache}
2. Move renders/*.png → output/renders/
3. Update .gitignore
4. Update code references
5. Remove old renders/

See architecture_migration_plan.md Phase 2 for detailed steps.

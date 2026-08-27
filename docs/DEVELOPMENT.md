# Development and releases

Released consumers use the public Git tag. While changing WiFiManager and a sibling dependency together, point an ignored local PlatformIO override at a `symlink://` or `file://` checkout rather than changing tracked application dependencies.

```ini
lib_deps =
    WiFiManager=symlink:///path/to/WiFiManager
```

Before a release, update `library.json`, `CHANGELOG.md`, and the relevant public documentation, then run:

```bash
./scripts/check-docs.sh
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/prepare-release.sh vMAJOR.MINOR.PATCH --tag
```

Push the branch and annotated tag. GitHub Actions repeats the board-free compile checks, validates the package, and creates a GitHub Release using that version’s changelog section. The workflow does not publish to the PlatformIO Registry.

Back to [documentation](README.md) · [project overview](../README.md).

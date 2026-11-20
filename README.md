# Character Creation Exit

An SKSE plugin that opens Skyrim's native system menu with a single **Escape**
while the character creation menu (`RaceSexMenu`) is open. If character
creation goes wrong, you can return to the main menu and start over without
closing the game.

## Usage

Press Escape while the character creation menu is open. If a message box or the
console is open, Escape is left to that menu instead. The plugin opens the
native system/pause menu over character creation without closing it. Skyrim
implements that UI internally as `Journal Menu`, the plugin selects its
`System` tab automatically. The plugin's temporary state resets when the menu
closes, so the sequence can be repeated.

## Requirements

- Skyrim SE or AE and a matching SKSE
- Address Library for SKSE Plugins
- RaceMenu is optional

## Limitations

- Only the keyboard Escape key is handled. Gamepad input is not supported.
- Skyrim VR is not supported. The plugin is built for the SE and AE runtimes only.

## Log

The game log is written to
`Documents\My Games\Skyrim Special Edition\SKSE\CharacterCreationExit.log`
(the folder name differs for other editions, for example GOG).

## Building

### Prerequisites

- Visual Studio 2026 with the "Desktop development with C++" workload
  (the presets use the `Visual Studio 18 2026` generator)
- A CMake version that supports that generator
- vcpkg, with the `VCPKG_ROOT` environment variable pointing to it
  (the vcpkg bundled with Visual Studio works)
- Git, used by CMake to fetch CommonLibSSE-NG

### Configure and build

```powershell
cmake --preset default
cmake --build --preset release   # or: --preset debug
```

The `default` preset in `CMakePresets.json` configures into `.build`, uses the
vcpkg toolchain and the `x64-windows-static` triplet. Dependencies are declared
in `vcpkg.json` (manifest mode) and are installed during the first configure.
Unless `MOD_PACKAGE_DATA_DIR` is set (see below), the DLL ends up in the
`Release` or `Debug` folder inside `.build`.

The first configure builds the vcpkg packages and the first build compiles all
of CommonLibSSE-NG, which takes a while and needs several GB of disk space in
`.build`. Later builds only recompile what changed. The `.build` folder can be
deleted at any time. After changing cached values (for example
`COMMONLIB_SOURCE_DIR`), reconfigure with `cmake --preset default --fresh`.

### CommonLibSSE-NG source

By default CMake fetches CommonLibSSE-NG at the tag set in `COMMONLIBSSE_TAG` in
`CMakeLists.txt`. To share one checkout between several projects (or to build
offline), point `COMMONLIB_SOURCE_DIR` at a local clone, either through an
environment variable of the same name or on the command line:

```powershell
cmake --preset default -DCOMMONLIB_SOURCE_DIR="C:/dev/libs/CommonLibSSE-NG"
```

When `COMMONLIB_SOURCE_DIR` is set, the pinned tag is ignored. Make sure the
checkout is compatible with this plugin's source, because CommonLibSSE-NG's API
changes between versions.

### Staging the DLL

To write the DLL and PDB straight into a mod folder, set `MOD_PACKAGE_DATA_DIR`
to that mod's `Data` directory (or its root). The files are placed in
`<directory>/SKSE/Plugins`:

```powershell
cmake --preset default -DMOD_PACKAGE_DATA_DIR="C:/path/to/mod"
cmake --build --preset release
```

For a personal setup, put the same cache variable into `CMakeUserPresets.json`
(not committed to git).

## License

The plugin statically links CommonLibSSE-NG, which is licensed under
GPL-3.0-or-later with a modding exception (see `EXCEPTIONS.md` in the
CommonLibSSE-NG repository). A distributed plugin must therefore be released
under GPL-3.0-or-later or a compatible license.
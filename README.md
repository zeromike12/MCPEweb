# Minecraft Web Build

## Dependencies

- **Git** – to clone emsdk
- **Python 3** – used by emsdk
- **PowerShell** – for build and install scripts
- **Emscripten SDK** – C/C++ to WebAssembly toolchain (installed by script)

## Install Dependencies

```powershell
.\install_deps.ps1
```

This clones emsdk into the parent directory (if missing) and installs/activates the latest toolchain.

## Build

### Windows (PowerShell)
```powershell
.\build.ps1
```

### Linux / macOS
```bash
./build.sh
```
or
```bash
python3 project/emscripten/build_web.py
```

Output: `project/emscripten/index.html`, `index.js`, `index.wasm`, `index.data`. Serve that folder with a local web server to run the game.

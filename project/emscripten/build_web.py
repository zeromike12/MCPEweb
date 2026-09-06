#!/usr/bin/env python3
import os, sys, re, subprocess, glob
from concurrent.futures import ProcessPoolExecutor, as_completed

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "../.."))
SRC_DIR = os.path.join(PROJECT_ROOT, "src")
DATA_DIR = os.path.join(PROJECT_ROOT, "data")
OBJ_DIR = os.path.join(SCRIPT_DIR, "obj")
os.makedirs(OBJ_DIR, exist_ok=True)

EMCC = "/home/user/emscripten/emcc"
EMPP = "/home/user/emscripten/em++"

COMPILE_FLAGS = [
    "-O2",
    "-std=c++17",
    "-DOPENGL_ES",
    "-DUSE_VBO",
    "-DEMSCRIPTEN",
    "-Wno-c++11-narrowing",
    "-Wno-register",
    "-Wno-unused-value",
    "-Wno-comment",
    "-sUSE_SDL=2",
    "-sUSE_LIBPNG=1",
]

LINK_FLAGS = [
    "-sASYNCIFY=1",
    "-sUSE_SDL=2",
    "-sUSE_LIBPNG=1",
    "-sLEGACY_GL_EMULATION=1",
    "-sGL_UNSAFE_OPTS=0",
    "-lopenal",
    "-lidbfs.js",
    "--js-library", os.path.join(SCRIPT_DIR, "mc_platform.js"),
    "--shell-file", os.path.join(SCRIPT_DIR, "shell.html"),
    "--preload-file", f"{DATA_DIR}@/data",
    "-sALLOW_MEMORY_GROWTH=1",
    "-sMAX_WEBGL_VERSION=2",
    "-sEXIT_RUNTIME=0",
    "-sSTACK_OVERFLOW_CHECK=0",
    "-sASSERTIONS=1",
    "-sEXPORTED_FUNCTIONS=['_main','_idbfsReady','_syncSaves','_malloc','_free']",
    "-sEXPORTED_RUNTIME_METHODS=['FS','ccall','cwrap','lengthBytesUTF8','stringToUTF8']",
    "-O2",
]

EXCLUDE_REGEX = re.compile(r"main_win|main_android|main_iOS|main_rpi|main_dedicated|AppPlatform_win32|AppPlatform_android|AppPlatform_iOS|AppPlatform_rpi|[/\\]main\.cpp|SoundSystemSL|[/\\]rhi[/\\]")

all_cpp = []
for root, _, files in os.walk(SRC_DIR):
    for f in files:
        if f.endswith(".cpp"):
            full = os.path.join(root, f)
            if not EXCLUDE_REGEX.search(full):
                all_cpp.append(full)

print(f"Found {len(all_cpp)} source files to compile.")

def compile_source(src):
    rel = os.path.relpath(src, SRC_DIR)
    obj_name = rel.replace("/", "_").replace("\\", "_") + ".o"
    obj_path = os.path.join(OBJ_DIR, obj_name)
    
    # Check modification time
    if os.path.exists(obj_path) and os.path.getmtime(obj_path) >= os.path.getmtime(src):
        return (src, obj_path, True, "")
    
    cmd = [EMPP, "-c", src, "-o", obj_path] + COMPILE_FLAGS
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        return (src, obj_path, False, res.stderr + "\n" + res.stdout)
    return (src, obj_path, True, "")

errors = []
compiled_objs = []
with ProcessPoolExecutor(max_workers=os.cpu_count() or 2) as executor:
    futures = {executor.submit(compile_source, src): src for src in all_cpp}
    done_count = 0
    for fut in as_completed(futures):
        src, obj, success, err = fut.result()
        done_count += 1
        if done_count % 20 == 0 or done_count == len(all_cpp):
            print(f"  Compiled {done_count}/{len(all_cpp)}...")
        if not success:
            errors.append((src, err))
        else:
            compiled_objs.append(obj)

if errors:
    print(f"\nCompilation failed for {len(errors)} files:")
    for src, err in errors:
        print(f"--- {src} ---")
        print(err)
    sys.exit(1)

print(f"All {len(compiled_objs)} files compiled successfully. Linking Emscripten target...")

out_html = os.path.join(SCRIPT_DIR, "index.html")
link_cmd = [EMPP] + compiled_objs + ["-o", out_html] + LINK_FLAGS
print("Running linker...")
res = subprocess.run(link_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
if res.returncode != 0:
    print("Link error:")
    print(res.stderr)
    print(res.stdout)
    sys.exit(res.returncode)

print("Build complete: index.html, index.js, index.wasm, index.data created!")

import shutil
for f in ["index.html", "index.js", "index.wasm", "index.data", "manifest.json", "sw.js", "mc_platform.js"]:
    src_f = os.path.join(SCRIPT_DIR, f)
    if os.path.exists(src_f):
        shutil.copy2(src_f, os.path.join(PROJECT_ROOT, f))
print("Synchronized playable game build to repository root.")

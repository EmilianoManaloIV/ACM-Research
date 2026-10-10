# ACM-Research

AI-driven research into novel ways to solve problems in software engineering, computer graphics, and human-computer interaction.

The repository holds three kinds of work side by side: notes on related papers, self-contained experiments, and a small shared C++/OpenGL codebase the experiments build on.

## Layout

```
papers/        Related-work notes, one Markdown file per paper (start from papers/_template.md)
experiments/   One dated folder per experiment with its hypothesis, method and results
src/acm/       Shared C++ library: window and OpenGL context, shaders, framebuffer capture
src/acm/skin/  Skinning research code: rigged glTF loading, linear blend and dual quaternion skinning, weight transfer, metrics
apps/sandbox/  Starter app: a lit, rotating cube to copy for new rendering experiments
apps/weightbench/  Scores a weight-transfer method on a garment over a set of poses
tools/         Python helpers: synthetic benchmark data, reference checks, result tables
tests/data/    Small meshes used by the checks (Epic's sphere-to-plane example, MIT)
shaders/       GLSL shaders, loaded from disk at runtime so they can be edited without rebuilding
third_party/   Vendored code (the glad OpenGL loader)
data/          Datasets and outputs, ignored by git
```

## Building

Requirements: CMake 3.20+, a C++17 compiler, Git, and a GPU driver with OpenGL 3.3. CMake downloads GLFW 3.4, GLM 1.0.1, libigl 2.5 (with Eigen) and tinygltf 2.9 the first time it configures.

On Linux, install the X11 and OpenGL headers first. On Debian or Ubuntu:

```sh
sudo apt install build-essential cmake git libgl-dev libx11-dev libxrandr-dev \
  libxinerama-dev libxcursor-dev libxi-dev libxkbcommon-dev
```

macOS needs the Xcode command line tools; Windows needs Visual Studio 2019 or newer with the C++ workload.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Running the sandbox

```sh
./build/apps/sandbox/sandbox            # interactive window, Esc to quit
./build/apps/sandbox/sandbox --frames 60 --screenshot cube.ppm --hidden
```

The second form renders a fixed number of frames with a fixed timestep and saves the last one, which keeps figures reproducible. On Windows the binary is under `build/apps/sandbox/Release/`. On a machine without a display, wrap the command in `xvfb-run -a`.

## Running the weight-transfer benchmark

The Python tools need `pip install numpy scipy libigl pygltflib`.

```sh
python3 tools/make_synthetic.py                      # bodies, garments and poses into data/synthetic/
tools/run_benchmark.sh data/results                  # every method on every garment, held-out poses
python3 tools/summarize_results.py data/results/results.csv
python3 tools/verify_inpaint.py build/apps/weightbench/weightbench   # C++ inpainting vs Epic's Python
```

`weightbench --help` style usage is at the top of `apps/weightbench/main.cpp`. New transfer methods go in `src/acm/skin/transfer.cpp` behind `runTransfer`, and each gets a dated folder in `experiments/` with its results table. Methods so far: `closest`, `inpaint` (Epic 2023), `bounded`, `anchored`, `relative`, `consistent`, `offsetblend`, `jointcap`, `lengthcap`, `bodyblend`, `kinblend` (current default: offset blend at scale 4, limited to bones within two hierarchy steps), each building on the last; `--refine` refits any of them against the training poses (M6); `--hem-rig hem.json` adds leg-driven hem bones for skirts and dresses (`src/acm/skin/hem.h`). `--skinning lbs|dqs` picks the skinning model for scoring (`tools/run_benchmark.sh out all dqs`). `ctest --test-dir build` runs the skinning check. `python3 tools/make_makehuman.py` fetches the MakeHuman body (CC0) and `tools/run_benchmark.sh out all lbs makehuman` scores it.

## Working in this repo

- **Reading a paper:** copy `papers/_template.md` to `papers/YYYY-firstauthor-short-title.md` and add a row to the index in `papers/README.md`.
- **Starting an experiment:** copy `experiments/_template/` to `experiments/YYYY-MM-DD-short-name/`, write the hypothesis before running anything, and log it in `experiments/README.md`.
- **New rendering prototype:** copy `apps/sandbox/` to a new folder under `apps/`, rename the target in its `CMakeLists.txt`, and add it with `add_subdirectory` in the root `CMakeLists.txt`. Move code into `src/acm/` once a second app needs it.
- **Data:** keep large files in `data/` and record where they came from in `data/README.md`.
- **Research workflow:** questions, evidence and decisions live in `research/`; each experiment has a protocol written before it runs and a review by a separate session. [`CLAUDE.md`](CLAUDE.md) describes the stages and the approval gates.

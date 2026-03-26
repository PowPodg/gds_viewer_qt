# GDS Viewer

### Purpose
`gds_viewer` is a desktop application built with **Qt 6 / C++20** for viewing and basic analysis of **GDSII** layout files.

The application opens a file, builds a scene from layout elements, and provides layer-based visualization controls: enabling and disabling layers, changing layer colors, viewing element counts, and inspecting properties of selected objects.

---

### Features

- open layout files via **File → Open**;
- display geometry in `QGraphicsView`;
- zoom with the mouse wheel and use **Fit** to show the entire scene;
- layer table with:
  - visibility on/off,
  - color selection,
  - element count per layer;
- apply visibility and color changes to multiple rows using **Shift + checkbox click** and **Shift + click on a color cell**;;
- background data preparation and incremental scene construction;
- progress indicator and loading cancellation;
- display service information in the status bar:
  - library name,
  - structure name,
  - number of elements,
  - information about the selected object;
- view element properties via the context menu;
- support selection of objects even at points where multiple elements overlap.

---

### What Is Displayed

The viewer works with layout elements produced by the parser and displays, among others:

- `BOUNDARY`
- `BOX`
- `PATH`
- `TEXT`
- `NODE`
- `SREF`
- `AREF`

---

### Architecture

The project consists of the `gds_viewer` GUI part, which:

- opens a file;
- starts background data preparation;
- adds elements to the scene incrementally;
- synchronizes the scene with the layer table;
- shows properties of selected objects.

The main architectural feature of the current version is the separation of work into two stages:

1. **background `parse/prepare`** — file reading and data preparation outside the GUI thread;
2. **cooperative batched scene construction (batched GUI build with `co_await nextGuiCycle(...)`)** — elements are added to the scene in portions using `co_await nextGuiCycle(...)` so that the interface remains responsive while loading “large” layouts.

As a result, the application does not try to build the entire scene in one long blocking pass: data preparation is performed separately, while GUI updates happen gradually, with support for progress reporting and cancellation.

Main components:

- `MainWindow` — main application window, menu, status bar, layer table, and loading start;
- `GdsSceneView` — scene and layout element visualization;
- `ElementPropertiesDialog` — dialog for properties of the selected element;
- `CoroutinePipeline.hpp` — coroutine-based pipeline for background preparation and staged GUI updates;
- `PreparedSceneData.hpp` — intermediate data structure for batched scene construction.

---

### Requirements

- **C++20**
- **CMake**
- **Qt 6**
  - `Widgets`
  - `Concurrent`
- for tests:
  - `Qt6::Test`

The project also uses the `gds_core` library as a build dependency.

---

### Build

Example build with CMake:

```bash
cmake -S . -B build
cmake --build build

```

<details>
<summary><strong>Project Structure</strong></summary>

```text
gds_viewer/
├─ CMakeLists.txt
├─ main.cpp
├─ MainWindow.hpp
├─ MainWindow.cpp
├─ MainWindow.ui
├─ GdsSceneView.hpp
├─ GdsSceneView.cpp
├─ ElementPropertiesDialog.hpp
├─ ElementPropertiesDialog.cpp
├─ PreparedSceneData.hpp
├─ CoroutinePipeline.hpp
├─ HoverPolygonItem.hpp
├─ HoverPolygonItem.cpp
├─ HoverPathItem.hpp
├─ HoverPathItem.cpp
└─ tests/
   ├─ CMakeLists.txt
   └─ ViewerSmokeTests.cpp
```

</details>

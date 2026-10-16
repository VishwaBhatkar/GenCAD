# GenCAD 4.0 — User Manual
**Professional Desktop 2D CAD Workstation (AutoCAD Edition)**

---

## 1. Introduction

**GenCAD** is a production-level 2D Computer-Aided Design (CAD) desktop application built with **C++17**, **Qt 6**, and **CMake**. Inspired by the modern AutoCAD interface, GenCAD features an authoritative CAD document model, interactive drawing tools with real-time feedback, an AutoCAD-style ribbon bar, dynamic full-viewport crosshairs, an intelligent command line with auto-completion, dockable layer and properties managers, cloud project collaboration, and an autonomous AI engineering copilot.

---

## 2. Workspace Layout & Interface

```
+------------------------------------------------------------------------------------+
| Menu Bar: File  Edit  View  Draw  Modify  Tools  Cloud  AI  Theme  Help             |
+------------------------------------------------------------------------------------+
| Ribbon Tabs: [ Home ] [ Annotate ] [ View ] [ Cloud ] [ AI Assistant ]     [▲]      |
| +--------------------------------------------------------------------------------+ |
| | Draw | Modify | Annotation | Layers | Snaps & Modes | Clipboard |                | |
| +--------------------------------------------------------------------------------+ |
+------------------+-----------------------------------------------+-----------------+
| Properties Dock  | Central Drafting Canvas (CadView)             | Layers Manager  |
|                  | - Infinite Crosshairs & Pickbox               | - Search Filter |
| - General        | - Adaptive Grid & UCS Icon (X/Y)              | - Layer Table   |
|   - Layer        | - Viewport HUD: [-][Top][2D Wireframe]        | - Color Swatch  |
|   - Color        | - Nav Overlay: [Extents] [+] [-] [1:1] [Pan]  | - On/Off / Lock |
|   - Lineweight   | - Window (Blue) / Crossing (Green) Selection  +-----------------+
| - Geometry       | - Model / Layout 1 Space Tabs                 | AI Assistant    |
|   - Coordinates  |                                               | - Prompt Gen    |
|   - Dimensions   |                                               | - Auto-Dim      |
| - Measurements   |                                               | - Analyzer      |
+------------------+-----------------------------------------------+-----------------+
| Command Line & History Transcript:                                                 |
| [▲] Command [LINE]: Specify next point or [Undo]:                                  |
+------------------------------------------------------------------------------------+
| X: 125.40 mm  Y: 82.50 mm  Z: 0.00 mm | Tool: Line | [GRID] [SNAP] [ORTHO] [POLAR] |
+------------------------------------------------------------------------------------+
```

### Key UI Sections:
1. **Menu Bar:** Standard hierarchical menus with keyboard shortcuts and SVG icons.
2. **AutoCAD Ribbon (`CadRibbon`):** Tabbed interface with large and small tool groupings for drafting, modifying, and managing drawing data. The ribbon can be collapsed to save screen space using the upper-right `▲` button.
3. **Central Drafting Canvas (`CadView` & `CadScene`):** High-performance vector canvas featuring infinite crosshairs, adaptive level-of-detail (LOD) grid, origin UCS axes, live entity grips, and floating viewport controls.
4. **Properties Palette (`PropertiesDock`):** Dockable inspector on the left. Displays and modifies properties for selected objects, whole document summaries, or multi-selected entity batches.
5. **Layers Manager (`LayerWidget`):** Dockable panel on the right. Search, add, delete, rename, lock, hide, and color-code CAD drawing layers.
6. **AI Assistant Dock (`AiAssistantDock`):** Integrated CAD copilot tabbed with the layers dock for generative modeling and geometric validation.
7. **Command Line & History (`CadCommandLine`):** Dynamic multi-line command terminal with auto-completion, command history recall, and numeric entry fields.
8. **Configurable Status Bar:** Live coordinate readout, tool indicator, selection counter, layer badge, interactive drafting toggle buttons (`GRID`, `SNAP`, `ORTHO`, `POLAR`), engineering units switcher, theme switcher, and cloud status pill.

---

## 3. Drawing Tools (Draw Group)

| Tool | Shortcut / Alias | Description | How to Use |
| :--- | :--- | :--- | :--- |
| **Select** | `Esc`, `SEL`, `SELECT` | Pick single objects or drag selection boxes. | Click entity or drag left-to-right (Window) or right-to-left (Crossing). |
| **Line** | `L`, `LINE` | Contiguous straight line segments. | Click start point, click next point (or type distance in command line). Press `Enter` or `Esc` to end. |
| **Polyline** | `PL`, `PLINE`, `POLYLINE` | Connected 2D multi-segment paths. | Click consecutive points. Press `Enter` to finish, or type `C` to close back to the start. |
| **Circle** | `C`, `CIRCLE` | Circles defined by center and radius. | Click center point, drag to set radius (or type exact radius), and click to confirm. |
| **Rectangle** | `REC`, `RECT`, `RECTANGLE` | Rectangular boundaries. | Click first corner, drag to opposite corner (or enter exact width and height), click to place. |
| **Arc** | `A`, `ARC` | 3-parameter circular arc segments. | Click center point, drag radius and start angle, sweep to end angle. |
| **Polygon** | `POL`, `POLYGON` | Equilateral regular polygons (3 to 64 sides). | Enter number of sides, click center point, drag radius and orientation. |
| **Ellipse** | `EL`, `ELLIPSE` | Ellipses with major and minor axes. | Click center point, drag major radius along axis, drag minor radius. |
| **Text** | `T`, `TEXT`, `MTEXT` | Engineering text annotations. | Click insertion position, enter text string in the popup dialog. |
| **Dimension** | `DIM`, `DIMALIGNED` | Linear aligned CAD dimension lines. | Click first reference point, click second reference point, pull out dimension line position. |

---

## 4. Editing & Modification Tools (Modify Group)

All editing operations integrate directly into the multi-level **Undo/Redo stack** (`Ctrl+Z` / `Ctrl+Y`).

| Tool | Shortcut / Alias | Description | How to Use |
| :--- | :--- | :--- | :--- |
| **Move** | `M`, `MOVE` | Displaces selected entities by a vector. | Select entities, click base point, click target destination point. |
| **Copy** | `CO`, `CP`, `COPY` | Duplicates selected entities. | Select entities, click base point, click placement point. |
| **Rotate** | `RO`, `ROTATE` | Rotates entities around a pivot center. | Select entities, click rotation center, move mouse to specify angle (or type degrees). |
| **Mirror** | `MI`, `MIRROR` | Reflects entities across a symmetry axis. | Select entities, pick first point on mirror line, pick second point on mirror line. |
| **Offset** | `O`, `OFFSET` | Concentric / parallel duplicate at offset distance. | Specify offset distance, click source entity, click side to place offset. |
| **Trim** | `TR`, `TRIM` | Cuts geometry at intersecting boundary edges. | Click cutting boundary entity, then click segment to trim away. |
| **Extend** | `EX`, `EXTEND` | Extends lines to boundary edges. | Click boundary entity, then click line to extend toward boundary. |
| **Delete** | `Del`, `E`, `ERASE` | Removes selected entities from drawing. | Select entities and press `Delete` key. |

---

## 5. Precision Drafting & Snaps

GenCAD features an intelligent snapping engine and directional constraints that operate simultaneously:

### 1. Object Snap (OSNAP) — `F3`
- Magnetic snapping to critical geometric landmarks:
  - **Endpoint ($\square$):** Line ends, arc ends, rectangle corners, polygon vertices.
  - **Center ($\odot$):** Circle centers, arc centers, ellipse centers.
  - **Midpoint ($\triangle$):** Line midpoints, arc midpoints.
  - **Quadrant ($\lozenge$):** Top, bottom, left, and right extremes of circles.
  - **Intersection ($\times$):** Where entities intersect.

### 2. Grid Snap — `F9`
- Locks cursor movement to discrete grid spacing intervals configured in Page Setup.

### 3. ORTHO Mode — `F8`
- Restricts drawing and moving tools strictly to horizontal ($0^\circ, 180^\circ$) and vertical ($90^\circ, 270^\circ$) axes.

### 4. Polar Tracking — `F10`
- Guides cursor movement along user-specified angular increment rays ($15^\circ, 30^\circ, 45^\circ, 90^\circ$) with magnetic alignment lines.

### 5. Direct Numerical / Exact Dimension Entry
- When drawing any primitive, the dynamic fields in the command line display live length, angle, width, radius, or coordinates.
- Press **Tab** to switch between input fields.
- Press **Enter** to commit exact dimensions.

---

## 6. Selection Techniques

GenCAD faithfully implements AutoCAD's signature dual-selection system:

### 1. Direct Click Selection
- Click directly on any line, circle, arc, or text to select it.
- Hold **Shift** or **Ctrl** while clicking to add or remove entities from the selection set.

### 2. Window Selection (Left-to-Right Drag)
- Click empty canvas and drag from **left to right**.
- Renders as a **semi-transparent blue box with a solid blue border**.
- **Rule:** Only entities completely enclosed inside the rectangle are selected.

### 3. Crossing Selection (Right-to-Left Drag)
- Click empty canvas and drag from **right to left**.
- Renders as a **semi-transparent green box with a dashed green border**.
- **Rule:** All entities touched or crossed by the rectangle boundary are selected.

### 4. Entity Grips
- Selected entities display cyan square grips ($\square$).
- Grips indicate entity control vertices (endpoints, centers, quadrant points, midpoints).

---

## 7. Layers Management System

Access via the **Layers Palette** (`Ctrl+2` or View $\rightarrow$ Layers) or the active layer dropdown in the Ribbon.

- **Active Layer:** Entities drawn will automatically belong to the current active layer. The active layer is indicated with a green checkmark ($\checkmark$) in the table.
- **Search Filter:** Type in the top filter bar to instantly locate layers in complex engineering drawings.
- **Layer Attributes:**
  - **Name:** Double-click any layer name to rename it (Layer `0` is protected).
  - **Color:** Click the color swatch to open the full color palette dialog. Entities on layer set to ByLayer inherit this color.
  - **Visibility ($\odot$):** Click the lightbulb icon to toggle layer visibility on/off. Hidden layers are not rendered on the canvas.
  - **Lock ($\theta$):** Click the padlock icon to lock/unlock layers against accidental editing.
  - **Object Counter:** Displays the real-time count of entities assigned to each layer.
- **Context Menu:** Right-click any row to *Set Current*, *Change Color*, *Toggle Visibility*, *Toggle Lock*, *Select All Objects on Layer*, or *Delete Layer*.

---

## 8. Properties Palette (Object Inspector)

Access via `Ctrl+1`, the Ribbon View tab, or right-click canvas context menu.

### Single Entity Inspection
When an entity is selected, the palette displays:
- **General Properties:**
  - **Layer:** Dropdown listing all layers. Selecting a layer immediately moves the entity to that layer with full Undo support.
  - **Color:** Interactive button showing the current hex/RGB swatch. Click to change.
  - **Stroke Width (Lineweight):** Double spinbox adjusting stroke thickness in millimeters.
- **Geometry Properties:**
  - Coordinates ($X, Y$), lengths, angles, radii, diameters, width, height, vertex counts.
  - Double-click any numerical value to edit it in place; the canvas updates instantly.
- **Measurement Properties:**
  - Real-time calculated Area, Perimeter, Circumference, Arc Length, and delta measurements.

### Multi-Selection Batch Editing
When multiple entities are selected:
- Displays total entity count, total combined length/perimeter, and total enclosed area.
- Allows **batch layer reassignments**, **batch color changes**, and **batch stroke width adjustments** for all selected entities in a single step.

---

## 9. Command Line & Auto-Completion

The command line at the bottom operates as a full AutoCAD terminal:

- **Auto-Completion:** Type one or two letters to view matching command suggestions. Press **Tab** or **Enter** to accept.
- **Command History:** Press **Up Arrow ($\uparrow$)** or **Down Arrow ($\downarrow$)** to recall previously executed commands.
- **Expand Transcript:** Click the **$\blacktriangle$ / $\blacktriangledown$** button on the left to expand the history log to view past outputs, lengths, and prompts.

### Command Aliases Table

| Alias | Command | Alias | Command |
| :--- | :--- | :--- | :--- |
| `L` | `LINE` | `M` | `MOVE` |
| `PL`, `PLINE` | `POLYLINE` | `RO` | `ROTATE` |
| `C` | `CIRCLE` | `MI` | `MIRROR` |
| `REC`, `RECT` | `RECTANGLE` | `CO`, `CP` | `COPY` |
| `A` | `ARC` | `O` | `OFFSET` |
| `POL` | `POLYGON` | `TR` | `TRIM` |
| `EL` | `ELLIPSE` | `EX` | `EXTEND` |
| `T`, `MTEXT` | `TEXT` | `E`, `DEL` | `DELETE` |
| `DIM` | `DIMENSION` | `Z`, `FIT` | `ZOOM FIT` |
| `U` | `UNDO` | `REDO` | `REDO` |
| `ORTHO` | Toggle Ortho mode | `POLAR` | Toggle Polar mode |
| `SNAP`, `OSNAP`| Toggle Object Snap | `GRID` | Toggle Grid Snap |
| `PR` | Open Properties | `LA` | Open Layers |
| `LIMITS` | Page Setup / Limits | `AI` | Open AI Assistant |

---

## 10. Autonomous AI CAD Assistant

Access via `Ctrl+Shift+A`, the **AI Assistant** tab in the Ribbon, or the right-side dock.

### 1. Natural Language Prompt Generation
- Type engineering descriptions such as:
  - *"Draw a mechanical flange of diameter 120mm with 6 bolt holes of diameter 10mm on a 90mm pitch circle"*
  - *"Create an L-bracket 80x50mm with 4mm thickness"*
- Click **Generate Proposal**. The AI synthesizes CAD DSL operations, validates geometric constraints, and presents a staged proposal for user approval.
- Click **Apply to Drawing** to commit the geometry into the document with full Undo support.

### 2. Auto-Dimensioning
- Click **Auto-Dimension Drawing** or select specific entities and click **Dimension Selected**.
- The AI analyzer calculates optimal offset witness lines, prevents overlap, and dimensions key geometry automatically.

### 3. Geometry Analyzer & Auto-Clean
- **Analyze Geometry:** Scans drawing for disconnected polyline vertices, duplicate overlapping line segments, unclosed contours, and zero-length primitives.
- **Auto-Clean:** Automatically deduplicates overlapping collinear lines and closes non-manifold gaps.

### 4. Engineering Standards RAG
- Search engineering standards (ISO, ASME, DIN, thread tolerances, surface finishes) directly inside the AI panel.

---

## 11. Cloud Collaboration & Workspace

Access via `Ctrl+Shift+C` or the **Cloud** tab in the Ribbon.

- **Authentication:** Register an account or log in with your credentials to connect to the GenCAD Cloud Server (`http://127.0.0.1:8000`).
- **Projects:** Create, organize, and manage CAD projects.
- **Version Control:** Save drawings to the cloud with automatic revision increments and commit messages. Review complete version histories and revert or download prior revisions.

---

## 12. File Formats & Interoperability

### Native `.gencad` Format
- Standard JSON schema storing document metadata, units, page limits, layers, and geometry definitions with persistent UUIDs.
- Fully portable across platforms.

### DXF Export & Import (AutoCAD Compatibility)
- **Export DXF (`Ctrl+E`):** Exports lines, circles, arcs, polylines, text, and dimensions to industry-standard AutoCAD DXF files (ASCII R12/R2000).
- **Import DXF (`Ctrl+I`):** Imports existing 2D DXF drawings into GenCAD.

---

## 13. Keyboard Shortcuts Reference

| Shortcut | Function | Shortcut | Function |
| :--- | :--- | :--- | :--- |
| **Ctrl + N** | New Drawing | **F3** | Toggle Object Snap (OSNAP) |
| **Ctrl + O** | Open Drawing | **F8** | Toggle ORTHO Mode |
| **Ctrl + S** | Save Drawing | **F9** | Toggle Grid Snap |
| **Ctrl + Shift + S** | Save Drawing As | **F10** | Toggle Polar Tracking |
| **Ctrl + Z** | Undo | **Delete** | Delete Selected Objects |
| **Ctrl + Y** | Redo | **Esc** | Cancel Active Tool / Clear Selection |
| **Ctrl + F** | Zoom Extents (Fit) | **Space + Drag** | Pan Drawing Canvas |
| **Ctrl + +** | Zoom In | **Wheel Scroll** | Cursor-anchored Zoom |
| **Ctrl + -** | Zoom Out | **Middle Click Drag** | Pan Canvas |
| **Ctrl + 1** | Toggle Properties Palette | **Ctrl + Shift + A** | Toggle AI Assistant Panel |
| **Ctrl + 2** | Toggle Layers Manager | **Ctrl + Shift + C** | Open Cloud Workspace |
| **Ctrl + E** | Export DXF | **Ctrl + Shift + P** | Page Setup / Drawing Limits |
| **Ctrl + I** | Import DXF | **Ctrl + A** | Select All Entities |

---

## 14. Quick Start: Launching GenCAD

To run GenCAD on Windows:

```cmd
:: 1. Navigate to the project directory
cd c:\Users\vishw\OneDrive\Desktop\GenCAD

:: 2. Launch using the all-in-one launcher (starts FastAPI cloud backend + GenCAD)
run_gencad.bat

:: Or launch the compiled release executable directly
.\build-release\gencad.exe
```

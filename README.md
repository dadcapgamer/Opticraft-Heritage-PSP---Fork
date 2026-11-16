# ATENCION: ARCHIVO assets.pak OBLIGATORIO

# ES TOTALMENTE NECESARIO CONTAR CON EL ARCHIVO assets.pak PARA QUE EL JUEGO ARRANQUE SIN ERRORES

## Se puede encontrar en la comunidad de Discord oficial disponible en:
## https://ocheritage.vercel.app/

***

# ATTENTION: assets.pak FILE REQUIRED

# IT IS ABSOLUTELY REQUIRED TO HAVE THE assets.pak FILE FOR THE GAME TO LAUNCH WITHOUT ERRORS

## It can be found in the official Discord community available at:
## https://ocheritage.vercel.app/

***

# OptiCraft Heritage Edition: PlayStation Portable (PSP)

Read this in English | Leer en Español

# English

## Overview

OptiCraft Heritage Edition (PSP Port) is an optimized, clean room C++17 port of classic Minecraft gameplay specifically crafted for the Sony PlayStation Portable (PSP) and the PPSSPP emulator.

Built directly against the bare metal PSPSDK and Allegrex MIPS architecture, this port brings the authentic classic sandbox experience to Sony legendary handheld while strictly respecting its tight hardware limitations (MIPS R4000 at 333 MHz, 32MB / 64MB RAM).

## Features and Optimizations

**PS2 Parity 2D Heightmap World Generator**
Uses the lightweight 2D heightmap generator (ChunkProviderGenerateLite) and fast biome sampling (WorldChunkManagerFast), mirroring the generation of the PlayStation 2 edition. Replaces heavy 3D double precision Perlin noise with single precision float mathematics for ultra fast chunk generation on MIPS without CPU stalls.

**Optimized 256x256 Bounded Worlds**
Authentic console sized worlds (16x16 chunks) with seamless edge physics. Outward velocity decomposition ensures the player can smoothly slide along world borders and walk back towards the center with zero friction, eliminating the classic stuck sensation.

**Custom Dual Mode Control Layout**
Gameplay Mode: Smooth analog nub movement, responsive face button camera look with exponential ramp curves, quick hotbar cycling, and dedicated trigger actions.
Menu Mode: Dual navigation featuring both intuitive D Pad grid jumping and a fast analog virtual pointer for inventories and crafting.

**Auto Jump Built In**
Automatic single block elevation step up enabled by default, complete with an in game toggle in Heritage Options and Host Options.

**Fixed Function 16 bit Depth Optimization**
Engineered specifically for the PSP Graphic Engine (GE) to avoid coplanar Z fighting in 2D overlays, preserving vibrant full hearts, hunger icons, and pixel perfect button glyphs.

**Memory Footprint and Chunk Streaming**
Configured memory ring buffers and chunk load ceilings to guarantee rock solid stability on both retail hardware and emulators.

## Controller Layout

### Gameplay Controls

Analog Stick: Walk / Strafe (360° Movement)
△ (Triangle): Look Up
✕ (Cross): Look Down
□ (Square): Look Left
○ (Circle): Look Right
R Trigger: Primary Action (Attack / Mine Block)
L Trigger: Secondary Action (Use / Place Block)
D Pad Up: Jump
D Pad Down: Sneak / Crouch
D Pad Left / Right: Hotbar Item Selection (Previous / Next)
START: Pause Menu / Host Options
SELECT: Inventory / Crafting Menu
R + SELECT: Open Host Options Directly

### Menu and Inventory Controls

D Pad: Navigate UI buttons and inventory slots
Analog Stick: Move virtual pointer cursor
✕ (Cross): Select / Confirm / Left Click
□ (Square): Split Stack / Right Click
△ (Triangle): Quick Move (Shift + Click)
○ (Circle): Back / Cancel / Exit Menu
L / R Triggers: Scroll Container / Change Creative Tabs

## Installation

### 1. Requirements
A PlayStation Portable (PSP 1000, 2000, 3000, Go, or Street) running custom firmware (PRO, ME, or ARK4), a PS Vita with Adrenaline, or the PPSSPP emulator.
Note: On PSP 1000 (32MB RAM), limited 256x256 worlds are strongly recommended.

### 2. File Placement
Copy the game folder to your Memory Stick under PSP/GAME/:

```text
ms0:/
 └── PSP/
      └── GAME/
           └── OptiCraft/
                ├── EBOOT.PBP
                └── assets.pak
```

**PPSSPP Path (Windows)**:
C:\Users\<YourUser>\Documents\PPSSPP\PSP\GAME\OptiCraft\

**PPSSPP Path (Android)**:
/storage/emulated/0/PSP/GAME/OptiCraft/

## Compiling from Scratch

You can compile OptiCraft for PSP using Windows (WSL2), Linux, or Docker.

### Building on Windows or Linux

1. From Windows in the project directory, run:
   "build psp.bat"

2. Or inside WSL / Linux terminal, run:
   bash ./build_psp.sh

The resulting EBOOT.PBP will be located in the project root and in build/psp/bin/psp/EBOOT.PBP.

### PSP performance capture with PSPLINK

The normal PSP build writes `debug.log` to the game directory. For hardware
profiling, build a symbol-preserving remote-debug image instead:

```bash
BUILD_PRX=ON PSP_REMOTE_DEBUG=ON PSP_BUILD_TYPE=Debug MC_LOG_LEVEL=2 bash ./build_psp.sh
```

This produces `OptiCraft.prx` for launching from `pspsh`, while retaining
`OptiCraft.elf` as the symbol file for `psp-gdb` and `psp-addr2line`. With
PSPLINK connected, the build sends the logger to PSPLINK stdout rather than the
Memory Stick. It emits a sample during the first five frames and then once
every 60 frames containing total frame time, tick time, render time, chunk
updates, and free/max contiguous memory. Keep this mode out of normal release
builds: remote output is intentionally synchronous and changes the timing being
measured.

At log level 2, PSP samples also include render phases (`skyMs`, `frustumMs`,
`buildMs`, `opaqueMs`, `entsMs`, `transMs`, `handMs`, and `hudMs`) plus the
loaded/rendered renderer counts and renderer grid dimensions. Phase samples are
reset every frame and only printed on the existing periodic frame samples.
The same samples include the essential chunk scheduler and mesh counters:
`meshPending`, `meshDone`, `meshVerts`, and `meshBuildMs`. On PSP, `meshBuildMs` measures
the complete synchronous renderer update, including display-list submission;
the current backend does not expose a separate GPU upload timer.
The additional `meshScanMs`, `meshGeomMs`, and `meshDisplayMs` fields split that
time into section traversal/block access, block geometry generation, and
display-list submission. Texture capture was measured separately but is below
one millisecond in the observed captures and is omitted from the compact line.
The compact line also reports `meshLoop`, `meshReads`, `meshLights`,
`meshRenders`, `meshStandard`, and `meshSpecial`, which count section-loop
iterations, block reads, lighting queries, block render dispatches, standard
cube render dispatches, and non-standard render dispatches. `meshVerts` is the
number of vertices accumulated by the section build. `meshHidden` and
`meshVisible` count full opaque cubes skipped because all six neighboring
blocks are opaque, and full opaque cubes that still reached geometry
generation, respectively. PSP mesh `ChunkCache` instances also reuse block IDs
for the small section volume and its one-block margin, reducing repeated
chunk/section lookups while preserving the existing behavior for larger
non-mesh cache spans. `meshCacheHits` and `meshCacheMisses` report cache
lookups directly; they are separate from `meshReads`, which continues to count
all `getBlockId()` calls, including cache hits. Special render dispatches are
split into `meshFluid`, `meshPlants`, `meshStairs`, and `meshOtherSpecial`.
The PSP mesh loop caches each block's render type for classification and
dispatch telemetry, avoiding repeated virtual lookups without changing the
rendering path.
For standard opaque PSP cubes, the section builder computes the six-face
visibility mask once and reuses it during standard/AO geometry generation.
The prepared full-cube bounds are also reused for that dispatch, avoiding a
duplicate state setup call. These shortcuts preserve the existing bounds,
lighting, and render-type paths. The PSP standard opaque-cube dispatch also
reuses its classification instead of repeating an opacity virtual call.
PSP standard-block AO reuses a generation-stamped scratch cache for its
27-neighbour brightness, packed-light, and block-ID workspaces. This removes
per-block array initialization while preserving the same cache contents and
AO calculations.
For PSP fluid meshes, a metadata-zero source block with source-fluid
neighbours on all four horizontal sides skips the flow-vector traversal and
angle calculation because its horizontal flow is provably zero. Partial,
flowing, and coastline fluid blocks retain the existing path.
The PSP frame sample also reports `meshDirtyMarks` and
`meshDirtyAlreadyPending`, which count renderer invalidations and invalidations
received after a renderer was already marked for rebuild. `meshDirtyDuringBuild`
remains zero on the current PSP synchronous section-builder path because it
has no resumable build state; it is reserved for incremental build paths.
It also reports invalidation source buckets (`meshDirtyBlockChanges`,
`meshDirtyLighting`, `meshDirtyStreaming`, and `meshDirtyOther`), the oldest
queued renderer age and nearest/farthest queued squared distances, and
per-category geometry time (`meshStandardMs`, `meshFluidMs`, `meshPlantMs`,
`meshStairMs`, and `meshOtherSpecialMs`). Category timers include the existing
render dispatch and are intentionally sampled for every rendered block, so they
should be compared between similar captures rather than treated as zero-cost
measurements.
At PSP log level 4, the remote sink also emits the `worldload` category. Its
world-load trace reports the durations of world creation, old-world saving,
preload columns, lighting, player setup, and spawn-chunk loading without
logging individual columns.
The PSP scheduler examines up to twelve queued candidates per frame and gives
queued sections a capped age bonus after they have waited, while retaining
urgent, in-frustum, and distance priorities. This improves queue fairness
without raising the one-section-per-frame mesh budget or the cost of a
completed section build.
Queued PSP sections older than eight seconds enter a starvation tier after
urgent and active work but before normal frustum and distance ranking. This
prevents distant non-urgent sections from waiting indefinitely during
streaming, while retaining the one-section-per-frame limit and the bounded
age bonus for newer work.
The split mesh line also includes scheduler candidates/attempts, completed
steps, terrain passes, vertex totals, block-read/cache totals, category counts,
and all mesh timing totals. The queue line includes the free-memory high-water
value and render-state diagnostics. Together these lines are the complete
periodic PSP mesh capture; no per-block logging is needed. The frame line emits
`telemetry=4` so a capture can be rejected immediately if an older PRX is still
running.
Fluid telemetry reports `fluidHeightQueries`, `fluidVisibleFaces`, and
`fluidSkippedFaces` to distinguish fluid-neighbour analysis from emitted
surface faces. On PSP, same-material fluid neighbours are rejected before the
full face-culling path; boundary and special-neighbour checks retain the
existing behavior. Fully enclosed fluids return before tinting or height
queries, and a fluid block whose only visible face is the bottom skips all
corner-height searches because that face does not use them. PSP frame telemetry
also reports `translucentMs` and
`translucentDrawLists`, plus `colorMaskCalls`. PSP uses a single translucent
water pass even when fancy water is enabled, avoiding the unsupported
depth-peeling state sequence that caused severe underwater stalls. Color-mask
operations are ignored on PSP and counted diagnostically because the PSP
OpenGL backend reports `GL_INVALID_ENUM` for that state operation. PSP
telemetry also reports `renderStateErrors`, `renderStateLastOperation`,
`renderStateLastValue`, and `renderStateLastError`; the operation IDs are enable=1, disable=2,
blendFunc=3, depthMask=4, and shadeModel=5.
PSP fluid corner heights are also cached for the lifetime of a section mesh
build. Adjacent visible fluid blocks commonly ask for the same corner multiple
times; the cache reuses the exact previously computed value without changing
the fluid metadata or face-visibility rules. Full-fluid fast results are cached
as well, so repeated samples in a dense column do not repeat the early
neighbour check.
For blend-function diagnostics, `renderStateLastValue` packs the source factor
in the high nibble and the destination factor in the low nibble, using the
`RenderBlendFactor` enum order.
PSP also emits `[TICK-PHASES]` lines for world-tick phases that take at least
one millisecond. Each entry is reported as `phase=total/max` in milliseconds;
these lines identify long chunk-eviction, scheduled-update, autosave, or random
block-update stalls without changing the tick order.
The client tick also records `runTickTotal` when it exceeds 20 ms. The periodic
surroundings refresh is split into `joinChunksLoad`, `joinChunksAttach`, and
`joinChunksTotal`, separating chunk acquisition from entity-list bookkeeping.
The pre-world portion is also split into `modTick`, `guiDestroy`, and
`worldDestroy`, covering mod callbacks and deferred object destruction that can
otherwise be hidden inside the total. World startup and transition work is
reported as `autoload` and `portal`, while background asset pumping is reported
as `backgroundTex`. GUI processing is reported as `screenInput` or
`playerScreens`, screen creation/destruction transitions as
`screenTransition`, and device/input processing remains reported as `input`.
Small pause-menu and click-counter bookkeeping is reported as
`tickBookkeeping`, closing the remaining pre-world timing gap.
World-side setup outside entity and world ticking is reported as `worldSetup`;
the no-world audio path is reported as `menuAudio`.
PSP fog enable/disable operations are ignored because the PSP OpenGL backend
rejects `GL_FOG` with `GL_INVALID_ENUM`; other platforms retain normal fog
behavior.
PSP lighting enable/disable operations are also ignored because the backend
rejects `GL_LIGHTING` with `GL_INVALID_ENUM`; the PSP terrain path uses
fullbright rendering and reports no lighting reads.
PSP `Light0` enable/disable operations are ignored for the same fixed-function
lighting path; other platforms retain normal behavior.
PSP `Light1` enable/disable operations are also ignored on this path.
PSP shade-model changes are ignored because the backend rejects the fixed-
function `glShadeModel` state with `GL_INVALID_ENUM`; other platforms retain
their normal behavior.
PSP exit/full saves serialize only chunks that are actually dirty (plus their
normal level, player, entity, and region-cache data), rather than rewriting
every loaded chunk. This avoids multi-minute memory-stick stalls without
discarding unchanged chunk data.
The pause-menu `Saving level..` indicator is shown only when the incremental
save reports that more dirty chunks remain; opening the menu alone no longer
shows a misleading saving message for its first frames.
Incremental pause-menu saving is advanced from `updateScreen()` and the
rendering path uses the cached result, preventing save work from being
repeated during every menu redraw.
On PSP, pause-menu background saving is disabled because repeated memory-stick
writes block menu input for seconds; Save & Quit still performs the explicit
dirty-chunk save.
During `PSP_REMOTE_DEBUG` captures, PSPLINK output is filtered to frame samples,
stall diagnostics, and errors/warnings; verbose world-load and lighting lines
are suppressed to reduce synchronous-output distortion.
The startup `build=` value is generated by CMake during configuration rather
than taken from compiler `__DATE__`/`__TIME__` macros, so ccache reuse cannot
make the deployed diagnostic identify an older cached object. Normal PSP
incremental builds should reuse the existing build directory; deleting it is
only necessary when the build directory or its configuration is corrupted.

***

# Español

## Descripcion General

OptiCraft Heritage Edition (Port PSP) es un port limpio y altamente optimizado en C++17 de la era clasica de Minecraft, diseñado especificamente para la consola portatil Sony PlayStation Portable (PSP) y el emulador PPSSPP.

Desarrollado directamente sobre el PSPSDK nativo y la arquitectura Allegrex MIPS, este port traslada la experiencia sandbox clasica a la legendaria consola de Sony respetando estrictamente sus limitaciones de hardware (CPU MIPS R4000 a 333 MHz y 32MB / 64MB de memoria RAM).

## Caracteristicas y Optimizaciones

**Generador de Terreno 2D Heightmap (Paridad con PS2)**
Utiliza el generador de mapa de alturas (ChunkProviderGenerateLite) y muestreo rapido de biomas (WorldChunkManagerFast), exactamente igual a la version de PlayStation 2. Reemplaza el pesado ruido 3D Perlin de doble precision por matematicas en coma flotante de precision simple, logrando una generacion de chunks instantanea sin congelamientos en la CPU MIPS.

**Mundos Delimitados de 256x256 Optimizados**
Mundos autenticos de consola (16x16 chunks) con fisica de bordes suave. La descomposicion de velocidad radial anula unicamente la componente de movimiento que intenta salir del mapa, permitiendo deslizarse por el borde y regresar hacia el interior con total fluidez, eliminando el clasico problema de quedarse enganchado o pegado en el limite.

**Sistema de Control Dual Adaptado**
Modo En Juego: Movimiento fluido en 360° con el joystick analogico, control de camara suave mediante los botones frontales con curva de aceleracion exponencial, cambio rapido de barra de acceso y gatillos de accion.
Modo Menus: Navegacion combinada mediante cruceta direccional (salto entre botones) y puntero virtual analogico para inventarios y crafteo.

**Salto Automatico (Auto Jump) Integrado**
Deteccion y subida automatica de obstaculos de un bloque activada por defecto, con interruptor de activacion o desactivacion en las opciones de pausa (Host Options) y de sistema (Heritage Options).

**Ajuste de Buffer de Profundidad de 16 bits**
Optimizado para el Graphic Engine (GE) de la PSP para evitar colisiones coplanares en el buffer de profundidad, asegurando corazones y comida con relleno visible y tipografias e iconos nitidos.

**Presupuesto Estricto de Memoria**
Administracion dinamica de memoria y streaming acotado de chunks para garantizar estabilidad total tanto en consolas reales como en emuladores.

## Esquema de Controles

### Durante la Partida

Stick Analogico: Caminar / Desplazarse (Movimiento 360°)
△ (Triangulo): Mirar hacia arriba
✕ (Cruz): Mirar hacia abajo
□ (Cuadrado): Mirar hacia la izquierda
○ (Circulo): Mirar hacia la derecha
Gatillo R: Accion Principal (Atacar / Romper bloque)
Gatillo L: Accion Secundaria (Usar / Colocar bloque)
Cruceta Arriba: Saltar
Cruceta Abajo: Agacharse / Sigilo
Cruceta Izq / Der: Cambiar objeto en la barra rapida
START: Menu de Pausa / Opciones de Anfitrion
SELECT: Inventario / Crafteo
R + SELECT: Abrir directamente Opciones de Anfitrion

### En Menus e Inventarios

Cruceta: Navegar botones y casillas de inventario
Stick Analogico: Mover cursor puntero virtual
✕ (Cruz): Seleccionar / Confirmar / Clic Izquierdo
□ (Cuadrado): Dividir monton / Clic Derecho
△ (Triangulo): Movimiento rapido (Shift + Clic)
○ (Circulo): Volver / Cancelar / Salir del menu
Gatillos L / R: Desplazar lista / Cambiar pestaña creativa

## Guia de Instalacion

### 1. Requisitos
Una consola PlayStation Portable (PSP 1000, 2000, 3000, Go o Street) con Custom Firmware instalado (PRO, ME o ARK4), una PS Vita con Adrenaline, o el emulador PPSSPP.
Nota: En PSP 1000 (32MB de RAM), se recomienda encarecidamente jugar en mundos antiguos de 256x256.

### 2. Ubicacion de Archivos
Copia la carpeta del juego en la Memory Stick bajo PSP/GAME/:

```text
ms0:/
 └── PSP/
      └── GAME/
           └── OptiCraft/
                ├── EBOOT.PBP
                └── assets.pak
```

**Ruta en PPSSPP (Windows)**:
C:\Usuarios\<TuUsuario>\Documentos\PPSSPP\PSP\GAME\OptiCraft\

**Ruta en PPSSPP (Android)**:
/storage/emulated/0/PSP/GAME/OptiCraft/

## Compilacion desde Cero

Puedes compilar OptiCraft para PSP en Windows (via WSL2), Linux o Docker.

### Compilacion en Windows o Linux

1. Desde Windows en la carpeta del proyecto, ejecuta:
   "build psp.bat"

2. O dentro de la terminal de WSL / Linux, ejecuta:
   bash ./build_psp.sh

El archivo compilado se encontrara en la raiz y en build/psp/bin/psp/EBOOT.PBP.

## Licencia

Este proyecto esta bajo los terminos de la licencia GPL v3 (LICENSE).
Minecraft es una marca registrada de Mojang Synergies AB / Microsoft. Este proyecto es una reimplementacion independiente desarrollada con fines educativos y de preservacion historica.

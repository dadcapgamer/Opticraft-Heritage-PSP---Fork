# OptiCraft Heritage Edition: PlayStation Portable (PSP)

## Overview

OptiCraft Heritage Edition (PSP Port) is an optimized, clean-room C++17 port of classic Minecraft gameplay specifically crafted for the Sony PlayStation Portable (PSP) and the PPSSPP emulator.

Built directly against the bare-metal PSPSDK and Allegrex MIPS architecture, this port brings the authentic classic sandbox experience to Sony's legendary handheld while strictly respecting its tight hardware limitations (MIPS R4000 at 333 MHz, 32MB / 64MB RAM).

## Features and Optimizations

* **Optimized World Generation:** Lightweight generation algorithms configured for ultra-fast chunk creation on MIPS architecture without CPU stalls.
* **Optimized Bounded Worlds:** Authentic console-sized worlds with seamless edge physics and outward velocity decomposition to prevent players from getting stuck at borders.
* **Custom Dual-Mode Control Layout:** 
  * *Gameplay Mode:* Smooth analog nub movement, responsive camera look curves, quick hotbar cycling, and dedicated trigger actions.
  * *Menu Mode:* Dual navigation featuring intuitive D-Pad grid jumping and a fast analog virtual pointer for inventories and crafting.
* **Auto-Jump Built-In:** Automatic single-block elevation step-up enabled by default, with an in-game toggle available in options.
* **Fixed-Function 16-bit Depth Optimization:** Engineered specifically for the PSP Graphic Engine (GE) to prevent Z-fighting in 2D overlays and UI elements.
* **Memory Management:** Configured memory ring buffers and chunk load ceilings to guarantee stability across retail hardware and emulators.

## Controller Layout

### Gameplay Controls
* **Analog Stick:** Walk / Strafe (360° Movement)
* **△ (Triangle):** Look Up
* **✕ (Cross):** Look Down
* **□ (Square):** Look Left
* **○ (Circle):** Look Right
* **R Trigger:** Primary Action (Attack / Mine Block)
* **L Trigger:** Secondary Action (Use / Place Block)
* **D-Pad Up:** Jump
* **D-Pad Down:** Sneak / Crouch
* **D-Pad Left / Right:** Hotbar Item Selection (Previous / Next)
* **START:** Pause Menu / Host Options
* **SELECT:** Inventory / Crafting Menu
* **R + SELECT:** Open Host Options Directly

### Menu and Inventory Controls
* **D-Pad:** Navigate UI buttons and inventory slots
* **Analog Stick:** Move virtual pointer cursor
* **✕ (Cross):** Select / Confirm / Left Click
* **□ (Square):** Split Stack / Right Click
* **△ (Triangle):** Quick Move (Shift + Click)
* **○ (Circle):** Back / Cancel / Exit Menu
* **L / R Triggers:** Scroll Container / Change Creative Tabs

## Installation

### 1. Requirements
* A PlayStation Portable (PSP 1000, 2000, 3000, Go, or Street) running custom firmware (PRO, ME, or ARK4), a PS Vita with Adrenaline, or the PPSSPP emulator.
* *Note:* On PSP 1000 (32MB RAM), smaller bounded worlds are strongly recommended.

### 2. File Placement
Copy the compiled game folder to your Memory Stick under `PSP/GAME/`:

```text
ms0:/
 └── PSP/
     └── GAME/
         └── OptiCraft/
             ├── EBOOT.PBP
             └── assets.pak
```

**PPSSPP Path (Windows)**:
`C:\Users\<YourUser>\Documents\PPSSPP\PSP\GAME\OptiCraft\`

**PPSSPP Path (Android)**:
`/storage/emulated/0/PSP/GAME/OptiCraft/`

## Compiling from Scratch

You can compile OptiCraft for PSP using Windows (WSL2), Linux, or Docker.

### Building on Windows or Linux

1. From Windows in the project directory, run:
   `build psp.bat`

2. Or inside WSL / Linux terminal, run:
   `bash ./build_psp.sh`

The resulting `EBOOT.PBP` will be located in the project root and in `build/psp/bin/psp/EBOOT.PBP`.

## License

This project is licensed under the terms of the GPL v3 (`LICENSE`).  
Minecraft is a trademark of Mojang Synergies AB / Microsoft. This project is an independent reimplementation developed for educational and historical preservation purposes.

# Heritage PSP Java-like roadmap

This fork is the new foundation for the MCPSP project. The earlier standalone
prototype remains useful as a reference for terrain tests and PSP ergonomics,
but runtime work belongs here.

## Product target

Deliver the most complete Java-like survival sandbox practical on retail PSP
hardware: deterministic streamed worlds, persistent exploration and building,
classic Java systems and content, console-native controls/UI, texture-pack
support, and a bundled PSP visual profile.

"Infinite" means no designed world border. A small moving set of chunks stays
resident in RAM; chunks outside it are saved or deterministically regenerated.
It does not mean an unlimited number of chunks are simultaneously resident.

## Existing foundation

Heritage already supplies the stronger gameplay base:

- Java-style chunk, biome, cave, ravine, ore, and decoration systems.
- Villages, mineshafts, strongholds, dungeons, and multiple dimensions.
- Survival hunger, sprinting-era gameplay, brewing, enchanting, mobs, and
  improved creative inventory systems.
- Region-file world persistence and deterministic seed-based generation.
- PSP input, rendering, storage, texture residency, and bounded chunk caches.
- Texture-pack loading infrastructure.

## Milestones

### 1. Infinite-world persistence foundation

- Make Infinite the PSP default while retaining Old (256x256) compatibility.
- Keep a bounded resident chunk window around the player.
- Save modified chunks before eviction and reload them without data loss.
- Validate travel across positive and negative region-file boundaries.
- Instrument generation, mesh, save, and free-memory spikes on hardware.

### 2. Terrain fidelity and continuity

- Establish deterministic seed snapshots for terrain, biome, cave, and ore
  generation at distant positive and negative coordinates.
- Fix chunk-edge seams in surfaces, caves, liquids, lighting, and structures.
- Tune biome scale and terrain variation toward Java Edition 1.2.5 behavior.
- Spread generation/population work across frames without visible stalls.

### 3. Stable survival loop

- Verify mining, placement, crafting, furnaces, hunger, combat, death, beds,
  weather, day/night, saving, and reopening across streamed chunks.
- Bound entity, tile-entity, particle, pathfinding, and scheduled-tick memory.
- Add graceful low-memory recovery instead of XMB crashes.

### 4. World content

- Validate animals and hostile mobs across all supported biomes.
- Validate villages/villagers, mineshafts, strongholds, dungeons, Nether, and
  End progression under the PSP streaming budget.
- Restore or tune features only where hardware measurements permit them.

### 5. PSP presentation

- Finalize analog movement, face-button camera controls, auto-jump, targeting,
  inventory navigation, remapping, inversion, and sensitivity settings.
- Ship a crisp low-resolution default texture profile and documented custom
  texture-pack workflow.
- Build a fixed-function PSP visual profile (color grading, fog, sky/water,
  lighting curves, and optional post-style effects). Arbitrary desktop GLSL
  shader packs are not a PSP GU capability.

## Release gates

Each milestone must pass on a PSP-1000 memory profile and a later 64 MB PSP:

- No crash during a 30-minute outward exploration run.
- Resident chunk count stays bounded.
- A modified block survives eviction, restart, and reload.
- No terrain, liquid, lighting, or structure seams at chunk boundaries.
- Frame pacing remains playable while chunks generate and save.
- Existing Old worlds remain loadable.


# PSP infinite-world acceptance test

Use a fixed seed for every regression run and record the PSP model, free-space,
build hash, and `assets.pak` hash.

1. Create an Infinite survival world and record spawn coordinates.
2. Place a distinctive block pattern and a chest containing named stacks at
   spawn.
3. Travel at least six chunks east, forcing spawn chunks outside the resident
   cache, then return and verify the pattern and chest contents.
4. Repeat west, north, and south, crossing both positive and negative chunk
   coordinates.
5. Cross an `.mcr` region boundary (32 chunks), restart, and return to edits on
   both sides of the boundary.
6. Confirm newly reached terrain has no designed border and remains deterministic
   after restarting with the same save.
7. Check cave openings, ores, liquids, trees, and structures where they cross a
   chunk boundary.
8. Save and quit during active exploration, reopen the world, and verify player
   position, inventory, time, entities, tile entities, and modified terrain.
9. Repeat the outward run for 30 minutes while recording frame pacing, resident
   chunks, generation time, save time, and minimum free memory.

Old 256x256 worlds must also be created, saved, reopened, and boundary-tested to
catch compatibility regressions.


#pragma once

#include <cstdint>

enum class PlatformRenderPhase
{
    Sky = 0,
    Frustum,
    Build,
    Opaque,
    Entities,
    Translucent,
    Hand,
    Hud,
    // Appended, not inserted: PS2's static_cast dispatch (Profiler_PS2.cpp ->
    // ps2_perf_add_render_phase) indexes this enum directly, so reordering the
    // values above would silently relabel their samples. Both are sub-phases of
    // Entities, not siblings of it; see Ps2RenderPhase.h. Wii and PC implement
    // platformProfileRenderPhaseEnd as a no-op and ignore them.
    EntityDraw,
    TileEntityDraw,
    // Sub-phases of Hud, on the same terms as the two above: nested inside it,
    // not siblings. Hud minus all three is the frame/crosshair/status setup.
    HudItems,
    HudText,
    HudHints
};

enum class PlatformPopulatePhase
{
    Total = 0,
    Lakes,
    Dungeons,
    Fillers,
    Ores,
    Decoration,
    Springs,
    Snow,
    // Appended, not inserted: PS2's static_cast dispatch (Profiler_PS2.cpp) and
    // Wii's (Profiler_WII.cpp, bounds-checked at 8) both index this enum
    // directly, so reordering existing values would silently relabel their
    // samples. Wii ignores this one (its bound check is `< 8`); it was never
    // wired to a distinct bucket there either, so nothing regresses.
    Structures
};

std::uint32_t platformProfileRenderPhaseBegin();
void platformProfileRenderPhaseEnd(std::uint32_t start, PlatformRenderPhase phase);
void platformProfileRenderPhaseSnapshot(long long* outNs, int count);
void platformProfileTickPhase(const char* name, long long ns);
void platformProfileChunkBuild(long long ns, int vertices);
void platformProfileChunkMeshPass(int pass, long long ns, int vertices);
void platformProfilePspMeshStep(long long ns, bool completed, int passes, int vertices,
    long long cpuNs, long long textureNs, long long displayNs,
    long long geometryNs);
std::uint32_t platformProfilePspGeometryBegin();
long long platformProfilePspGeometryEnd(std::uint32_t start);
void platformProfilePspMeshQueue(int pending, int candidates, int attempts, int completed);
void platformProfilePspMeshInvalidation(bool alreadyPending, bool duringBuild, int source);
void platformProfilePspMeshQueueStats(int oldestMs, int nearestDistance, int farthestDistance);
void platformProfilePspMeshCategory(int category, long long ns);
void platformProfilePspMeshCache(bool hit);
void platformProfilePspMeshSpecialType(int renderType);
void platformProfilePspFluidHeightQuery();
void platformProfilePspFluidFace(bool visible);
void platformProfilePspTranslucentPass(long long ns, int drawLists);
void platformProfilePspColorMaskCall();
void platformProfilePspRenderStateError(int operation, int value, unsigned int error);
struct PspTickPhaseSnapshot
{
    char name[16] = {};
    long long totalNs = 0;
    long long maxNs = 0;
    int count = 0;
};
int platformProfilePspTickSnapshot(PspTickPhaseSnapshot* out, int capacity);
void platformProfilePspMeshCounters(int blockLoop, int blockReads, int lightReads,
    int renderCalls, int vertices, int standardBlocks, int specialBlocks,
    int hiddenOpaqueBlocks, int visibleOpaqueBlocks);

struct PspMeshProfileSnapshot
{
    long long buildNs = 0;
    long long cpuNs = 0;
    long long textureNs = 0;
    long long displayNs = 0;
    long long geometryNs = 0;
    int steps = 0;
    int completed = 0;
    int passes = 0;
    int vertices = 0;
    int pending = 0;
    int candidates = 0;
    int attempts = 0;
    int queueCompleted = 0;
    int dirtyMarks = 0;
    int dirtyAlreadyPending = 0;
    int dirtyDuringBuild = 0;
    int dirtyBlockChanges = 0;
    int dirtyLighting = 0;
    int dirtyStreaming = 0;
    int dirtyOther = 0;
    int queueOldestMs = 0;
    int queueNearestDistance = 0;
    int queueFarthestDistance = 0;
    int blockLoop = 0;
    int blockReads = 0;
    int lightReads = 0;
    int renderCalls = 0;
    int standardBlocks = 0;
    int specialBlocks = 0;
    int hiddenOpaqueBlocks = 0;
    int visibleOpaqueBlocks = 0;
    int cacheHits = 0;
    int cacheMisses = 0;
    int fluidBlocks = 0;
    int plantBlocks = 0;
    int stairBlocks = 0;
    int otherSpecialBlocks = 0;
    long long standardNs = 0;
    long long fluidNs = 0;
    long long plantNs = 0;
    long long stairNs = 0;
    long long otherSpecialNs = 0;
    int fluidHeightQueries = 0;
    int fluidVisibleFaces = 0;
    int fluidSkippedFaces = 0;
    long long translucentNs = 0;
    int translucentDrawLists = 0;
    int colorMaskCalls = 0;
    int renderStateErrors = 0;
    int renderStateLastOperation = 0;
    int renderStateLastValue = 0;
    unsigned int renderStateLastError = 0;
};

PspMeshProfileSnapshot platformProfilePspMeshSnapshot();
void platformProfileSnowColumn(bool icePlaced, bool snowPlaced, int notifyCalls);
void platformProfilePopulatePhase(PlatformPopulatePhase phase, long long ns);

void platformProfileChunkLoad(long long ns);
void platformProfilePopulate(long long ns);
void platformProfileGenerate(long long ns);
void platformProfileMesh(long long ns);
void platformProfileUnloadSave(long long ns);
void platformProfileTickUpdates(long long ns);
void platformProfileTickQueue(long long size);
void platformProfileMobSpawn(long long ns);
void platformProfileSaveWorldInfo(long long ns);
void platformProfileMapStorage(long long ns);
void platformProfileChunkEvict(long long ns);

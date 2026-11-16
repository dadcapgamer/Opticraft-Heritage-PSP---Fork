#include "platform/Profiler.h"

#ifdef PSP_PLATFORM

#include <cstring>
#include <pspkernel.h>
namespace
{
const int kPspTickPhaseSlots = 16;
struct PspTickPhase
{
    char name[16];
    long long totalNs;
    long long maxNs;
    int count;
};
unsigned long long g_renderPhaseUs[static_cast<int>(PlatformRenderPhase::HudHints) + 1] = {};
PspTickPhase g_pspTickPhases[kPspTickPhaseSlots] = {};
int g_pspTickPhaseCount = 0;
long long g_pspMeshBuildNs = 0;
long long g_pspMeshCpuNs = 0;
long long g_pspMeshTextureNs = 0;
long long g_pspMeshDisplayNs = 0;
long long g_pspMeshGeometryNs = 0;
int g_pspMeshSteps = 0;
int g_pspMeshCompleted = 0;
int g_pspMeshPasses = 0;
int g_pspMeshVertices = 0;
int g_pspMeshPending = 0;
int g_pspMeshCandidates = 0;
int g_pspMeshAttempts = 0;
int g_pspMeshCompletedQueue = 0;
int g_pspMeshDirtyMarks = 0;
int g_pspMeshDirtyAlreadyPending = 0;
int g_pspMeshDirtyDuringBuild = 0;
int g_pspMeshDirtyBlockChanges = 0;
int g_pspMeshDirtyLighting = 0;
int g_pspMeshDirtyStreaming = 0;
int g_pspMeshDirtyOther = 0;
int g_pspMeshQueueOldestMs = 0;
int g_pspMeshQueueNearestDistance = 0;
int g_pspMeshQueueFarthestDistance = 0;
long long g_pspMeshStandardNs = 0;
long long g_pspMeshFluidNs = 0;
long long g_pspMeshPlantNs = 0;
long long g_pspMeshStairNs = 0;
long long g_pspMeshOtherSpecialNs = 0;
int g_pspMeshBlockLoop = 0;
int g_pspMeshBlockReads = 0;
int g_pspMeshLightReads = 0;
int g_pspMeshRenderCalls = 0;
int g_pspMeshStandardBlocks = 0;
int g_pspMeshSpecialBlocks = 0;
int g_pspMeshHiddenOpaqueBlocks = 0;
int g_pspMeshVisibleOpaqueBlocks = 0;
int g_pspMeshCacheHits = 0;
int g_pspMeshCacheMisses = 0;
int g_pspMeshFluidBlocks = 0;
int g_pspMeshPlantBlocks = 0;
int g_pspMeshStairBlocks = 0;
int g_pspMeshOtherSpecialBlocks = 0;
int g_pspFluidHeightQueries = 0;
int g_pspFluidVisibleFaces = 0;
int g_pspFluidSkippedFaces = 0;
long long g_pspTranslucentNs = 0;
int g_pspTranslucentDrawLists = 0;
int g_pspColorMaskCalls = 0;
int g_pspRenderStateErrors = 0;
int g_pspRenderStateLastOperation = 0;
int g_pspRenderStateLastValue = 0;
unsigned int g_pspRenderStateLastError = 0;
}

std::uint32_t platformProfileRenderPhaseBegin()
{
    return sceKernelGetSystemTimeLow();
}
void platformProfileRenderPhaseEnd(std::uint32_t start, PlatformRenderPhase phase)
{
    const int index = static_cast<int>(phase);
    if (index >= 0 && index < static_cast<int>(sizeof(g_renderPhaseUs) / sizeof(g_renderPhaseUs[0])))
        g_renderPhaseUs[index] += static_cast<unsigned int>(sceKernelGetSystemTimeLow() - start);
}
void platformProfileRenderPhaseSnapshot(long long* outNs, int count)
{
    if (outNs == nullptr || count <= 0)
        return;

    const int available = static_cast<int>(sizeof(g_renderPhaseUs) / sizeof(g_renderPhaseUs[0]));
    const int copied = count < available ? count : available;
    for (int i = 0; i < copied; ++i)
    {
        outNs[i] = static_cast<long long>(g_renderPhaseUs[i]) * 1000LL;
        g_renderPhaseUs[i] = 0;
    }
    for (int i = copied; i < count; ++i)
        outNs[i] = 0;
}
void platformProfileTickPhase(const char* name, long long ns)
{
    if (name == nullptr || ns < 0)
        return;

    for (int i = 0; i < g_pspTickPhaseCount; ++i)
    {
        PspTickPhase& phase = g_pspTickPhases[i];
        if (std::strncmp(phase.name, name, sizeof(phase.name)) == 0)
        {
            phase.totalNs += ns;
            if (ns > phase.maxNs)
                phase.maxNs = ns;
            ++phase.count;
            return;
        }
    }

    if (g_pspTickPhaseCount >= kPspTickPhaseSlots)
        return;

    PspTickPhase& phase = g_pspTickPhases[g_pspTickPhaseCount++];
    std::strncpy(phase.name, name, sizeof(phase.name) - 1);
    phase.name[sizeof(phase.name) - 1] = '\0';
    phase.totalNs = ns;
    phase.maxNs = ns;
    phase.count = 1;
}
int platformProfilePspTickSnapshot(PspTickPhaseSnapshot* out, int capacity)
{
    if (out == nullptr || capacity <= 0)
        return 0;

    const int copied = capacity < g_pspTickPhaseCount ? capacity : g_pspTickPhaseCount;
    for (int i = 0; i < copied; ++i)
    {
        std::memcpy(out[i].name, g_pspTickPhases[i].name, sizeof(out[i].name));
        out[i].totalNs = g_pspTickPhases[i].totalNs;
        out[i].maxNs = g_pspTickPhases[i].maxNs;
        out[i].count = g_pspTickPhases[i].count;
        g_pspTickPhases[i] = PspTickPhase();
    }
    g_pspTickPhaseCount = 0;
    return copied;
}
void platformProfileChunkBuild(long long, int) {}
void platformProfileChunkMeshPass(int, long long, int) {}
void platformProfilePspMeshStep(long long ns, bool completed, int passes, int vertices,
    long long cpuNs, long long textureNs, long long displayNs, long long geometryNs)
{
    (void)vertices;
    g_pspMeshBuildNs += ns;
    g_pspMeshCpuNs += cpuNs;
    g_pspMeshTextureNs += textureNs;
    g_pspMeshDisplayNs += displayNs;
    g_pspMeshGeometryNs += geometryNs;
    ++g_pspMeshSteps;
    g_pspMeshPasses += passes;
    if (completed)
        ++g_pspMeshCompleted;
}
void platformProfilePspMeshQueue(int pending, int candidates, int attempts, int completed)
{
    g_pspMeshPending = pending;
    g_pspMeshCandidates = candidates;
    g_pspMeshAttempts = attempts;
    g_pspMeshCompletedQueue = completed;
}
void platformProfilePspMeshInvalidation(bool alreadyPending, bool duringBuild, int source)
{
    ++g_pspMeshDirtyMarks;
    if (alreadyPending)
        ++g_pspMeshDirtyAlreadyPending;
    if (duringBuild)
        ++g_pspMeshDirtyDuringBuild;
    if (source == 0)
        ++g_pspMeshDirtyBlockChanges;
    else if (source == 1)
        ++g_pspMeshDirtyLighting;
    else if (source == 2)
        ++g_pspMeshDirtyStreaming;
    else
        ++g_pspMeshDirtyOther;
}
void platformProfilePspMeshQueueStats(int oldestMs, int nearestDistance, int farthestDistance)
{
    g_pspMeshQueueOldestMs = oldestMs;
    g_pspMeshQueueNearestDistance = nearestDistance;
    g_pspMeshQueueFarthestDistance = farthestDistance;
}
void platformProfilePspMeshCategory(int category, long long ns)
{
    if (ns < 0)
        return;
    if (category == 0)
        g_pspMeshStandardNs += ns;
    else if (category == 1)
        g_pspMeshFluidNs += ns;
    else if (category == 2)
        g_pspMeshPlantNs += ns;
    else if (category == 3)
        g_pspMeshStairNs += ns;
    else
        g_pspMeshOtherSpecialNs += ns;
}
void platformProfilePspMeshCache(bool hit)
{
    if (hit)
        ++g_pspMeshCacheHits;
    else
        ++g_pspMeshCacheMisses;
}
void platformProfilePspMeshSpecialType(int renderType)
{
    if (renderType == 4)
        ++g_pspMeshFluidBlocks;
    else if (renderType == 1 || renderType == 6)
        ++g_pspMeshPlantBlocks;
    else if (renderType == 10)
        ++g_pspMeshStairBlocks;
    else
        ++g_pspMeshOtherSpecialBlocks;
}
void platformProfilePspFluidHeightQuery()
{
    ++g_pspFluidHeightQueries;
}
void platformProfilePspFluidFace(bool visible)
{
    if (visible)
        ++g_pspFluidVisibleFaces;
    else
        ++g_pspFluidSkippedFaces;
}
void platformProfilePspTranslucentPass(long long ns, int drawLists)
{
    g_pspTranslucentNs += ns;
    g_pspTranslucentDrawLists += drawLists;
}
void platformProfilePspColorMaskCall()
{
    ++g_pspColorMaskCalls;
}
void platformProfilePspRenderStateError(int operation, int value, unsigned int error)
{
    ++g_pspRenderStateErrors;
    g_pspRenderStateLastOperation = operation;
    g_pspRenderStateLastValue = value;
    g_pspRenderStateLastError = error;
}
void platformProfilePspMeshCounters(int blockLoop, int blockReads, int lightReads,
    int renderCalls, int vertices, int standardBlocks, int specialBlocks,
    int hiddenOpaqueBlocks, int visibleOpaqueBlocks)
{
    g_pspMeshBlockLoop += blockLoop;
    g_pspMeshBlockReads += blockReads;
    g_pspMeshLightReads += lightReads;
    g_pspMeshRenderCalls += renderCalls;
    g_pspMeshVertices += vertices;
    g_pspMeshStandardBlocks += standardBlocks;
    g_pspMeshSpecialBlocks += specialBlocks;
    g_pspMeshHiddenOpaqueBlocks += hiddenOpaqueBlocks;
    g_pspMeshVisibleOpaqueBlocks += visibleOpaqueBlocks;
}
std::uint32_t platformProfilePspGeometryBegin()
{
    return sceKernelGetSystemTimeLow();
}
long long platformProfilePspGeometryEnd(std::uint32_t start)
{
    return (long long)(unsigned int)(sceKernelGetSystemTimeLow() - start) * 1000LL;
}
PspMeshProfileSnapshot platformProfilePspMeshSnapshot()
{
    PspMeshProfileSnapshot snapshot;
    snapshot.buildNs = g_pspMeshBuildNs;
    snapshot.cpuNs = g_pspMeshCpuNs;
    snapshot.textureNs = g_pspMeshTextureNs;
    snapshot.displayNs = g_pspMeshDisplayNs;
    snapshot.geometryNs = g_pspMeshGeometryNs;
    snapshot.steps = g_pspMeshSteps;
    snapshot.completed = g_pspMeshCompleted;
    snapshot.passes = g_pspMeshPasses;
    snapshot.vertices = g_pspMeshVertices;
    snapshot.pending = g_pspMeshPending;
    snapshot.candidates = g_pspMeshCandidates;
    snapshot.attempts = g_pspMeshAttempts;
    snapshot.queueCompleted = g_pspMeshCompletedQueue;
    snapshot.dirtyMarks = g_pspMeshDirtyMarks;
    snapshot.dirtyAlreadyPending = g_pspMeshDirtyAlreadyPending;
    snapshot.dirtyDuringBuild = g_pspMeshDirtyDuringBuild;
    snapshot.dirtyBlockChanges = g_pspMeshDirtyBlockChanges;
    snapshot.dirtyLighting = g_pspMeshDirtyLighting;
    snapshot.dirtyStreaming = g_pspMeshDirtyStreaming;
    snapshot.dirtyOther = g_pspMeshDirtyOther;
    snapshot.queueOldestMs = g_pspMeshQueueOldestMs;
    snapshot.queueNearestDistance = g_pspMeshQueueNearestDistance;
    snapshot.queueFarthestDistance = g_pspMeshQueueFarthestDistance;
    snapshot.blockLoop = g_pspMeshBlockLoop;
    snapshot.blockReads = g_pspMeshBlockReads;
    snapshot.lightReads = g_pspMeshLightReads;
    snapshot.renderCalls = g_pspMeshRenderCalls;
    snapshot.standardBlocks = g_pspMeshStandardBlocks;
    snapshot.specialBlocks = g_pspMeshSpecialBlocks;
    snapshot.hiddenOpaqueBlocks = g_pspMeshHiddenOpaqueBlocks;
    snapshot.visibleOpaqueBlocks = g_pspMeshVisibleOpaqueBlocks;
    snapshot.cacheHits = g_pspMeshCacheHits;
    snapshot.cacheMisses = g_pspMeshCacheMisses;
    snapshot.fluidBlocks = g_pspMeshFluidBlocks;
    snapshot.plantBlocks = g_pspMeshPlantBlocks;
    snapshot.stairBlocks = g_pspMeshStairBlocks;
    snapshot.otherSpecialBlocks = g_pspMeshOtherSpecialBlocks;
    snapshot.standardNs = g_pspMeshStandardNs;
    snapshot.fluidNs = g_pspMeshFluidNs;
    snapshot.plantNs = g_pspMeshPlantNs;
    snapshot.stairNs = g_pspMeshStairNs;
    snapshot.otherSpecialNs = g_pspMeshOtherSpecialNs;
    snapshot.fluidHeightQueries = g_pspFluidHeightQueries;
    snapshot.fluidVisibleFaces = g_pspFluidVisibleFaces;
    snapshot.fluidSkippedFaces = g_pspFluidSkippedFaces;
    snapshot.translucentNs = g_pspTranslucentNs;
    snapshot.translucentDrawLists = g_pspTranslucentDrawLists;
    snapshot.colorMaskCalls = g_pspColorMaskCalls;
    snapshot.renderStateErrors = g_pspRenderStateErrors;
    snapshot.renderStateLastOperation = g_pspRenderStateLastOperation;
    snapshot.renderStateLastValue = g_pspRenderStateLastValue;
    snapshot.renderStateLastError = g_pspRenderStateLastError;
    g_pspMeshBuildNs = 0;
    g_pspMeshCpuNs = 0;
    g_pspMeshTextureNs = 0;
    g_pspMeshDisplayNs = 0;
    g_pspMeshGeometryNs = 0;
    g_pspMeshSteps = 0;
    g_pspMeshCompleted = 0;
    g_pspMeshPasses = 0;
    g_pspMeshVertices = 0;
    g_pspMeshPending = 0;
    g_pspMeshCandidates = 0;
    g_pspMeshAttempts = 0;
    g_pspMeshCompletedQueue = 0;
    g_pspMeshDirtyMarks = 0;
    g_pspMeshDirtyAlreadyPending = 0;
    g_pspMeshDirtyDuringBuild = 0;
    g_pspMeshDirtyBlockChanges = 0;
    g_pspMeshDirtyLighting = 0;
    g_pspMeshDirtyStreaming = 0;
    g_pspMeshDirtyOther = 0;
    g_pspMeshQueueOldestMs = 0;
    g_pspMeshQueueNearestDistance = 0;
    g_pspMeshQueueFarthestDistance = 0;
    g_pspMeshBlockLoop = 0;
    g_pspMeshBlockReads = 0;
    g_pspMeshLightReads = 0;
    g_pspMeshRenderCalls = 0;
    g_pspMeshStandardBlocks = 0;
    g_pspMeshSpecialBlocks = 0;
    g_pspMeshHiddenOpaqueBlocks = 0;
    g_pspMeshVisibleOpaqueBlocks = 0;
    g_pspMeshCacheHits = 0;
    g_pspMeshCacheMisses = 0;
    g_pspMeshFluidBlocks = 0;
    g_pspMeshPlantBlocks = 0;
    g_pspMeshStairBlocks = 0;
    g_pspMeshOtherSpecialBlocks = 0;
    g_pspMeshStandardNs = 0;
    g_pspMeshFluidNs = 0;
    g_pspMeshPlantNs = 0;
    g_pspMeshStairNs = 0;
    g_pspMeshOtherSpecialNs = 0;
    g_pspFluidHeightQueries = 0;
    g_pspFluidVisibleFaces = 0;
    g_pspFluidSkippedFaces = 0;
    g_pspTranslucentNs = 0;
    g_pspTranslucentDrawLists = 0;
    g_pspColorMaskCalls = 0;
    g_pspRenderStateErrors = 0;
    g_pspRenderStateLastOperation = 0;
    g_pspRenderStateLastValue = 0;
    g_pspRenderStateLastError = 0;
    return snapshot;
}
void platformProfileSnowColumn(bool, bool, int) {}
void platformProfilePopulatePhase(PlatformPopulatePhase, long long) {}
void platformProfileChunkLoad(long long) {}
void platformProfilePopulate(long long) {}
void platformProfileGenerate(long long) {}
void platformProfileMesh(long long) {}
void platformProfileUnloadSave(long long) {}
void platformProfileTickUpdates(long long) {}
void platformProfileTickQueue(long long) {}
void platformProfileMobSpawn(long long) {}
void platformProfileSaveWorldInfo(long long) {}
void platformProfileMapStorage(long long) {}
void platformProfileChunkEvict(long long) {}

#endif // PSP_PLATFORM

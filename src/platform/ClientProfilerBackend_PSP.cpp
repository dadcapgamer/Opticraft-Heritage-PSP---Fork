#include "platform/ClientProfilerBackend.h"

#ifdef PSP_PLATFORM

#include "platform/Log.h"
#include "platform/Profiler.h"
#include "net/minecraft/src/RenderGlobal.h"
#include <cstdio>
#include <pspkernel.h>

namespace ClientProfilerBackend
{
void frameBegin() {}
void ticks(long long, int) {}
void lighting(long long) {}
void displayUpdate(long long) {}
void render(long long) {}
void frameEnd(long long frameNs, long long tickNs, long long renderNs,
              int ticksThisFrame, int chunkUpdates, World*, RenderGlobal* renderGlobal)
{
#if MC_LOG_LEVEL >= 2
    static int frameCount = 0;
    ++frameCount;
    long long phases[static_cast<int>(PlatformRenderPhase::HudHints) + 1] = {};
    platformProfileRenderPhaseSnapshot(
        phases, static_cast<int>(sizeof(phases) / sizeof(phases[0])));
    const RenderGlobalProfileSnapshot renderSnapshot =
        renderGlobal != nullptr ? renderGlobal->getProfileSnapshot() : RenderGlobalProfileSnapshot();
    const PspMeshProfileSnapshot meshSnapshot = platformProfilePspMeshSnapshot();
    PspTickPhaseSnapshot tickPhases[16] = {};
    const int tickPhaseCount = platformProfilePspTickSnapshot(
        tickPhases, static_cast<int>(sizeof(tickPhases) / sizeof(tickPhases[0])));
    char phaseLine[512] = {};
    int phaseLineLength = 0;
    for (int i = 0; i < tickPhaseCount; ++i)
    {
        if (tickPhases[i].maxNs < 1000000LL)
            continue;
        phaseLineLength += snprintf(
            phaseLine + phaseLineLength,
            sizeof(phaseLine) - static_cast<size_t>(phaseLineLength),
            " %s=%ld/%ld",
            tickPhases[i].name,
            (long)(tickPhases[i].totalNs / 1000000LL),
            (long)(tickPhases[i].maxNs / 1000000LL));
        if (phaseLineLength >= static_cast<int>(sizeof(phaseLine)))
            break;
    }
    if (frameCount <= 5 || (frameCount % 60) == 0)
    {
        MC_LOG_DEBUG("frame",
            "telemetry=4 frame=%d frameMs=%ld tickMs=%ld renderMs=%ld build=%ld sky=%ld frustum=%ld opq=%ld ents=%ld trans=%ld hand=%ld hud=%ld loaded=%d rendered=%d ticks=%d chunks=%d freeMem=%ldKB\n",
            frameCount,
            (long)(frameNs / 1000000LL),
            (long)(tickNs / 1000000LL),
            (long)(renderNs / 1000000LL),
            (long)(phases[static_cast<int>(PlatformRenderPhase::Build)] / 1000000LL),
            (long)(phases[static_cast<int>(PlatformRenderPhase::Sky)] / 1000000LL),
            (long)(phases[static_cast<int>(PlatformRenderPhase::Frustum)] / 1000000LL),
            (long)(phases[static_cast<int>(PlatformRenderPhase::Opaque)] / 1000000LL),
            (long)(phases[static_cast<int>(PlatformRenderPhase::Entities)] / 1000000LL),
            (long)(phases[static_cast<int>(PlatformRenderPhase::Translucent)] / 1000000LL),
            (long)(phases[static_cast<int>(PlatformRenderPhase::Hand)] / 1000000LL),
            (long)(phases[static_cast<int>(PlatformRenderPhase::Hud)] / 1000000LL),
            (int)renderSnapshot.renderersLoaded,
            (int)renderSnapshot.renderersBeingRendered,
            ticksThisFrame,
            chunkUpdates,
            (long)(sceKernelTotalFreeMemSize() / 1024));
        MC_LOG_DEBUG("mesh",
                     "meshPending=%d meshDone=%d meshCandidates=%d meshAttempts=%d meshSteps=%d meshPasses=%d meshVerts=%d meshLoop=%d meshReads=%d meshLights=%d meshRenders=%d meshStandard=%d meshSpecial=%d meshHidden=%d meshVisible=%d meshCacheHits=%d meshCacheMisses=%d meshFluid=%d meshPlants=%d meshStairs=%d meshOtherSpecial=%d fluidHeightQueries=%d fluidVisibleFaces=%d fluidSkippedFaces=%d meshStandardMs=%ld meshFluidMs=%ld meshPlantMs=%ld meshStairMs=%ld meshOtherSpecialMs=%ld meshBuildMs=%ld meshScanMs=%ld meshGeomMs=%ld meshDisplayMs=%ld\n",
                     meshSnapshot.pending,
                     meshSnapshot.queueCompleted,
                     meshSnapshot.candidates,
                     meshSnapshot.attempts,
                     meshSnapshot.steps,
                     meshSnapshot.passes,
                     meshSnapshot.vertices,
                     meshSnapshot.blockLoop,
                     meshSnapshot.blockReads,
                     meshSnapshot.lightReads,
                     meshSnapshot.renderCalls,
                     meshSnapshot.standardBlocks,
                     meshSnapshot.specialBlocks,
                     meshSnapshot.hiddenOpaqueBlocks,
                     meshSnapshot.visibleOpaqueBlocks,
                     meshSnapshot.cacheHits,
                     meshSnapshot.cacheMisses,
                     meshSnapshot.fluidBlocks,
                     meshSnapshot.plantBlocks,
                     meshSnapshot.stairBlocks,
                     meshSnapshot.otherSpecialBlocks,
                     meshSnapshot.fluidHeightQueries,
                     meshSnapshot.fluidVisibleFaces,
                     meshSnapshot.fluidSkippedFaces,
                     (long)(meshSnapshot.standardNs / 1000000LL),
                     (long)(meshSnapshot.fluidNs / 1000000LL),
                     (long)(meshSnapshot.plantNs / 1000000LL),
                     (long)(meshSnapshot.stairNs / 1000000LL),
                     (long)(meshSnapshot.otherSpecialNs / 1000000LL),
                     (long)(meshSnapshot.buildNs / 1000000LL),
                     (long)((meshSnapshot.cpuNs > meshSnapshot.geometryNs
                         ? meshSnapshot.cpuNs - meshSnapshot.geometryNs : 0) / 1000000LL),
                     (long)(meshSnapshot.geometryNs / 1000000LL),
                     (long)(meshSnapshot.displayNs / 1000000LL));
        MC_LOG_DEBUG("queue",
                     "meshDirtyMarks=%d meshDirtyAlreadyPending=%d meshDirtyDuringBuild=%d meshDirtyBlockChanges=%d meshDirtyLighting=%d meshDirtyStreaming=%d meshDirtyOther=%d meshQueueOldestMs=%d meshQueueNearestDistance=%d meshQueueFarthestDistance=%d colorMaskCalls=%d renderStateErrors=%d renderStateLastOperation=%d renderStateLastValue=%d renderStateLastError=0x%x freeMemMaxKB=%ld\n",
                     meshSnapshot.dirtyMarks,
                     meshSnapshot.dirtyAlreadyPending,
                     meshSnapshot.dirtyDuringBuild,
                     meshSnapshot.dirtyBlockChanges,
                     meshSnapshot.dirtyLighting,
                     meshSnapshot.dirtyStreaming,
                     meshSnapshot.dirtyOther,
                     meshSnapshot.queueOldestMs,
                     meshSnapshot.queueNearestDistance,
                     meshSnapshot.queueFarthestDistance,
                     meshSnapshot.colorMaskCalls,
                     meshSnapshot.renderStateErrors,
                     meshSnapshot.renderStateLastOperation,
                     meshSnapshot.renderStateLastValue,
                     meshSnapshot.renderStateLastError,
                     (long)(sceKernelMaxFreeMemSize() / 1024));
    }
    if (phaseLineLength > 0)
        MC_LOG_INFO("perf", "[TICK-PHASES]%s ms(total/max)\n", phaseLine);
#else
    (void)frameNs;
    (void)tickNs;
    (void)renderNs;
    (void)ticksThisFrame;
    (void)chunkUpdates;
    (void)renderGlobal;
#endif
}
}

#endif // PSP_PLATFORM

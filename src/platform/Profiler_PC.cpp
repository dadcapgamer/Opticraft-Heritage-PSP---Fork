#include "platform/Profiler.h"

std::uint32_t platformProfileRenderPhaseBegin() { return 0; }
void platformProfileRenderPhaseEnd(std::uint32_t, PlatformRenderPhase) {}
void platformProfileTickPhase(const char*, long long) {}
int platformProfilePspTickSnapshot(PspTickPhaseSnapshot*, int) { return 0; }
void platformProfileChunkBuild(long long, int) {}
void platformProfileChunkMeshPass(int, long long, int) {}
void platformProfilePspMeshStep(long long, bool, int, int, long long, long long, long long, long long) {}
std::uint32_t platformProfilePspGeometryBegin() { return 0; }
long long platformProfilePspGeometryEnd(std::uint32_t) { return 0; }
void platformProfilePspMeshQueue(int, int, int, int) {}
void platformProfilePspMeshInvalidation(bool, bool, int) {}
void platformProfilePspMeshQueueStats(int, int, int) {}
void platformProfilePspMeshCategory(int, long long) {}
void platformProfilePspMeshCache(bool) {}
void platformProfilePspMeshSpecialType(int) {}
void platformProfilePspFluidHeightQuery() {}
void platformProfilePspFluidFace(bool) {}
void platformProfilePspTranslucentPass(long long, int) {}
void platformProfilePspColorMaskCall() {}
void platformProfilePspRenderStateError(int, int, unsigned int) {}
void platformProfilePspMeshCounters(int, int, int, int, int, int, int, int, int) {}
PspMeshProfileSnapshot platformProfilePspMeshSnapshot() { return PspMeshProfileSnapshot(); }
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

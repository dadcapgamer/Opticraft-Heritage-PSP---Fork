#include "platform/Profiler.h"

#include "platform/PlatformCompat.h"

extern "C" void wii_perf_add_render_phase_us(int phase, unsigned int us);
extern "C" void wii_perf_note_tick_phase(const char* name, long long ns);
extern "C" void wii_perf_add_chunk_load_ns(long long ns);
extern "C" void wii_perf_add_populate_ns(long long ns);
extern "C" void wii_perf_add_populate_phase_ns(int phase, long long ns);
extern "C" void wii_perf_add_generate_ns(long long ns);
extern "C" void wii_perf_add_mesh_ns(long long ns);
extern "C" void wii_perf_add_unload_save_ns(long long ns);
extern "C" void wii_perf_add_tickupdates_ns(long long ns);
extern "C" void wii_perf_add_tickupdates_queue(long long size);
extern "C" void wii_perf_add_mobspawn_ns(long long ns);
extern "C" void wii_perf_add_saveworldinfo_ns(long long ns);
extern "C" void wii_perf_add_mapstorage_ns(long long ns);

// Low 32 bits of the time-base microsecond counter: a phase is milliseconds,
// so the truncated difference is exact for anything under ~71 minutes.
std::uint32_t platformProfileRenderPhaseBegin()
{
	return static_cast<std::uint32_t>(PlatformCompat::getMonotonicMicros());
}
void platformProfileRenderPhaseEnd(std::uint32_t start, PlatformRenderPhase phase)
{
	const std::uint32_t now = static_cast<std::uint32_t>(PlatformCompat::getMonotonicMicros());
	wii_perf_add_render_phase_us(static_cast<int>(phase), now - start);
}
void platformProfileTickPhase(const char* name, long long ns) { wii_perf_note_tick_phase(name, ns); }
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
void platformProfilePopulatePhase(PlatformPopulatePhase phase, long long ns) { wii_perf_add_populate_phase_ns(static_cast<int>(phase), ns); }
void platformProfileChunkLoad(long long ns) { wii_perf_add_chunk_load_ns(ns); }
void platformProfilePopulate(long long ns) { wii_perf_add_populate_ns(ns); }
void platformProfileGenerate(long long ns) { wii_perf_add_generate_ns(ns); }
void platformProfileMesh(long long ns) { wii_perf_add_mesh_ns(ns); }
void platformProfileUnloadSave(long long ns) { wii_perf_add_unload_save_ns(ns); }
void platformProfileTickUpdates(long long ns) { wii_perf_add_tickupdates_ns(ns); }
void platformProfileTickQueue(long long size) { wii_perf_add_tickupdates_queue(size); }
void platformProfileMobSpawn(long long ns) { wii_perf_add_mobspawn_ns(ns); }
void platformProfileSaveWorldInfo(long long ns) { wii_perf_add_saveworldinfo_ns(ns); }
void platformProfileMapStorage(long long ns) { wii_perf_add_mapstorage_ns(ns); }
void platformProfileChunkEvict(long long) {}

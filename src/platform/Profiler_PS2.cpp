#include "platform/Profiler.h"

#include "ps2/system/Ps2Perf.h"

extern "C" void ps2_perf_add_chunk_build_ns(long long ns, int vertices);
extern "C" void ps2_perf_note_tick_phase(const char* name, long long ns);
extern "C" void ps2_perf_add_render_phase(int phase, unsigned int cycles);
extern "C" void ps2_perf_add_mesh_pass_ns(int pass, long long ns, int vertices);
extern "C" void ps2_perf_add_snow_column(int icePlaced, int snowPlaced, int notifyCalls);
extern "C" void ps2_perf_add_chunk_load_ns(long long ns);
extern "C" void ps2_perf_add_populate_ns(long long ns);
extern "C" void ps2_perf_add_generate_ns(long long ns);
extern "C" void ps2_perf_add_unload_save_ns(long long ns);
extern "C" void ps2_perf_add_chunk_evict_ns(long long ns);

namespace
{
unsigned int platformPs2Cycles()
{
	unsigned int cycles;
	__asm__ __volatile__("mfc0 %0, $9" : "=r"(cycles));
	return cycles;
}
}

std::uint32_t platformProfileRenderPhaseBegin() { return platformPs2Cycles(); }
void platformProfileRenderPhaseEnd(std::uint32_t start, PlatformRenderPhase phase)
{
	ps2_perf_add_render_phase(static_cast<int>(phase), platformPs2Cycles() - start);
}
void platformProfileTickPhase(const char* name, long long ns) { ps2_perf_note_tick_phase(name, ns); }
int platformProfilePspTickSnapshot(PspTickPhaseSnapshot*, int) { return 0; }
void platformProfileChunkBuild(long long ns, int vertices) { ps2_perf_add_chunk_build_ns(ns, vertices); }
void platformProfileChunkMeshPass(int pass, long long ns, int vertices) { ps2_perf_add_mesh_pass_ns(pass, ns, vertices); }
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
void platformProfileSnowColumn(bool icePlaced, bool snowPlaced, int notifyCalls)
{
	ps2_perf_add_snow_column(icePlaced ? 1 : 0, snowPlaced ? 1 : 0, notifyCalls);
}
void platformProfilePopulatePhase(PlatformPopulatePhase phase, long long ns)
{
	ps2_perf_add_populate_phase(static_cast<int>(phase), ns);
}
void platformProfileChunkLoad(long long ns)
{
	ps2_perf_add_chunk_load_ns(ns);
	ps2_perf_note_tick_phase("chunkLoad", ns);
}
void platformProfilePopulate(long long ns)
{
	ps2_perf_add_populate_ns(ns);
	ps2_perf_note_tick_phase("populate", ns);
}
void platformProfileGenerate(long long ns)
{
	ps2_perf_add_generate_ns(ns);
	ps2_perf_note_tick_phase("chunkGenerate", ns);
}
void platformProfileMesh(long long) {}
void platformProfileUnloadSave(long long ns)
{
	ps2_perf_add_unload_save_ns(ns);
	ps2_perf_note_tick_phase("unloadSave", ns);
}
void platformProfileTickUpdates(long long) {}
void platformProfileTickQueue(long long) {}
void platformProfileMobSpawn(long long) {}
void platformProfileSaveWorldInfo(long long) {}
void platformProfileMapStorage(long long) {}
void platformProfileChunkEvict(long long ns)
{
	ps2_perf_add_chunk_evict_ns(ns);
	ps2_perf_note_tick_phase("chunkEvict", ns);
}

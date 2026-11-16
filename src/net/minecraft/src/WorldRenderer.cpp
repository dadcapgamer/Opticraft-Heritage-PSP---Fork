#include "WorldRenderer.h"
#include "GLAllocation.h"

#include "java/Arithmetic.h"

#include "platform/RenderAPI.h"
#include "platform/RenderTerrainAPI.h"
#include "World.h"
#include "Config.h"
#include "ConnectedTextures.h"
#include "ICamera.h"
#include "Entity.h"
#include "MathHelper.h"
#include "Block.h"
#include "RenderBlocks.h"
#include "RenderItem.h"
#include "Tessellator.h"
#include "Chunk.h"
#include "ChunkCache.h"
#include "TileEntity.h"
#include "TileEntityRenderer.h"
#include "AxisAlignedBB.h"

#include <algorithm>
#include <cstdint>
#include <utility>

// Not PS2-only: PLATFORM_RENDERER_AABB_MARGIN is used unconditionally further
// down, and this header is the platform-neutral place it comes from (it only
// pulls in ps2/render/Ps2Tuning.h when PLATFORM_PS2 is set). It reached the PC build
// transitively via ChunkProvider.h, which is luck rather than design.
#include "platform/PlatformTuning.h"
#if PLATFORM_PS2
#include "ps2/diagnostics/Ps2OptimizationValidation.h"
#endif
#include "platform/PlatformCompat.h"
#include "platform/ExtendedProfiler.h"
#include "platform/Profiler.h"
#if PLATFORM_PC_LEGACY
#include "pc/render/PcLegacyStaticTileEntityMesh.h"
#endif
#ifdef PS2_PLATFORM
#include "platform/RenderTerrainStaging.h"
#endif



int_t WorldRenderer::chunksUpdated = 0;


namespace
{
	static float rendererAabbMargin()
	{
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM)
		// Consoles use a tuned conservative margin because their terrain clip paths
		// and fixed renderer grids differ from desktop Advanced OpenGL.
		return PLATFORM_RENDERER_AABB_MARGIN;
#else
		// OptiFine C6 uses the exact 16x16x16 section box for both frustum and
		// occlusion tests. A zero margin also makes Fancy Occlusion's "fully in
		// frustum" classification useful instead of inflating every section.
		return 0.0f;
#endif
	}
}

void WorldRenderer::eraseAllTileEntityRefs(std::vector<TileEntity *> *list, TileEntity *te)
{
	if (list == nullptr || te == nullptr)
		return;
	list->erase(std::remove(list->begin(), list->end(), te), list->end());
}

void WorldRenderer::pushUniqueTileEntityRef(std::vector<TileEntity *> *list, TileEntity *te)
{
	if (list == nullptr || te == nullptr)
		return;
	if (std::find(list->begin(), list->end(), te) == list->end())
		list->push_back(te);
}

WorldRenderer::WorldRenderer(World *world, std::vector<TileEntity *> *tileEntitiesIn, int_t posX, int_t posY, int_t posZ, int_t size, int_t glListId)
{
	worldObj      = world;
	tileEntities  = tileEntitiesIn;
	sizeWidth = sizeHeight = sizeDepth = size;
#if PLATFORM_PC || PLATFORM_PSP
	glRenderList = glListId;
#else
	(void)glListId;
#endif
#ifdef WII_PLATFORM
	renderTerrainChunkHandlesCreate(terrainChunkHandles);
#endif
#ifdef WII_PLATFORM
	for (int_t p = 0; p < 2; ++p)
	{
		wiiBuildVertexCount[p] = 0;
		wiiBuildHasTexture[p] = false;
		wiiBuildHasColor[p] = false;
		wiiBuildDrew[p] = false;
		wiiPassNeedsAlphaTest[p] = true;
		wiiBuildNeedsAlphaTest[p] = false;
	}
	wiiBuildActive = false;
	wiiBuildSourceAvailability = 0u;
	wiiBuildSourceAvailabilityValid = false;
	wiiBuildPass = 0;
	wiiBuildCursor = 0;
	wiiBuildHasPass1 = false;
	wiiBuildChunkLit = false;
	wiiBuildDirtyDuringBuild = false;
	wiiStepDidWork = false;
#endif
	needsUpdate    = false;
	isChunkLit     = false;
#if PLATFORM_PC
	isWaitingOnOcclusionQuery = false;
#endif
	isVisible      = true;
	isInFrustum    = false;
#if PLATFORM_PC || PLATFORM_PS2 || PLATFORM_PSP
	isFullyInFrustum = false;
#endif
#if PLATFORM_PC_LEGACY
	pcLegacyBuildActive = false;
	pcLegacyBuildSourceAvailability = 0u;
	pcLegacyBuildSourceAvailabilityValid = false;
	pcLegacyBuildPass = 0;
	pcLegacyBuildCursor = 0;
	pcLegacyBuildHasPass1 = false;
	pcLegacyBuildChunkLit = false;
	pcLegacyStepDidWork = false;
	pcLegacyBuildDirtyDuringBuild = false;
	for (int_t face = 0; face < 6; ++face)
		pcLegacyPublishedVisibility[face] = 0x3f;
	pcLegacyCpuVisible = true;
#endif
#if PLATFORM_PC
	isVisibleFromPosition = false;
	visibleFromX = 0.0;
	visibleFromY = 0.0;
	visibleFromZ = 0.0;
	needsOcclusionBoxUpdate = false;
	glOcclusionQuery = 0;
#endif
	chunkIndex     = 0;
	isInitialized  = false;
	_skipRenderPass[0] = false;
	_skipRenderPass[1] = false;
	rendererBoundingBox = nullptr;

	this->posX = -999;
	setPosition(posX, posY, posZ);
	needsUpdate = false;
	queuedForUpdate = false;

#ifdef PS2_PLATFORM
	for (int_t p = 0; p < 2; ++p)
	{
		ps2VertexCount[p] = 0;
		ps2DrawMode[p] = 7;
		ps2HasTexture[p] = false;
		ps2HasColor[p] = false;
		ps2HasNormals[p] = false;
		ps2BuildVertexCount[p] = 0;
		ps2BuildDrawMode[p] = 7;
		ps2BuildHasTexture[p] = false;
		ps2BuildHasColor[p] = false;
		ps2BuildHasNormals[p] = false;
		ps2BuildDrew[p] = false;
	}
	ps2BuildActive = false;
	ps2BuildStagingSlot = RENDER_TERRAIN_STAGING_INVALID_SLOT;
	ps2BuildSourceAvailability = 0u;
	ps2BuildSourceAvailabilityValid = false;
	ps2MissingNeighbourMask = 0u;
	ps2BuildPass = 0;
	ps2BuildCursor = 0;
	ps2BuildGreedyFace = 0;
	ps2BuildGreedySlice = 0;
	ps2BuildHasPass1 = false;
	ps2BuildDirtyDuringBuild = false;
	ps2BuildOpaqueBits.fill(0);
	ps2BuildOpaqueCount = 0;
	for (int_t face = 0; face < 6; ++face)
		ps2PublishedVisibility[face] = 0x3f;
	ps2CpuVisible = true;
	ps2StepDidWork = false;
	renderTerrainCacheReset(ps2TerrainCache);
#endif
}

WorldRenderer::~WorldRenderer()
{
	cleanup();
#ifdef PS2_PLATFORM
	renderTerrainCacheDestroy(ps2TerrainCache);
#endif
}

void WorldRenderer::removeTileEntityRenderersFromGlobalList()
{
#if PLATFORM_PC_LEGACY
	pcLegacyStaticTileEntityClearOwner(this);
	pcLegacyStaticTileEntityRenderers.clear();
#endif
	// RenderGlobal::tileEntities is a non-owning render list.  A WorldRenderer is
	// the owner of the references it contributed to that list.  When the renderer
	// is recycled, disabled or destroyed, those references must be removed before
	// the world/chunk code can invalidate and delete the TileEntity objects.
	for (TileEntity *te : tileEntityRenderers)
		eraseAllTileEntityRefs(tileEntities, te);
	tileEntityRenderers.clear();
#ifdef PS2_PLATFORM
	ps2BuildTileEntityRenderers.clear();
#endif
#ifdef WII_PLATFORM
	wiiBuildTileEntityRenderers.clear();
#endif
}

void WorldRenderer::cleanup()
{
#ifdef WII_PLATFORM
	// Native GX handles are renderer-owned. Any compatibility list block was
	// allocated lazily by this renderer and is released below after contents are
	// detached from the active world position.
#else
	// Display lists and occlusion queries come from ranges owned by
	// RenderGlobal. Deleting individual entries frees names that are reused by
	// new WorldRenderers and may then collide with model or sky display lists.
#endif
	setDontDraw();

	// rendererBoundingBox is created with AxisAlignedBB::getBoundingBox(), which
	// returns a heap allocation. Repositioning a renderer used to overwrite this
	// pointer and leak one AABB every time chunks were recycled around the player.
	delete rendererBoundingBox;
	rendererBoundingBox = nullptr;

	worldObj = nullptr;
	queuedForUpdate = false;

#ifdef PS2_PLATFORM
	// Abandon any build in progress first: the staging pair is pool storage, so
	// it must go back rather than be freed with the renderer.
	ps2ResetBuildState();
	for (int_t p = 0; p < 2; ++p)
	{
		std::vector<int_t>().swap(ps2RawBuffer[p]);
		ps2VertexCount[p] = 0;
	}
	renderTerrainCacheRelease(ps2TerrainCache);
#endif

#ifdef WII_PLATFORM
	renderTerrainChunkHandlesDestroy(terrainChunkHandles);
#endif

#if PLATFORM_PC || PLATFORM_PSP
        if (glRenderList != 0)
        {
                GLAllocation::deleteDisplayLists(glRenderList);
                glRenderList = 0;
        }
#endif
#if PLATFORM_PC
	glOcclusionQuery = 0;
#endif
}

void WorldRenderer::setPosition(int_t x, int_t y, int_t z)
{
	if (x == posX && y == posY && z == posZ)
		return;

	setDontDraw();
	posX = x;
	posY = y;
	posZ = z;
	posXPlus = x + sizeWidth  / 2;
	posYPlus = y + sizeHeight / 2;
	posZPlus = z + sizeDepth  / 2;
	posXClip = x & 0x3ff;
	posYClip = y;
	posZClip = z & 0x3ff;
	posXMinus = x - posXClip;
	posYMinus = y - posYClip;
	posZMinus = z - posZClip;

	// Desktop C6 tests the exact section bounds so Fancy Occlusion can classify
	// fully-contained chunks accurately. Console backends keep their tuned
	// conservative margin because their fixed grids and clip paths are different.
	const float f = rendererAabbMargin();
	if (rendererBoundingBox == nullptr)
	{
		rendererBoundingBox = AxisAlignedBB::getBoundingBox(
			(float)x - f, (float)y - f, (float)z - f,
			(float)(x + sizeWidth) + f, (float)(y + sizeHeight) + f, (float)(z + sizeDepth) + f);
	}
	else
	{
		rendererBoundingBox->setBounds(
			(float)x - f, (float)y - f, (float)z - f,
			(float)(x + sizeWidth) + f, (float)(y + sizeHeight) + f, (float)(z + sizeDepth) + f);
	}

	// OptiFine C6 defers rebuilding the occlusion AABB list until this section
	// actually rebuilds. Repositioning a renderer grid can touch hundreds of
	// sections at once; compiling a list for every moved section here creates a
	// large synchronous spike before any useful terrain work begins.
#if PLATFORM_PC
	needsOcclusionBoxUpdate = true;
	isVisibleFromPosition = false;
#endif

	markDirty();
}

#if PLATFORM_PC
void WorldRenderer::updateOcclusionBox()
{
	if (!needsOcclusionBoxUpdate)
		return;

	const float f = rendererAabbMargin();
	renderBeginDisplayList(glRenderList + 2);
	RenderItem::renderAABB(AxisAlignedBB::getBoundingBoxFromPool(
		(float)posXClip - f, (float)posYClip - f, (float)posZClip - f,
		(float)(posXClip + sizeWidth) + f, (float)(posYClip + sizeHeight) + f, (float)(posZClip + sizeDepth) + f));
	renderEndDisplayList();
	needsOcclusionBoxUpdate = false;
}
#endif

void WorldRenderer::updateInFrustrum(ICamera *icamera)
{
#if PLATFORM_PS2
	// PS2 needs the three-state result for its per-section clip fast path.
	const int cls = icamera->classifyBoundingBox(rendererBoundingBox);
	isInFrustum = (cls != 0);
	isFullyInFrustum = (cls == 2);
#elif PLATFORM_WII
	// GX has no occlusion queries, so the stronger fully-inside classification
	// is dead work here. A plain frustum test is the complete Wii contract.
	isInFrustum = icamera->isBoundingBoxInFrustum(rendererBoundingBox);
#else
	if (Config::isOcclusionFancy())
	{
		const int cls = icamera->classifyBoundingBox(rendererBoundingBox);
		isInFrustum = (cls != 0);
		isFullyInFrustum = (cls == 2);
	}
	else
	{
		isInFrustum = icamera->isBoundingBoxInFrustum(rendererBoundingBox);
		isFullyInFrustum = false;
	}
#endif
}




#if PLATFORM_PSP
void WorldRenderer::pspResetBuildState()
{
	pspBuildActive = false;
	pspBuildSourceAvailability = 0u;
	pspBuildSourceAvailabilityValid = false;
	pspBuildPass = 0;
	pspBuildCursor = 0;
	pspBuildHasPass1 = false;
	pspStepDidWork = false;
	pspBuildTileEntityRenderers.clear();
	for (int_t p = 0; p < 2; ++p)
	{
		pspBuildMeshes[p].clear();
		pspBuildExtraTextureMeshes[p].clear();
		pspBuildDrew[p] = false;
	}
}

bool WorldRenderer::pspBuildRendererStep(int_t blockBudget)
{
	pspStepDidWork = false;
	if (worldObj == nullptr)
	{
		pspResetBuildState();
		needsUpdate = false;
		return true;
	}

	const int_t x0 = posX;
	const int_t y0 = posY;
	const int_t z0 = posZ;
	const int_t totalBlocks = sizeWidth * sizeHeight * sizeDepth;

	const int_t centerChunkX = JavaArithmetic::intShr(x0, 4);
    const int_t centerChunkZ = JavaArithmetic::intShr(z0, 4);
    if (!worldObj->chunkExists(centerChunkX, centerChunkZ))
        return false;

    unsigned int sourceAvailability = 0;
    {
        const int_t ccx0 = JavaArithmetic::intShr(x0 - 1, 4);
        const int_t ccx1 = JavaArithmetic::intShr(x0 + sizeWidth + 1, 4);
        const int_t ccz0 = JavaArithmetic::intShr(z0 - 1, 4);
        const int_t ccz1 = JavaArithmetic::intShr(z0 + sizeDepth + 1, 4);
        unsigned int sourceBit = 1u;
        for (int_t ccx = ccx0; ccx <= ccx1; ++ccx)
        {
            for (int_t ccz = ccz0; ccz <= ccz1; ++ccz, sourceBit <<= 1)
            {
                if (worldObj->chunkExists(ccx, ccz))
                    sourceAvailability |= sourceBit;
            }
        }
    }

	if (!pspBuildActive)
	{
		pspResetBuildState();
		pspBuildActive = true;
		pspBuildSourceAvailability = sourceAvailability;
		pspBuildSourceAvailabilityValid = true;
	}

	if (blockBudget <= 0 || urgentRebuild)
		blockBudget = totalBlocks;
	const uint64_t stepStartUs = PlatformCompat::getMonotonicMicros();
	int_t processed = 0;

	while (pspBuildPass < 2)
	{
		if (pspBuildPass == 1 && !pspBuildHasPass1)
		{
			pspBuildPass = 2;
			pspBuildCursor = 0;
			break;
		}

		Chunk::isLit = false;
		ChunkCache chunkcache(worldObj, x0 - 1, y0 - 1, z0 - 1,
			x0 + sizeWidth + 1, y0 + sizeHeight + 1, z0 + sizeDepth + 1);
		RenderBlocks renderblocks(&chunkcache);
		Tessellator *tessellator = &Tessellator::instance;
		tessellator->startDrawingQuads();
		tessellator->setTranslationD(-(double)posX, -(double)posY, -(double)posZ);
		bool stepDrew = false;
		int_t blockLoops = 0;
		int_t renderCalls = 0;
		int_t standardBlocks = 0;
		int_t specialBlocks = 0;
		int_t hiddenBlocks = 0;
		int_t visibleBlocks = 0;

		while (pspBuildCursor < totalBlocks && processed < blockBudget)
		{
			const int_t cursor = pspBuildCursor++;
			const int_t lx = cursor % sizeWidth;
			const int_t yz = cursor / sizeWidth;
			const int_t lz = yz % sizeDepth;
			const int_t ly = yz / sizeDepth;
			const int_t x = x0 + lx;
			const int_t y = y0 + ly;
			const int_t z = z0 + lz;
			++processed;
			++blockLoops;

			const int_t id = chunkcache.getBlockId(x, y, z);
			if (id <= 0)
				continue;
			Block *block = Block::blocksList[id];
			if (block == nullptr)
				continue;

			if (pspBuildPass == 0 && Block::isBlockContainer[id])
			{
				TileEntity *te = chunkcache.getBlockTileEntity(x, y, z);
				if (te != nullptr && TileEntityRenderer::instance.hasSpecialRenderer(te) &&
					std::find(pspBuildTileEntityRenderers.begin(), pspBuildTileEntityRenderers.end(), te) == pspBuildTileEntityRenderers.end())
					pspBuildTileEntityRenderers.push_back(te);
			}

			const int_t blockPass = block->getRenderBlockPass();
			if (pspBuildPass == 0 && blockPass != 0)
				pspBuildHasPass1 = true;
			if (blockPass != pspBuildPass)
				continue;

			const int_t renderType = block->getRenderType();
			if (renderType == 0)
			{
				renderblocks.preparePspOpaqueFaceMask(block, x, y, z, renderType);
				if (renderblocks.isFullyOccludedOpaqueCube(block, x, y, z, renderType))
				{
					++hiddenBlocks;
					renderblocks.clearPspOpaqueFaceMask();
					continue;
				}
				++visibleBlocks;
				++standardBlocks;
			}
			else
			{
				++specialBlocks;
				platformProfilePspMeshSpecialType(renderType);
			}
			++renderCalls;
			const uint64_t categoryStart = PlatformCompat::getMonotonicMicros();
#if PLATFORM_PSP_ISOLATE_MESH_GEOMETRY
			(void)block;
			(void)x;
			(void)y;
			(void)z;
			(void)renderType;
#else
			stepDrew |= renderblocks.renderBlockByRenderType(block, x, y, z, renderType);
#endif
			const int category = renderType == 0 ? 0 :
				(renderType == 4 ? 1 :
				((renderType == 1 || renderType == 6) ? 2 :
				(renderType == 10 ? 3 : 4)));
			const uint64_t categoryEnd = PlatformCompat::getMonotonicMicros();
			platformProfilePspMeshCategory(category,
				categoryEnd >= categoryStart
					? static_cast<long long>(categoryEnd - categoryStart) * 1000LL : 0);
			renderblocks.clearPspOpaqueFaceMask();

			if (!urgentRebuild && (processed % PLATFORM_CHUNK_BUILD_TIME_CHECK_BLOCKS) == 0 &&
				PLATFORM_CHUNK_BUILD_BUDGET_MS > 0)
			{
				const uint64_t nowUs = PlatformCompat::getMonotonicMicros();
				if (nowUs > stepStartUs &&
					nowUs - stepStartUs >= static_cast<uint64_t>(PLATFORM_CHUNK_BUILD_BUDGET_MS) * 1000ULL)
					break;
			}
		}

		const int_t verticesBeforeCapture = pspBuildMeshes[pspBuildPass].vertexCount;
#if PLATFORM_PSP_ISOLATE_MESH_CAPTURE
		RenderCapturedMesh discardedMesh;
		(void)tessellator->capture(discardedMesh, false);
		for (TessellatorTextureMesh &group : pspBuildExtraTextureMeshes[pspBuildPass])
			group.mesh.clear();
		pspBuildExtraTextureMeshes[pspBuildPass].clear();
		pspBuildDrew[pspBuildPass] = false;
#else
		const bool captured = tessellator->capture(pspBuildMeshes[pspBuildPass], true);
		tessellator->captureTextureGroups(pspBuildExtraTextureMeshes[pspBuildPass], true);
		if (captured)
			pspBuildDrew[pspBuildPass] = true;
		pspBuildDrew[pspBuildPass] = pspBuildDrew[pspBuildPass] || stepDrew;
#endif
		platformProfilePspMeshCounters(blockLoops, 0, 0, renderCalls,
			pspBuildMeshes[pspBuildPass].vertexCount - verticesBeforeCapture,
			standardBlocks, specialBlocks, hiddenBlocks, visibleBlocks);
		tessellator->setTranslationD(0.0, 0.0, 0.0);
		pspStepDidWork = processed > 0;

		if (pspBuildCursor < totalBlocks)
                        return false;

                if (pspBuildPass == 0 && !pspBuildHasPass1)
                        pspBuildPass = 2;
                else
                {
                        ++pspBuildPass;
                        pspBuildCursor = 0;
                }

                if (pspBuildPass < 2 && processed >= blockBudget)
                        return false;
	}

	for (int_t pass = 0; pass < 2; ++pass)
	{
		_skipRenderPass[pass] = !pspBuildDrew[pass];
		if (!pspBuildDrew[pass])
			continue;
#if PLATFORM_PSP_ISOLATE_MESH_PUBLICATION
		pspBuildMeshes[pass].clear();
		pspBuildExtraTextureMeshes[pass].clear();
		_skipRenderPass[pass] = true;
		continue;
#else
		renderPreparePspTerrainMesh(pspBuildMeshes[pass], 0.0f, 0.0f, 0.0f);
                renderBeginDisplayList(glRenderList + pass);
                renderPushMatrix();
                renderTranslate((float)posXClip, (float)posYClip, (float)posZClip);
                (void)renderRecordCapturedMesh(std::move(pspBuildMeshes[pass]));
                for (TessellatorTextureMesh &group : pspBuildExtraTextureMeshes[pass])
                {
                        renderPreparePspTerrainMesh(group.mesh, 0.0f, 0.0f, 0.0f);
                        renderBindTexture(group.textureId);
                        (void)renderRecordCapturedMesh(std::move(group.mesh));
                }
                if (!pspBuildExtraTextureMeshes[pass].empty())
                        renderBindTexture(ConnectedTextures::getTerrainTextureId());
                renderPopMatrix();
                renderEndDisplayList();
#endif
	}

	removeTileEntityRenderersFromGlobalList();
	for (TileEntity *te : pspBuildTileEntityRenderers)
	{
		if (std::find(tileEntityRenderers.begin(), tileEntityRenderers.end(), te) == tileEntityRenderers.end())
			pushUniqueTileEntityRef(&tileEntityRenderers, te);
		if (std::find(tileEntities->begin(), tileEntities->end(), te) == tileEntities->end())
			tileEntities->push_back(te);
	}
	isChunkLit = Chunk::isLit;
	isInitialized = true;
	needsUpdate = false;
	pspResetBuildState();
	return true;
}
#endif

#if !defined(PS2_PLATFORM) && !defined(WII_PLATFORM) && !PLATFORM_PC_LEGACY
void WorldRenderer::updateRenderer()
{
	if (!needsUpdate)
		return;

#if PLATFORM_PSP
	(void)pspBuildRendererStep(PLATFORM_CHUNK_BUILD_BLOCKS_PER_STEP);
	pspLastStepDidWork = pspStepDidWork;
	return;
#endif

#if PLATFORM_PC
	updateOcclusionBox();
	isVisibleFromPosition = false;
#endif

	{
		const int_t ccx0 = JavaArithmetic::intShr(posX - 1, 4);
		const int_t ccx1 = JavaArithmetic::intShr(posX + sizeWidth + 1, 4);
		const int_t ccz0 = JavaArithmetic::intShr(posZ - 1, 4);
		const int_t ccz1 = JavaArithmetic::intShr(posZ + sizeDepth + 1, 4);
		for (int_t ccx = ccx0; ccx <= ccx1; ++ccx)
		{
			for (int_t ccz = ccz0; ccz <= ccz1; ++ccz)
			{
				if (worldObj->isChunkInLoadRadius(ccx, ccz))
				{
					if (!worldObj->chunkExists(ccx, ccz))
						worldObj->getChunkFromChunkCoords(ccx, ccz);
					if (!worldObj->chunkExists(ccx, ccz))
						return;
				}
			}
		}
	}

	int_t x0 = posX,             y0 = posY,              z0 = posZ;
	int_t x1 = posX + sizeWidth, y1 = posY + sizeHeight, z1 = posZ + sizeDepth;

	chunksUpdated++;

	for (int_t k1 = 0; k1 < 2; k1++)
		_skipRenderPass[k1] = true;

	Chunk::isLit = false;

	std::vector<TileEntity *> rebuiltTileEntityRenderers;

	int_t margin = 1;
	ChunkCache chunkcache(worldObj, x0 - margin, y0 - margin, z0 - margin,
	                                x1 + margin, y1 + margin, z1 + margin);

	RenderBlocks renderblocks(&chunkcache);
	Tessellator *tessellator = &Tessellator::instance;

	for (int_t pass = 0; pass < 2; )
	{
		bool hasOtherPass = false;
		bool drewAnything = false;
		bool listOpen = false;

		for (int_t y = y0; y < y1; y++)
		{
			for (int_t z = z0; z < z1; z++)
			{
				for (int_t x = x0; x < x1; x++)
				{
					int_t id = chunkcache.getBlockId(x, y, z);
					int_t blockPass = id > 0 && Block::blocksList[id] != nullptr
						? Block::blocksList[id]->getRenderBlockPass() : 0;
					if (id <= 0)
						continue;

					if (!listOpen)
					{
						listOpen = true;
						renderBeginDisplayList(glRenderList + pass);
						renderPushMatrix();
						renderTranslate((float)posXClip, (float)posYClip, (float)posZClip);
						float f = 1.000001f;
						renderTranslate(-(float)sizeDepth / 2.0f, -(float)sizeHeight / 2.0f, -(float)sizeDepth / 2.0f);
						renderScale(f, f, f);
						renderTranslate( (float)sizeDepth / 2.0f,  (float)sizeHeight / 2.0f,  (float)sizeDepth / 2.0f);
						tessellator->startDrawingQuads();
						tessellator->setTranslationD(-(double)posX, -(double)posY, -(double)posZ);
					}

					if (pass == 0 && Block::isBlockContainer[id])
					{
						TileEntity *te = chunkcache.getBlockTileEntity(x, y, z);
						if (te != nullptr && TileEntityRenderer::instance.hasSpecialRenderer(te) &&
							std::find(rebuiltTileEntityRenderers.begin(), rebuiltTileEntityRenderers.end(), te) == rebuiltTileEntityRenderers.end())
							rebuiltTileEntityRenderers.push_back(te);
					}

					if (blockPass != pass)
					{
						hasOtherPass = true;
						continue;
					}

					Block *block = Block::blocksList[id];
					if (block != nullptr)
						drewAnything |= renderblocks.renderBlockByRenderType(block, x, y, z);
				}
			}
		}

		if (listOpen)
		{
			tessellator->draw();
			renderPopMatrix();
			renderEndDisplayList();
			tessellator->setTranslationD(0.0, 0.0, 0.0);
		}
		else
		{
			drewAnything = false;
		}

		if (drewAnything)
			_skipRenderPass[pass] = false;

		if (!hasOtherPass)
			break;

		pass++;
	}

	for (TileEntity *te : rebuiltTileEntityRenderers)
	{
		if (std::find(tileEntityRenderers.begin(), tileEntityRenderers.end(), te) == tileEntityRenderers.end())
			pushUniqueTileEntityRef(tileEntities, te);
	}
	for (TileEntity *te : tileEntityRenderers)
	{
		if (std::find(rebuiltTileEntityRenderers.begin(), rebuiltTileEntityRenderers.end(), te) == rebuiltTileEntityRenderers.end())
		{
			eraseAllTileEntityRefs(tileEntities, te);
		}
	}

	isChunkLit  = Chunk::isLit;
	isInitialized = true;
	tileEntityRenderers = rebuiltTileEntityRenderers;
	needsUpdate = false;
}
#endif

void WorldRenderer::markDirty()
{
#if PLATFORM_PSP
	const bool wasBuilding = pspBuildActive;
	if (pspBuildActive)
		pspResetBuildState();
	int invalidationSource = 3;
	if (worldObj != nullptr && worldObj->isMarkingFromLighting())
		invalidationSource = 1;
	else if (worldObj != nullptr &&
		worldObj->isChunkPopulationPendingForRendering(JavaArithmetic::intShr(posX, 4), JavaArithmetic::intShr(posZ, 4)))
		invalidationSource = 2;
	else if (worldObj != nullptr && worldObj->isMarkingFromPlayerEdit())
		invalidationSource = 0;
	platformProfilePspMeshInvalidation(needsUpdate, wasBuilding, invalidationSource);
#endif
#if PLATFORM_PC_LEGACY
#if PC_LEGACY_COALESCE_MESH_REBUILDS
	if (pcLegacyBuildActive)
	{
		const int_t chunkX = JavaArithmetic::intShr(posX, 4);
		const int_t chunkZ = JavaArithmetic::intShr(posZ, 4);
		if (worldObj != nullptr && worldObj->isChunkPopulationPendingForRendering(chunkX, chunkZ))
			pcLegacyBuildDirtyDuringBuild = true;
		else
			pcLegacyResetBuildState();
	}
#else
	pcLegacyResetBuildState();
#endif
#endif
#ifdef PS2_PLATFORM
#if PLATFORM_COALESCE_MESH_REBUILDS
	if (ps2BuildActive)
	{
		const int_t chunkX = JavaArithmetic::intShr(posX, 4);
		const int_t chunkZ = JavaArithmetic::intShr(posZ, 4);
		if (worldObj != nullptr && worldObj->isChunkPopulationPendingForRendering(chunkX, chunkZ))
		{
			ps2BuildDirtyDuringBuild = true;
#ifdef PS2_OPTIMIZATION_VALIDATION
			Ps2OptimizationValidation::meshDirtyCoalesced();
#endif
		}
		else
		{
#ifdef PS2_OPTIMIZATION_VALIDATION
			Ps2OptimizationValidation::meshDirtyRestarted();
#endif
#if MC_LOG_LEVEL > 2
			platformProfileMeshReset(PlatformMeshResetReason::DirtyRestart);
#endif
			ps2ResetBuildState();
		}
	}
#else
#ifdef PS2_OPTIMIZATION_VALIDATION
	if (ps2BuildActive)
		Ps2OptimizationValidation::meshDirtyRestarted();
#endif
#if MC_LOG_LEVEL > 2
	if (ps2BuildActive)
		platformProfileMeshReset(PlatformMeshResetReason::DirtyRestart);
#endif
	ps2ResetBuildState();
#endif
#endif
#ifdef WII_PLATFORM
#if PLATFORM_COALESCE_MESH_REBUILDS
	if (wiiBuildActive)
	{
		const int_t chunkX = JavaArithmetic::intShr(posX, 4);
		const int_t chunkZ = JavaArithmetic::intShr(posZ, 4);
		if (worldObj != nullptr && worldObj->isChunkPopulationPendingForRendering(chunkX, chunkZ))
			wiiBuildDirtyDuringBuild = true;
		else
		{
			wiiResetBuildState();
			renderTerrainChunkHandlesClearStaging(terrainChunkHandles);
		}
	}
#else
	wiiResetBuildState();
	renderTerrainChunkHandlesClearStaging(terrainChunkHandles);
#endif
#endif
	needsUpdate = true;
}

#if WII_PLATFORM || PS2_PLATFORM || PLATFORM_PC_LEGACY
void WorldRenderer::markDirtyFromLighting()
{
#if PLATFORM_COALESCE_MESH_REBUILDS
#if PLATFORM_PC_LEGACY
	if (pcLegacyBuildActive)
	{
		pcLegacyBuildDirtyDuringBuild = true;
		needsUpdate = true;
		return;
	}
#endif
#ifdef PS2_PLATFORM
	if (ps2BuildActive)
	{
		ps2BuildDirtyDuringBuild = true;
#ifdef PS2_OPTIMIZATION_VALIDATION
		Ps2OptimizationValidation::meshDirtyCoalesced();
#endif
		needsUpdate = true;
		return;
	}
#endif
#ifdef WII_PLATFORM
	if (wiiBuildActive)
	{
		wiiBuildDirtyDuringBuild = true;
		needsUpdate = true;
		return;
	}
#endif
#endif
	markDirty();
}
#endif

void WorldRenderer::setDontDraw()
{
	#if WII_PLATFORM || PS2_PLATFORM || PLATFORM_PC_LEGACY
	// Whatever edit marked this renderer urgent was at its old position.
	urgentRebuild = false;
	#endif
#if PLATFORM_PC_LEGACY
	pcLegacyResetBuildState();
	for (int_t face = 0; face < 6; ++face)
		pcLegacyPublishedVisibility[face] = 0x3f;
	pcLegacyCpuVisible = true;
#endif
	removeTileEntityRenderersFromGlobalList();
#ifdef WII_PLATFORM
	// A renderer is about to be recycled for another world position. Keeping its
	// old, now invisible lists until the replacement can be built creates a
	// deadlock at the GX cap: no new list fits because stale lists own the cache,
	// yet no stale list is released because rebuilding is blocked. Release the
	// commands now while retaining the IDs for this renderer.
	releaseDisplayListsForCache();
	wiiResetBuildState();
	renderTerrainChunkHandlesClearStaging(terrainChunkHandles);
#else
	_skipRenderPass[0] = true;
	_skipRenderPass[1] = true;
#if PLATFORM_PSP
	pspResetBuildState();
#endif
#endif
#ifdef PS2_PLATFORM
	// clear(), NOT a swap-to-empty.
	//
	// This runs from setPosition(), i.e. every time the sliding renderer window
	// re-targets a section. On the 5x3x5 grid a horizontal chunk crossing can
	// recycle one 5x3 renderer plane at once.
	// Releasing the storage there and letting the rebuild grow it back by
	// doubling emitted a continuous stream of odd tens-of-KB malloc/free pairs,
	// which is the fragmentation this was supposed to avoid: measured
	// 2026-07-27, mallocFree sat at a steady ~4.5MB of free-but-unusable blocks
	// while the arena kept sbrk'ing (heap 0x01cd1000 -> 0x01f9b000) until
	// `free` hit 0 and the game hit Out of memory -- with mallocUsed itself only
	// up ~2MB over the same window. The memory was there; it was in the wrong
	// shaped holes.
	//
	// Normal capacities remain reusable because each slot keeps its vertical
	// band across repositions. Only an unusually large high-water allocation is
	// released here; that prevents one dense section from permanently inflating
	// the 75-renderer grid without returning to per-rebuild shrink/copy churn.
	// detachFromWorld() still releases every remaining allocation.
	// The staging pair is not cleared here: ps2ResetBuildState() below hands it
	// back to the pool, which empties it. Retention applies to the live buffers
	// this renderer owns, never to borrowed ones.
	for (int_t p = 0; p < 2; ++p)
	{
		const size_t retainedBytes = ps2RawBuffer[p].capacity() * sizeof(int_t);
		if (retainedBytes > PS2_MAX_RETAINED_RAW_MESH_BYTES)
			std::vector<int_t>().swap(ps2RawBuffer[p]);
		else
			ps2RawBuffer[p].clear();
	}

	if (renderTerrainCacheRamBytes(ps2TerrainCache) > PS2_MAX_RETAINED_PACKED_MESH_BYTES)
		renderTerrainCacheRelease(ps2TerrainCache);
	else
		renderTerrainCacheReset(ps2TerrainCache);
	ps2VertexCount[0] = 0;
	ps2VertexCount[1] = 0;
	ps2MissingNeighbourMask = 0u;
	for (int_t face = 0; face < 6; ++face)
		ps2PublishedVisibility[face] = 0x3f;
	ps2CpuVisible = true;
	// The face ranges describe a mesh that no longer exists. Leaving them valid
	// would let a repositioned renderer index into the next build's buffer with
	// the previous section's offsets.
#if MC_LOG_LEVEL > 2
	if (ps2BuildActive)
		platformProfileMeshReset(PlatformMeshResetReason::Recycle);
#endif
	ps2ResetBuildState();
#endif
	isInFrustum = false;
#if PLATFORM_PC || PLATFORM_PS2
	isFullyInFrustum = false;
#endif
#if PLATFORM_PC
	isVisibleFromPosition = false;
#endif
	isInitialized = false;
}

void WorldRenderer::detachFromWorld()
{
	setDontDraw();
#ifdef PS2_PLATFORM
	// Leaving the world for good. setDontDraw() deliberately keeps the mesh
	// storage so a repositioned renderer can reuse it; here there is nothing
	// left to reuse it for, so hand it back. The staging pair was already
	// returned to the pool by setDontDraw()'s ps2ResetBuildState().
	std::vector<int_t>().swap(ps2RawBuffer[0]);
	std::vector<int_t>().swap(ps2RawBuffer[1]);
	renderTerrainCacheRelease(ps2TerrainCache);
#endif
	worldObj = nullptr;
}

#if PLATFORM_PC
void WorldRenderer::callOcclusionQueryList()
{
	renderCallDisplayList(glRenderList + 2);
}
#endif

#if PLATFORM_PC || PLATFORM_PSP
int_t WorldRenderer::getGLCallListForPass(int_t pass)
{
	if (!isInFrustum)
		return -1;
	if (!_skipRenderPass[pass])
		return glRenderList + pass;
	return -1;
}
#endif


bool WorldRenderer::skipAllRenderPasses()
{
	if (!isInitialized)
		return false;
	return _skipRenderPass[0] && _skipRenderPass[1];
}

bool WorldRenderer::skipRenderPass(int_t pass)
{
	if (pass < 0 || pass > 1)
		return true;
	return _skipRenderPass[pass];
}

// Narrow first, subtract second. The result only ever ranks renderers (the
// EntitySorter comparator, the mesh-budget sort) or meets a 256.0f threshold,
// and it is already returned as a float, so the double subtraction bought
// nothing -- while costing three libgcc calls per axis on a CPU with no double
// FPU, several hundred times per frame from inside a sort comparator.

float WorldRenderer::distanceToEntitySquared(Entity *entity)
{
	if (entity == nullptr)
		return 0.0f;
	float dx = (float)entity->posX - (float)posXPlus;
	float dy = (float)entity->posY - (float)posYPlus;
	float dz = (float)entity->posZ - (float)posZPlus;
	return dx * dx + dy * dy + dz * dz;
}

#include "BlockRedstoneTorch.h"
#include "Block.h"
#include "World.h"
#include "IBlockAccess.h"
#include "java/Arithmetic.h"

std::vector<RedstoneUpdateInfo> BlockRedstoneTorch::torchUpdates;

#if PLATFORM_PSP
namespace
{
constexpr std::size_t kPspTorchUpdateCapacity = 256;
RedstoneUpdateInfo pspTorchUpdates[kPspTorchUpdateCapacity] = {};
std::size_t pspTorchUpdateCount = 0;
}
#endif

BlockRedstoneTorch::BlockRedstoneTorch(int_t i, int_t j, bool flag) : BlockTorch(i, j)
{
    torchActive = false;
    torchActive = flag;
    setTickOnLoad(true);
}

int_t BlockRedstoneTorch::getBlockTextureFromSideAndMetadata(int_t i, int_t j)
{
    if (i == 1) return Block::redstoneWire->getBlockTextureFromSideAndMetadata(i, j);
    return BlockTorch::getBlockTextureFromSideAndMetadata(i, j);
}

bool BlockRedstoneTorch::checkForBurnout(World *world, int_t i, int_t j, int_t k, bool flag)
{
    const long_t worldTime = world->getWorldTime();
#if PLATFORM_PSP
    std::size_t writeIndex = 0;
    for (std::size_t readIndex = 0; readIndex < pspTorchUpdateCount; ++readIndex)
    {
        if (JavaArithmetic::longSub(worldTime, pspTorchUpdates[readIndex].updateTime) <= 60LL)
            pspTorchUpdates[writeIndex++] = pspTorchUpdates[readIndex];
    }
    pspTorchUpdateCount = writeIndex;

    if (flag)
    {
        if (pspTorchUpdateCount == kPspTorchUpdateCapacity)
        {
            for (std::size_t index = 1; index < pspTorchUpdateCount; ++index)
                pspTorchUpdates[index - 1] = pspTorchUpdates[index];
            --pspTorchUpdateCount;
        }
        pspTorchUpdates[pspTorchUpdateCount++] = RedstoneUpdateInfo(i, j, k, worldTime);
    }

    int_t count = 0;
    for (std::size_t index = 0; index < pspTorchUpdateCount; ++index)
    {
        const RedstoneUpdateInfo &info = pspTorchUpdates[index];
        if (info.x == i && info.y == j && info.z == k && ++count >= 8)
            return true;
    }
    return false;
#else
    while (!torchUpdates.empty() &&
           JavaArithmetic::longSub(worldTime, torchUpdates.front().updateTime) > 60LL)
        torchUpdates.erase(torchUpdates.begin());

    if (flag)
    {
        // Neighbor cascades can enqueue many torch updates before updateTick()
        // gets a chance to perform its normal expiry pass. Keep this legacy
        // burnout history bounded on fixed-memory platforms.
        constexpr std::size_t maxTorchUpdates = 1024;
        if (torchUpdates.capacity() < maxTorchUpdates)
            torchUpdates.reserve(maxTorchUpdates);
        if (torchUpdates.size() >= maxTorchUpdates)
            torchUpdates.erase(torchUpdates.begin());
        torchUpdates.push_back(RedstoneUpdateInfo(i, j, k, worldTime));
    }
    int_t l = 0;
    for (int_t i1 = 0; i1 < (int_t)torchUpdates.size(); i1++)
    {
        RedstoneUpdateInfo &info = torchUpdates[i1];
        if (info.x == i && info.y == j && info.z == k && ++l >= 8)
            return true;
    }
    return false;
#endif
}

int_t BlockRedstoneTorch::tickRate() { return 2; }

void BlockRedstoneTorch::onBlockAdded(World *world, int_t i, int_t j, int_t k)
{
    if (world->getBlockMetadata(i, j, k) == 0)
        BlockTorch::onBlockAdded(world, i, j, k);
    if (torchActive)
    {
        world->notifyBlocksOfNeighborChange(i, j - 1, k, blockID);
        world->notifyBlocksOfNeighborChange(i, j + 1, k, blockID);
        world->notifyBlocksOfNeighborChange(i - 1, j, k, blockID);
        world->notifyBlocksOfNeighborChange(i + 1, j, k, blockID);
        world->notifyBlocksOfNeighborChange(i, j, k - 1, blockID);
        world->notifyBlocksOfNeighborChange(i, j, k + 1, blockID);
    }
}

void BlockRedstoneTorch::onBlockRemoval(World *world, int_t i, int_t j, int_t k)
{
    if (torchActive)
    {
        world->notifyBlocksOfNeighborChange(i, j - 1, k, blockID);
        world->notifyBlocksOfNeighborChange(i, j + 1, k, blockID);
        world->notifyBlocksOfNeighborChange(i - 1, j, k, blockID);
        world->notifyBlocksOfNeighborChange(i + 1, j, k, blockID);
        world->notifyBlocksOfNeighborChange(i, j, k - 1, blockID);
        world->notifyBlocksOfNeighborChange(i, j, k + 1, blockID);
    }
}

bool BlockRedstoneTorch::isPoweringTo(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
#if PLATFORM_PSP_ISOLATE_REDSTONE_POWER
    (void)iblockaccess;
    (void)i;
    (void)j;
    (void)k;
    (void)l;
    return false;
#else
    if (!torchActive) return false;
    int_t i1 = iblockaccess->getBlockMetadata(i, j, k);
    if (i1 == 5 && l == 1) return false;
    if (i1 == 3 && l == 3) return false;
    if (i1 == 4 && l == 2) return false;
    if (i1 == 1 && l == 5) return false;
    return i1 != 2 || l != 4;
#endif
}

bool BlockRedstoneTorch::getFlowDecay(World *world, int_t i, int_t j, int_t k)
{
    int_t l = world->getBlockMetadata(i, j, k);
    if (l == 5 && world->isBlockIndirectlyProvidingPowerTo(i, j - 1, k, 0)) return true;
    if (l == 3 && world->isBlockIndirectlyProvidingPowerTo(i, j, k - 1, 2)) return true;
    if (l == 4 && world->isBlockIndirectlyProvidingPowerTo(i, j, k + 1, 3)) return true;
    if (l == 1 && world->isBlockIndirectlyProvidingPowerTo(i - 1, j, k, 4)) return true;
    return l == 2 && world->isBlockIndirectlyProvidingPowerTo(i + 1, j, k, 5);
}

void BlockRedstoneTorch::updateTick(World *world, int_t i, int_t j, int_t k, Random &random)
{
    bool flag = getFlowDecay(world, i, j, k);
    while (!torchUpdates.empty() &&
           JavaArithmetic::longSub(world->getWorldTime(), torchUpdates.front().updateTime) > 60LL)
        torchUpdates.erase(torchUpdates.begin());
    if (torchActive)
    {
        if (flag)
        {
            world->setBlockAndMetadataWithNotify(i, j, k,
                Block::torchRedstoneIdle->blockID, world->getBlockMetadata(i, j, k));
            if (checkForBurnout(world, i, j, k, true))
            {
                world->playSoundEffect((float)i + 0.5f, (float)j + 0.5f, (float)k + 0.5f,
                    "random.fizz", 0.5f, 2.6f + world->rand.nextFloatDifference() * 0.8f);
                for (int_t l = 0; l < 5; l++)
                {
                    double d  = (double)i + random.nextDouble() * 0.6 + 0.2;
                    double d1 = (double)j + random.nextDouble() * 0.6 + 0.2;
                    double d2 = (double)k + random.nextDouble() * 0.6 + 0.2;
                    world->spawnParticle("smoke", d, d1, d2, 0.0, 0.0, 0.0);
                }
            }
        }
    }
    else if (!flag && !checkForBurnout(world, i, j, k, false))
    {
        world->setBlockAndMetadataWithNotify(i, j, k,
            Block::torchRedstoneActive->blockID, world->getBlockMetadata(i, j, k));
    }
}

void BlockRedstoneTorch::onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l)
{
    BlockTorch::onNeighborBlockChange(world, i, j, k, l);
    world->scheduleBlockUpdate(i, j, k, blockID, tickRate());
}

bool BlockRedstoneTorch::isIndirectlyPoweringTo(World *world, int_t i, int_t j, int_t k, int_t l)
{
    if (l == 0) return isPoweringTo(world, i, j, k, l);
    return false;
}

int_t BlockRedstoneTorch::idDropped(int_t i, Random &random)
{
    return Block::torchRedstoneActive->blockID;
}

bool BlockRedstoneTorch::canProvidePower() { return true; }

void BlockRedstoneTorch::randomDisplayTick(World *world, int_t i, int_t j, int_t k, Random &random)
{
    if (!torchActive) return;
    int_t l  = world->getBlockMetadata(i, j, k);
    double d  = (double)((float)i + 0.5f) + (double)(random.nextFloat() - 0.5f) * 0.2;
    double d1 = (double)((float)j + 0.7f) + (double)(random.nextFloat() - 0.5f) * 0.2;
    double d2 = (double)((float)k + 0.5f) + (double)(random.nextFloat() - 0.5f) * 0.2;
    double d3 = 0.2199999988079071;
    double d4 = 0.27000001072883606;
    if (l == 1)
        world->spawnParticle("reddust", d - d4, d1 + d3, d2, 0.0, 0.0, 0.0);
    else if (l == 2)
        world->spawnParticle("reddust", d + d4, d1 + d3, d2, 0.0, 0.0, 0.0);
    else if (l == 3)
        world->spawnParticle("reddust", d, d1 + d3, d2 - d4, 0.0, 0.0, 0.0);
    else if (l == 4)
        world->spawnParticle("reddust", d, d1 + d3, d2 + d4, 0.0, 0.0, 0.0);
    else
        world->spawnParticle("reddust", d, d1, d2, 0.0, 0.0, 0.0);
}

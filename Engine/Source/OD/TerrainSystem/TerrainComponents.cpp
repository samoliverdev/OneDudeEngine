#include "OD/pch.h"
#include "TerrainComponents.h"

namespace OD{
void TerrainRendererComponent::Validate(){
    patchCells = TerrainCpu::NormalizeCells(patchCells);
    maxDepth = std::clamp(maxDepth, 0, 16);
    maxPatches = std::clamp(maxPatches, 1, 65536);
    minPatchSize = std::clamp(TerrainCpu::Finite(minPatchSize, 128), 1.f, 1000000.f);
    pixelError = std::clamp(TerrainCpu::Finite(pixelError, 6), 0.25f, 256.f);
    maxDrawDistance = std::clamp(TerrainCpu::Finite(maxDrawDistance, 100000), 1.f, 10000000.f);
    textureUpdateInterval = std::clamp(TerrainCpu::Finite(textureUpdateInterval, 0.1f), 0.f, 10.f);
    for(auto& l : layers){
        for(int i = 0; i < 2; ++i)
            l.tileSize[i] = std::clamp(TerrainCpu::Finite(l.tileSize[i], 8), 0.01f, 100000.f);
        l.metallic = std::clamp(TerrainCpu::Finite(l.metallic, 0), 0.f, 1.f);
        l.roughness = std::clamp(TerrainCpu::Finite(l.roughness, 0.8f), 0.025f, 1.f);
        l.normalStrength = std::clamp(TerrainCpu::Finite(l.normalStrength, 1), 0.f, 10.f);
        for(int i = 0; i < 4; ++i)
            l.tint[i] = std::clamp(TerrainCpu::Finite(l.tint[i], 1), 0.f, 10.f);
    }
}

void TerrainCollisionComponent::Validate(){
    tilesX = std::clamp(tilesX, 1, 128);
    tilesZ = std::clamp(tilesZ, 1, 128);
    // Samples include both endpoints: 257 samples = 256 cells. Jolt pads its
    // internal blocks itself; forcing a multiple of four shifts the sample lattice.
    int cells = 8;
    while(cells < std::clamp(samplesPerTile, 9, 1025) - 1)
        cells *= 2;
    samplesPerTile = cells + 1;
    rebuildsPerFrame = std::clamp(rebuildsPerFrame, 1, 64);
}

void TerrainNavmeshDataComponent::Validate(){
    sampleSpacing = std::clamp(TerrainCpu::Finite(sampleSpacing, 8), 0.1f, 10000.f);
    maxCellsPerAxis = std::clamp(maxCellsPerAxis, 4, 1024);
}

void TerrainEditorComponent::Validate(){
    radius = std::clamp(TerrainCpu::Finite(radius, 64), 0.1f, 100000.f);
    strength = std::clamp(TerrainCpu::Finite(strength, 10), 0.f, 10000.f);
    hardness = std::clamp(TerrainCpu::Finite(hardness, 0.25f), 0.f, 0.99f);
    flattenHeight = (std::max)(0.f, TerrainCpu::Finite(flattenHeight, 100));
    paintStrength = std::clamp(TerrainCpu::Finite(paintStrength, 2), 0.f, 100.f);
    rayDistance = std::clamp(TerrainCpu::Finite(rayDistance, 100000), 1.f, 10000000.f);
    rayStep = std::clamp(TerrainCpu::Finite(rayStep, 2), 0.01f, 1000.f);
    layer = std::clamp(layer, 0, 3);
    if(int(tool) < 0 || int(tool) > 4)
        tool = TerrainCpu::BrushMode::Raise;
}
}

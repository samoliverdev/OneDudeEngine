# Resident terrain system

This module coexists with `OD/Terrain`. Its assets, components and shaders do not
replace legacy terrain. Include `OD/TerrainSystem/TerrainComponents.h` in game code.

## Create and edit

1. Create an entity and add **Terrain System / TerrainDataComponent** in the inspector.
2. Click **Add terrain components** to add the renderer, collision, navigation source
   and editor controls. Each is also independently available in Add Component.
3. Under Create, choose height/paint resolutions and create data, or assign an
   existing `.heightmap`. RAW16 import/export supports rectangular dimensions,
   explicit byte order and exact file-size validation.
4. Set world X/height/Z dimensions. The 32 km preset uses 32000 x 1000 x 32000 local
   metres and 8193 x 8193 samples. Any dimensions/resolution within the inspector
   limits are supported. The first sample is at local (0,0,0); rows increase along +Z.
5. Assign up to four albedo/normal layer pairs in TerrainRendererComponent. Set each
   layer's texture repeat size in metres, tint, roughness, metallic and normal strength.
6. Enable viewport editing in TerrainEditorComponent, select Raise, Lower, Flatten,
   Smooth or Paint, then drag the left mouse button over the terrain. Shift reverses
   raise/lower. Ctrl+Z and Ctrl+Y undo/redo whole strokes while hovering the viewport.
   The brush works without a collider. Its radius and target height are terrain-local
   metres; entity transforms are applied to picking and the outline.
7. **Save terrain assets**, then save the scene. Scene serialization stores asset
   references/settings, not a giant JSON height array. Save new assets before the
   scene. Editing detaches shared data on write; saving explicitly writes the chosen
   asset paths. Replacing/importing data or assigning another asset clears history.

## Components and backend boundary

- `TerrainDataComponent`: resident float heightmap and independent RGBA8 paintmap,
  dimensions, revision/dirty-region journal, IO and bounded sparse edit history.
- `TerrainRendererComponent`: configuration only. `TerrainQuadtreeRenderer` consumes
  data/settings and emits RenderData through `OnCollectCameraRenderData`. Replacing
  this feature leaves physics/navigation/data unaffected. GPU resources are owned by
  each rendering feature/context, shared grids by grid resolution and stitch mask.
- `TerrainCollisionComponent`: resident physics heightfield tiles, with independent
  sample density, X/Z tile counts, edit rebuild policy and work budget. Uses the
  engine's HeightmapColliderComponent/backend and creates no visual tile entities.
  Initial collision is fully prepared before simulation by default. Disable
  `buildAllOnStart` to distribute startup work over frames, accepting temporary
  missing collision while preparation finishes. Transform edits invalidate tiles.
  Sample counts use power-of-two cells plus one endpoint (257 samples = 256 cells).
  Rendering and Jolt use the same cell diagonal. Different collision density and
  visual LOD still approximate curved ground differently; a constant offset cannot
  remove that error. The legacy terrain's half-texel offset is not used here.
- `TerrainNavmeshDataComponent`: bounded navigation source geometry sampled directly
  from heights, with source spacing and allocation cap independent of render LOD.
  Use the existing NavmeshComponent/asset for bake bounds, agent settings, tile size,
  actual Detour navmesh ownership, queries and persistence.
- `TerrainEditorComponent`: brush settings. Runtime edit activation is not serialized.

Programmatic height edits must call `Touch()` or `TouchRegion(false,x0,z0,x1,z1)`;
paint edits call the same APIs with `true`. Region ends are exclusive. External
changes must also call `ClearHistory()` so old undo entries cannot target new data.
Data views and input samples must remain valid for the duration of a consumer call.

## Rendering controls

`patchCells` chooses 2,4,8,16,32,64 or128 cells per patch edge. `minPatchSize` and
`maxDepth` bound the smallest patch. `pixelError` is the target projected vertex
spacing (not a measured geometric approximation error); lower values increase detail.
`maxPatches` limits the complete selection, including shadow-only patches. Selection
never discards coverage to fit the budget: split closures preserve 2:1 balanced edges,
and the finer grid stitches to the coarse neighbor. No skirts are needed.

The actual camera passed to the collector determines LOD for every pass, including
reflection probes. Engine renderer lists handle frustum/layer/shadow culling;
`maxDrawDistance` is an additional AABB-distance limit. Materials honor the instancing
toggle in color and shadows. Both shaders share exactly the same displacement/UV code.
Height sampling maps edge samples to GPU texel centres and matches CPU bilinear sampling.

The OpenGL backend supports regional height/paint updates. Other backends use
the full re-upload fallback when `Texture2D::UpdateRegion` returns false. The existing
experimental Gfx texture format support does not yet expose float height textures;
this module targets the engine's current OpenGL renderer. `textureUpdateInterval`
throttles edit uploads. `heightTexture32Bit=false` reduces GPU height storage using
R16F at the cost of precision; CPU height data remain float32.

## Large-world tuning and limits

An 8193² float heightmap takes about 256 MiB. A 1024² RGBA paintmap takes 4 MiB.
R32F heights add about 256 MiB per graphics context. Collision storage is approximately
`tilesX * tilesZ * samplesPerTile² * 4` CPU bytes before backend compression. Undo
history has a separate configurable budget. No streaming or whole-world CPU render
mesh is used. A 32 km extent does not imply 32k height samples per axis.

Collision cells and navigation source cells can be coarser than height samples; tune
them to gameplay accuracy. Navigation geometry per bake region is capped, so large
single-region bakes automatically coarsen source sampling. Prefer tiled navigation
and bounded bake regions for a large world; the existing Recast bake itself still
has voxel-memory costs. Height edits invalidate source geometry for the next bake;
they do not automatically rebake an existing Detour navigation asset.

There is no floating-origin implementation here. Transforms use the engine's float
coordinates; precision far from the world origin remains an engine-wide constraint.
Avoid zero, negative or sheared terrain transforms for physics; ordinary positive
scale/rotation/translation are supported. Holes, caves and foliage scattering are
not part of this heightfield module.

## Verification

`cmake --build build --target TerrainSystemCpuTests Tests Editor`

`ctest --test-dir build -R TerrainSystemCpuTests --output-on-failure`

`Bin/Tests.exe --gtest_filter=TerrainSystemDataTest.*` runs without graphics startup.

`Bin/Tests.exe --gtest_filter=TerrainSystemRenderTest.*` requires an OpenGL context
and engine content. It checks shader variants, camera selection, regional uploads,
and independent navigation geometry. A 32 km frame-rate target must be measured on
the intended hardware; no fixed FPS guarantee is implied by these tests.

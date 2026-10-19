# Terrain tiles and render submission design

## Goal and approved scope

Render terrain using a shared tile mesh set and direct RenderData submissions,
without a MeshRenderer entity for every chunk or tile. Base geometry density
must be independent of terrain width, terrain length, and logical chunk size or
count. Preserve heightmap displacement, materials, LOD seams, shadows, collider
creation, and navigation mesh generation.

The user approved fixed tile meshes, fixed base density, logical chunks, direct
render submission, and position-based heightmap sampling. Their additional
requirements are configurable tile grid resolution and an instancing toggle.

## Terrain configuration

- `tileResolution`: vertices along each tile edge; default 33. Supported values
  are powers of two plus one, from 3 through 257, selected explicitly in the
  inspector. Invalid programmatic or loaded values normalize to the nearest
  supported value, with ties choosing the smaller value.
- `vertexSpacing`: distance between adjacent base LOD vertices in terrain local
  units; default 1. Nonpositive or nonfinite values normalize to 1.
- `useInstancing`: default true. Selects DrawMeshInstanced or individual DrawMesh
  submissions for terrain color and shadow rendering.
- Existing width, length, height, material, heightmap, and LOD bias controls
  remain available. Chunk count controls logical grouping and LOD selection.
- Serialize the new settings and expose them in the inspector. Existing saved
  terrains without these fields must load with the defaults. Retain legacy
  `mapChunkSize` when needed for compatibility and navigation mesh behavior;
  it no longer determines render mesh resolution.

Base tile span is `(tileResolution - 1) * vertexSpacing`. For example, default
tiles cover 32 by 32 terrain units. Increasing terrain dimensions increases tile
coverage. Changing logical chunk count does not move the base vertex lattice.
Increasing tileResolution changes tile granularity while preserving base spacing.
An entity transform scales terrain local units into world units, as usual.

## Shared meshes and terrain coverage

Cache normalized flat grid meshes by tileResolution, LOD index, and four-bit
border mask. Terrain instances with the same resolution share this immutable
mesh set. Terrain dimensions, chunk count, and heightmap changes do not generate
another mesh set. Release cached resources with the owning rendering lifetime;
avoid graphics resources surviving graphics-device shutdown.

Cover the terrain with a lattice anchored at its local origin. Use ceiling
division for tile coverage. On the last tile along either axis, clamp vertex
coordinates to the terrain boundary before height sampling. This preserves the
interior vertex spacing, exact terrain dimensions, and shared edge coordinates;
some triangles at the outer edge may become degenerate. Apply identical rules
in color and shadow shaders. Zero or invalid terrain dimensions submit no tiles.

ChunkData contains logical coordinates, bounds, tile membership, and LOD state;
it does not own render entities. Assign tiles to logical chunks by their centers.
Logical chunk bounds enclose their member tiles even when a tile crosses an
ideal chunk boundary. This allows arbitrary positive chunk counts without
splitting meshes or changing the lattice.

## LOD and seams

Generate LODs from tileResolution down to two cells per edge. Neighboring levels
use a 2:1 sample spacing ratio. Omit a one-cell LOD so the existing stitched-border
strategy always has a valid edge to collapse. The minimum resolution therefore
has a single LOD. Reuse border variants within the shared mesh set.

Select desired LOD using the existing distance/bias behavior adapted to actual
terrain coordinates and logical chunk extents. Propagate and constrain final
tile LODs so adjacent tiles differ by at most one level, including across chunk
boundaries. Select stitched edges on the finer tile. Outer terrain edges have
no neighbor stitch requirement. A change in tile resolution rebuilds topology;
a change in chunk grouping updates membership and LOD state.

## Direct rendering API

Add `DrawMesh` and `DrawMeshInstanced` submission methods to RenderContext.
Graphics::DrawMesh already performs immediate graphics commands and keeps its
existing semantics. The new context methods append RenderData during collection,
before camera and shadow renderer lists are built. Document that collection is
the supported call phase; do not introduce a global queue or submissions that
silently survive into later frames.

DrawMesh accepts mesh and material resources, a model transform, local bounds
(including shader displacement), and draw options for layer, shadow casting,
custom shadow material, and custom instance data. It uses the existing matrix
instance-data convention and computes world bounds and distance consistently
with MeshRenderer collection. Validate required resources and initialize every
RenderData field consumed by the pipeline.

DrawMeshInstanced accepts the same shared resources/options and an array of
instance transforms, custom data, and bounds. Copy or retain submitted data until
all render passes finish; never retain pointers into caller-temporary arrays.
Use the engine InstancingBuffer path for grouped rendering. Empty submissions
are harmless. Respect backend batch limits, shader support, layers, and custom
shadow materials. Unsupported instancing falls back to individual submissions.

With terrain useInstancing=false, prevent automatic pipeline batching from
recombining those submissions into instanced draws. With it enabled, group
compatible tiles by terrain material, LOD mesh, and border variant. Bounds must
enclose all displaced vertices and must not discard shadow-only visible tiles.

## Collection and shader integration

Terrain automatically participates in RenderContext collection for the supplied
scene; no manual renderer-feature installation is required. Collect enabled
terrains for each rendering context without relying on chunk entities. Use
per-context scratch/buffer storage so cameras cannot overwrite one another's
instance buffers before their draws finish.

Terrain and TerrainShadow shaders support instancing and derive height UVs from
terrain local position divided by terrain width/length. Color and shadow paths
use the same origin, clamping, sampling, displacement, and transform conventions.
Use shared material parameters for terrain dimensions and height scale, and the
existing instance-data mechanism for tile-specific data. Preserve texture layer
sampling across tile boundaries. Update materials once per terrain collection
instead of once per tile.

## Compatibility and boundaries

Keep terrain entities and collider/navigation helper entities. Do not create
visual Chunk or tile MeshRenderer entities. Preserve the original heightmap
data resolution: the renderer cannot add height detail absent from that data.
Existing user changes in physics, Defines.h, settings, and tests are unrelated
and must be preserved. No unrelated rendering or physics refactoring is included.

## Validation and acceptance

Check default and alternative supported resolutions, invalid settings, terrains
smaller than one tile, nonmultiple dimensions, rectangular terrain, and missing
heightmaps. Confirm base sample spacing is unchanged when dimensions or chunk
count changes. Check neighboring LOD seams and exact terrain boundary coverage.
Confirm disabling instancing changes the draw path in both color and shadows
while preserving geometry and sampling. Check custom shadow materials, layers,
resource lifetime, multiple cameras, and off-camera shadow casters.

Build the affected engine/sandbox targets and use focused tests for pure tile
layout, LOD, and submission rules where practical. Report any graphics validation
that requires an interactive run and cannot be exercised in this environment.

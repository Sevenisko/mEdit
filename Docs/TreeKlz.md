# tree.klz — Mafia Collision File Format

## Overview

The KLZ (Kladzivo / "Hammer") file stores static collision data for a mission/level. It is loaded by `g_collision::LoadCollision` (0x5C2B70) and contains:

- A **link table** mapping indices to named scene frames
- A **grid** that spatially partitions the world into cells
- **Volume arrays** (faces, AABBs, XTOBBs, OBBs, cylinders, spheres)
- **Cell data** referencing which volumes overlap each grid cell

The file uses **little-endian** byte order throughout.

---

## File Layout

```
+========================+
| File Header   (24 B)   |
+--------------------------+
| Link Offset Table       |  (4 * numLinks bytes)
+--------------------------+
| Link Data               |  (variable, padded)
+--------------------------+  <-- gridDataOffset
| Grid Header  (0x68 B)  |
+--------------------------+
| Bounds X     (w+1 floats)|
| Bounds Y     (l+1 floats)|
+--------------------------+
| Skip Int32   (4 B)      |
+--------------------------+
| Face Array               |  (numFaces * 32 B)
| AABB Array               |  (numAABBs * 32 B)
| XTOBB Array              |  (numXTOBBs * 184 B)
| Cylinder Array           |  (numCylinders * 20 B)
| OBB Array                |  (numOBBs * 160 B)
| Sphere Array             |  (numSpheres * 24 B)
+--------------------------+
| Skip Int32   (4 B)      |
+--------------------------+
| Cell Data                |  (width * length cells)
+========================+
```

---

## 1. File Header (24 bytes)

| Offset | Size | Type   | Field           | Description |
|--------|------|--------|-----------------|-------------|
| 0x00   | 4    | char[4]| fourCC          | Magic: `"GifC"` (0x43666947) |
| 0x04   | 4    | uint32 | version         | File version (see below) |
| 0x08   | 4    | uint32 | gridDataOffset  | Absolute file offset to the grid data block |
| 0x0C   | 4    | uint32 | numLinks        | Number of entries in the link table |
| 0x10   | 4    | uint32 | reserved0       | Unknown (original TUTORIAL uses 379) |
| 0x14   | 4    | uint32 | reserved1       | Always 0 |

### Version Values

| Version      | Value        | Encoding |
|--------------|-------------|----------|
| VERSION_MAFIA     | 0x00000005 | major=0, minor=5 |
| VERSION_HD2       | 0x00010005 | major=1, minor=5 |
| VERSION_CHAMELEON | 0x0002000A | major=2, minor=10 |

This document focuses on **VERSION_MAFIA** (0x00000005).

---

## 2. Link Table

Immediately after the header. Two parts: an offset table, then the link data entries.

### Link Offset Table

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0x18   | 4 * numLinks | uint32[] | Absolute file offsets to each link data entry |

### Link Data Entries

Each link entry is at the file position specified in the offset table:

| Offset | Size | Type   | Field | Description |
|--------|------|--------|-------|-------------|
| +0x00  | 4    | uint32 | type  | Link type flags (see below) |
| +0x04  | var  | char[] | name  | Null-terminated frame name |
| +var   | 0-3  | bytes  | pad   | Padding to 4-byte alignment (based on name length + 1) |

### Link Types

| Value | Name | Bit 0 (Dynamic) | Description |
|-------|------|-----------------|-------------|
| 0     | LINK_NONE    | 0 | No link |
| 1     | LINK_SURFACE | 1 | Frame is referenced by face triangles |
| 2     | LINK_VOLUME  | 0 | Frame is referenced by primitive volumes |
| 3     | LINK_SURFACE \| LINK_VOLUME | 1 | Frame is referenced by both faces and volumes |

### Link Resolution at Load Time

The loader processes links in **two passes**:

**Pass 1** — Name resolution (iterates in reverse order):
1. Reads `type & 1` to capture the "dynamic" flag
2. Overwrites the `type` field in memory with the frame pointer from `FindFrameFast(scene, name)`
3. If dynamic (bit 0 was set):
   - If the frame is not of type VISUAL (frame[0x110] != 1), NULLs it
   - If still valid, calls `SetOn(true)` on the frame

**Pass 2** — Vertex base resolution (iterates in reverse order):
For **every** link (regardless of original type):
1. If frame is non-NULL, is VISUAL (type==1), is not "MIRR" (frame[0x1D4] != 0x5252494D), and has mesh data (frame[0x1E0] != 0):
   - Replaces the link entry with the mesh's vertex base pointer
2. Otherwise: sets the link entry to NULL

After both passes, each link entry contains either a **vertex base pointer** or **NULL**.

---

## 3. Grid Header (0x68 bytes)

Located at file position `gridDataOffset`. All offsets below are relative to the grid data start.

| Offset | Size | Type   | Field       | Description |
|--------|------|--------|-------------|-------------|
| 0x00   | 4    | float  | minX        | Grid minimum X coordinate (world space) |
| 0x04   | 4    | float  | minY        | Grid minimum Y coordinate (world Z axis) |
| 0x08   | 4    | float  | maxX        | Grid maximum X coordinate |
| 0x0C   | 4    | float  | maxY        | Grid maximum Y coordinate (world Z axis) |
| 0x10   | 4    | float  | cellWidth   | Width of each cell in X direction |
| 0x14   | 4    | float  | cellHeight  | Height of each cell in Y direction |
| 0x18   | 4    | uint32 | width       | Number of cells in X direction |
| 0x1C   | 4    | uint32 | length      | Number of cells in Y direction |
| 0x20   | 4    | float  | unknown     | Used for DynSpatialHash computation (often 20.0) |
| 0x24   | 4    | uint32 | _reserved   | Overwritten at runtime with pBoundsX pointer |
| 0x28   | 4    | uint32 | _reserved   | Overwritten at runtime with pBoundsY pointer |
| 0x2C   | 4    | uint32 | _reserved   | Unused padding |
| 0x30   | 4    | uint32 | numFaces    | Number of triangle faces |
| 0x34   | 4    | uint32 | _pFaces     | Reserved; filled at runtime with pointer to face array |
| 0x38   | 4    | uint32 | numXTOBBs   | Number of XTOBB volumes |
| 0x3C   | 4    | uint32 | _pXTOBBs    | Reserved; filled at runtime |
| 0x40   | 4    | uint32 | numAABBs    | Number of AABB volumes |
| 0x44   | 4    | uint32 | _pAABBs     | Reserved; filled at runtime |
| 0x48   | 4    | uint32 | numSpheres  | Number of sphere volumes |
| 0x4C   | 4    | uint32 | _pSpheres   | Reserved; filled at runtime |
| 0x50   | 4    | uint32 | numOBBs     | Number of OBB volumes |
| 0x54   | 4    | uint32 | _pOBBs      | Reserved; filled at runtime |
| 0x58   | 4    | uint32 | numCylinders| Number of cylinder volumes |
| 0x5C   | 4    | uint32 | _pCylinders | Reserved; filled at runtime |
| 0x60   | 4    | uint32 | numVertices | Number of standalone vertices (typically 0 for Mafia) |
| 0x64   | 4    | uint32 | _pVertices  | Reserved; filled at runtime |

**Note:** The header field order for volume counts (Faces, XTOBBs, AABBs, Spheres, OBBs, Cylinders) differs from the data layout order (Faces, AABBs, XTOBBs, Cylinders, OBBs, Spheres). The loader's pointer arithmetic handles this correctly.

### Grid Coordinate System

The grid operates in the **XZ horizontal plane**:
- Grid X axis = World X axis
- Grid Y axis = World **Z** axis (not Y!)
- Grid `minY`/`maxY` correspond to world Z bounds

Cell size is typically 5.0 x 5.0 units.

---

## 4. Bounds Arrays

Immediately after the grid header (at grid offset 0x68):

| Data | Size | Description |
|------|------|-------------|
| boundsX | (width + 1) * 4 | Cell boundary X coordinates. `boundsX[i] = minX + i * cellWidth` |
| boundsY | (length + 1) * 4 | Cell boundary Y coordinates. `boundsY[i] = minY + i * cellHeight` |

Followed by **1 skip int32** (4 bytes, typically 0).

---

## 5. Volume Arrays

Written in this exact order (this is the **data order**, not the header field order):

### 5.1 Face (Triangle) — 32 bytes each

| Offset | Size | Type    | Field | Description |
|--------|------|---------|-------|-------------|
| 0x00   | 1    | uint8   | type  | Volume type (0x00-0x07, recomputed at load time from plane normal) |
| 0x01   | 1    | uint8   | sortInfo | Sort/priority info |
| 0x02   | 1    | uint8   | flags | Collision flags |
| 0x03   | 1    | uint8   | mtlId | Material ID |
| 0x04   | 2    | uint16  | vert0.vertexBufferIndex | Index into the frame's mesh vertex buffer |
| 0x06   | 2    | uint16  | vert0.linkIndex | Index into the link table (0x7FFF = global vertex buffer) |
| 0x08   | 2    | uint16  | vert1.vertexBufferIndex | |
| 0x0A   | 2    | uint16  | vert1.linkIndex | Ignored by game (uses vert0.linkIndex for all vertices) |
| 0x0C   | 2    | uint16  | vert2.vertexBufferIndex | |
| 0x0E   | 2    | uint16  | vert2.linkIndex | Ignored by game (uses vert0.linkIndex for all vertices) |
| 0x10   | 4    | float   | plane.normal.x | |
| 0x14   | 4    | float   | plane.normal.y | |
| 0x18   | 4    | float   | plane.normal.z | |
| 0x1C   | 4    | float   | plane.distance | |

#### Face Type Byte (0x00-0x07)

Recomputed by `ComputeFaceOrientations` (0x5C2980) after loading based on the plane normal. The algorithm determines the dominant axis and a sub-classification:

| Type | Meaning |
|------|---------|
| 0x00 | (unused by orientation algorithm, but valid) |
| 0x01 | Dominant = Z, sub = thin XZ |
| 0x02 | Dominant = Z, other |
| 0x03 | Dominant = Y, sub = thin XY |
| 0x04 | Dominant = Y, sub = thin YZ |
| 0x05 | Dominant = Y, other |
| 0x06 | Dominant = X, sub = thin XY |
| 0x07 | Dominant = X, other |
| 0x20 | **Disabled** — face has degenerate geometry or NULL link |

A face is disabled (set to 0x20) if:
- Its `vert0.linkIndex` resolves to a NULL link (frame not found, not VISUAL, etc.)
- Its resolved vertices form a degenerate triangle (>1 thin axis, determined by edge length < 1e-8)

The check `(type & 0xA0) == 0` is used to test if a face is active (bits 5 and 7 clear).

#### Face Vertex Resolution at Load Time

The game uses **only `vert0.linkIndex`** to look up the vertex base. All three vertices are resolved using the same base:

```
vertexBase = *linkTable[vert0.linkIndex]   // NULL → face disabled
vert0_ptr = vertexBase + vert0.vertexBufferIndex * 32
vert1_ptr = vertexBase + vert1.vertexBufferIndex * 32
vert2_ptr = vertexBase + vert2.vertexBufferIndex * 32
```

If `vert0.linkIndex == 0x7FFF`, the global vertex buffer (pVertices) is used instead, with 12-byte stride.

### 5.2 AABB — 32 bytes each

| Offset | Size | Type    | Field | Description |
|--------|------|---------|-------|-------------|
| 0x00   | 1    | uint8   | type  | Volume type (0x81) |
| 0x01   | 1    | uint8   | sortInfo | |
| 0x02   | 1    | uint8   | flags | |
| 0x03   | 1    | uint8   | mtlId | |
| 0x04   | 4    | uint32  | linkId | Index into link table (overwritten at runtime with resolved pointer) |
| 0x08   | 12   | vec3    | min   | Minimum corner |
| 0x14   | 12   | vec3    | max   | Maximum corner |

### 5.3 XTOBB — 184 bytes each

| Offset | Size | Type    | Field | Description |
|--------|------|---------|-------|-------------|
| 0x00   | 1    | uint8   | type  | Volume type (0x80) |
| 0x01   | 1    | uint8   | sortInfo | |
| 0x02   | 1    | uint8   | flags | |
| 0x03   | 1    | uint8   | mtlId | |
| 0x04   | 4    | uint32  | linkId | |
| 0x08   | 12   | vec3    | min   | World-space minimum |
| 0x14   | 12   | vec3    | max   | World-space maximum |
| 0x20   | 12   | vec3    | minExtent | Local-space minimum (VERSION_MAFIA: vec3) |
| 0x2C   | 12   | vec3    | maxExtent | Local-space maximum (VERSION_MAFIA: vec3) |
| 0x38   | 64   | mat4x4  | transform | Local-to-world transform |
| 0x78   | 64   | mat4x4  | inverseTransform | World-to-local transform |

### 5.4 Cylinder — 20 bytes each

| Offset | Size | Type    | Field | Description |
|--------|------|---------|-------|-------------|
| 0x00   | 1    | uint8   | type  | Volume type (0x84) |
| 0x01   | 1    | uint8   | sortInfo | |
| 0x02   | 1    | uint8   | flags | |
| 0x03   | 1    | uint8   | mtlId | |
| 0x04   | 4    | uint32  | linkId | |
| 0x08   | 4    | float   | pos.x | X position (world space) |
| 0x0C   | 4    | float   | pos.y | Y position (= world Z) |
| 0x10   | 4    | float   | radius | |

### 5.5 OBB — 160 bytes each

| Offset | Size | Type    | Field | Description |
|--------|------|---------|-------|-------------|
| 0x00   | 1    | uint8   | type  | Volume type (0x83) |
| 0x01   | 1    | uint8   | sortInfo | |
| 0x02   | 1    | uint8   | flags | |
| 0x03   | 1    | uint8   | mtlId | |
| 0x04   | 4    | uint32  | linkId | |
| 0x08   | 12   | vec3    | minExtent | Local-space minimum |
| 0x14   | 12   | vec3    | maxExtent | Local-space maximum |
| 0x20   | 64   | mat4x4  | transform | |
| 0x60   | 64   | mat4x4  | inverseTransform | |

### 5.6 Sphere — 24 bytes each

| Offset | Size | Type    | Field | Description |
|--------|------|---------|-------|-------------|
| 0x00   | 1    | uint8   | type  | Volume type (0x82) |
| 0x01   | 1    | uint8   | sortInfo | |
| 0x02   | 1    | uint8   | flags | |
| 0x03   | 1    | uint8   | mtlId | |
| 0x04   | 4    | uint32  | linkId | |
| 0x08   | 12   | vec3    | position | World-space center |
| 0x14   | 4    | float   | radius | |

### Volume Type Constants

| Value | Name | Cell Ref Type Byte | Struct Size |
|-------|------|-------------------|-------------|
| 0x00-0x07 | FACE (orientations) | 0x00 | 32 |
| 0x80 | XTOBB | 0x80 | 184 |
| 0x81 | AABB | 0x81 | 32 |
| 0x82 | SPHERE | 0x82 | 24 |
| 0x83 | OBB | 0x83 | 160 |
| 0x84 | CYLINDER | 0x84 | 20 |

---

## 6. Cell Data

After the volume arrays and another **1 skip int32** (4 bytes).

There are `width * length` cells in **row-major order**: cell index = `row * width + column`, where row iterates the Y/Z axis and column iterates the X axis.

### Cell Structure (variable size)

| Offset | Size | Type   | Field      | Description |
|--------|------|--------|------------|-------------|
| 0x00   | 4    | uint32 | numRefs    | Number of volume references in this cell |
| 0x04   | 4    | int32  | _reserved  | Overwritten at runtime with pReferences pointer |
| 0x08   | 4    | int32  | _reserved  | Overwritten at runtime with pFlags pointer |
| 0x0C   | 4    | float  | maxHeight  | Max world Y of all volumes in cell; used by `TestLineV`/`TestSphereB` to cull cells where `maxHeight < query.minY` |

If `numRefs > 0`:

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0x10   | 4 * numRefs | int32[] | references | Packed volume references (see below) |
| 0x10 + 4*N | numRefs | uint8[] | flags | Per-reference flag bytes |
| ... | 0-3 | bytes | padding | Pad flags to 4-byte alignment |

### Cell Reference Format (packed int32)

```
 31..24    23..0
+--------+------------------------+
|  type  |      offset            |
+--------+------------------------+
```

- **type** (bits 31-24): Volume type byte
  - `0x00`: Face reference
  - `0x80`: XTOBB reference
  - `0x81`: AABB reference
  - `0x82`: Sphere reference
  - `0x83`: OBB reference
  - `0x84`: Cylinder reference
- **offset** (bits 23-0): Byte offset relative to the start of the respective volume array within the grid data block

### Cell Reference Resolution at Load Time

```
switch(type_byte):
  case 0x00:       ref_value += pFaces          // Face: add full 32-bit value to pFaces base
  case 0x80:       ptr = pXTOBBs + (ref & 0xFFFFFF)
  case 0x81:       ptr = pAABBs + (ref & 0xFFFFFF)
  case 0x82:       ptr = pSpheres + (ref & 0xFFFFFF)
  case 0x83:       ptr = pOBBs + (ref & 0xFFFFFF)
  case 0x84:       ptr = pCylinders + (ref & 0xFFFFFF)
  default:         skip (no resolution)
```

**Important for faces:** The loader uses `*v125 += pFaces` which adds the **full 32-bit value** (not masked to 24 bits) to pFaces. Since the type byte is 0x00, the high byte is zero and the full value equals the offset. This means face offsets can theoretically use all 32 bits, but in practice 24 bits (16 MB) is more than sufficient.

### Cell Reference Flags

Each reference has a corresponding flag byte. For faces, this is the `Collider::flags` byte. The flags array is padded to a 4-byte boundary using zero bytes.

---

## 7. Runtime Pointer Computation

After loading the entire grid data block into memory, the loader computes array base pointers:

```
pBoundsX   = gridData + 0x68
pBoundsY   = pBoundsX + 4 * (width + 1)
pFaces     = pBoundsY + 4 * length + 8        // +8 = 4*(length+1 - length) + 4 skip
pAABBs     = pFaces + 32 * numFaces            (if numAABBs > 0, else NULL)
pXTOBBs    = pAABBs + 32 * numAABBs           (if numXTOBBs > 0, else NULL)
pCylinders = pXTOBBs + 184 * numXTOBBs        (if numCylinders > 0, else NULL)
pOBBs      = pCylinders + 20 * numCylinders    (if numOBBs > 0, else NULL)
pSpheres   = pOBBs + 160 * numOBBs            (if numSpheres > 0, else NULL)
pVertices  = pSpheres + 24 * numSpheres        (if numVertices > 0, else NULL)
cellStart  = pVertices_end + 12 * numVertices + 4
```

The `pBoundsY + 4*length + 8` formula accounts for:
- boundsY has `length + 1` entries = `4*(length+1)` bytes
- Plus 4 bytes for the skip int32
- Total: `4*length + 4 + 4 = 4*length + 8`

---

## 8. Post-Load Processing

### Volume Link Resolution

After pointer computation, each volume type's `linkId` fields are resolved:

```
For each AABB:    aabb.linkId = *linkTable[aabb.linkId]    // Replace index with vertex base or NULL
For each XTOBB:   xtobb.linkId = *linkTable[xtobb.linkId]
For each Cylinder: cyl.linkId = *linkTable[cyl.linkId]
For each OBB:     obb.linkId = *linkTable[obb.linkId]
For each Sphere:  sphere.linkId = *linkTable[sphere.linkId]
```

### Face Vertex Resolution

For each face, using **only vert0.linkIndex**:

1. Look up `vertexBase = *linkTable[vert0.linkIndex]`
2. If NULL → set face type to 0x20 (disabled), skip to next face
3. For each vertex j (0-2):
   - `vertex[j] = vertexBase + vertexBufferIndex[j] * 32`
4. Compute face AABB from the 3 resolved vertex positions
5. Check for degeneracy (>1 thin axis where edge lengths < 1e-8)
   - If degenerate → set face type to 0x20 (disabled)

### ComputeFaceOrientations (0x5C2980)

Called last. Iterates all cells and their references. For each face reference where `(type & 0xA0) == 0` (not disabled):

1. Read plane normal from face data (offsets 0x10, 0x14, 0x18)
2. Determine dominant axis by comparing `|normal.x|`, `|normal.y|`, `|normal.z|`
3. Compute sub-classification based on vertex position differences
4. Write orientation type (1-7) to the face's type byte, preserving sortInfo/flags/mtlId

### DynSpatialHash Initialization

If requested, allocates a secondary spatial hash for dynamic collision:
```
dynGrid.width  = (maxX - minX) / unknown_0x20    // uses the float at grid offset 0x20
dynGrid.height = (maxY - minY) / unknown_0x20
```

---

## 9. VERSION_HD2 Differences (0x00010005)

Not fully documented here, but key differences:

- XTOBB: `minExtent`/`maxExtent` are vec4 instead of vec3
- Sphere: different field order (radius, unknown, position, unknown)
- Cylinder: 3 additional unknown floats
- Grid header: additional `minZ`, `maxZ`, `cellHeight`, `height` fields, plus more skip ints
- Grid has 3D cells: `width * length * height`
- Bounds arrays include a Z dimension
- More skip ints throughout

## 10. VERSION_CHAMELEON Differences (0x0002000A)

- Cell struct: no `unknown` field (no int32 at offset 0x0C)
- Cell flags array: not present (VERSION_MAFIA only)
- Different skip int counts in grid header and between sections

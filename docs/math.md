# Project 9 — Math derivations

All four primitives and the camera are generated procedurally in `src/main.cpp`
(`build_sphere`, `build_cylinder`, `build_cube`, and the per-frame camera
block). Every vertex uses the same 8-float layout: `position(3) + normal(3) +
uv(2)`.

## Sphere (lat/long parameterization)

For stack index `i` (0..stacks) and slice index `j` (0..slices):

```
phi   = pi * i / stacks        phi   in [0, pi]      (pole to pole)
theta = 2*pi * j / slices      theta in [0, 2*pi]     (around the equator)

x = sin(phi) * cos(theta)
y = cos(phi)
z = sin(phi) * sin(theta)
```

The mesh is a unit sphere, so the surface normal at every vertex is simply the
position itself, `N = (x, y, z)` — no separate normal computation is needed
because for a sphere centered at the origin, the outward normal is always
parallel to the radius vector. UV is `(j/slices, 1 - i/stacks)`, giving a
standard equirectangular layout. Two triangles connect each `(i, j)` quad to
its neighbors `(i+1, j)` and `(i+1, j+1)`.

## Cylinder

For slice index `j` (0..slices), with `theta = 2*pi*j/slices`:

```
top ring:    (cos(theta), 1, sin(theta))
bottom ring: (cos(theta), -1, sin(theta))
```

This is a unit-radius, height-2 cylinder aligned with the Y axis (no end
caps — only the curved side wall is built). Because every point on the side
wall lies on the circle `x^2 + z^2 = 1` regardless of height, the outward
normal has no Y component: `N = (cos(theta), 0, sin(theta))`, i.e. the same
direction as the position's X/Z components, normalized. Each slice contributes
one quad (two triangles) connecting its top/bottom ring points to the next
slice's.

## Cube

The cube is *not* built from 8 shared corner vertices — it uses 24 vertices (4
per face) so each face can carry its own constant normal and its own 0–1 UV
square, instead of the incorrect averaged/shared normals a corner-indexed cube
would produce. For each of the 6 faces:

```
normal = the face's outward axis, e.g. (0,0,1) for the +Z face,
         (1,0,0) for the +X face, etc.
corners = the four ±1 corners of that face, wound consistently (CCW as
          seen from outside) so the two triangles per face
          (corners[0,1,2] and corners[0,2,3]) face outward.
uv = (0,0), (1,0), (1,1), (0,1)  — the same square for every face.
```

Six constant face normals plus a per-face UV square is the standard technique
for a "flat-shaded, correctly-mapped" cube, at the cost of not sharing
vertices between adjacent faces (24 vertices instead of 8).

## Fly-through camera

The camera stores position `g_pos` and two angles, `yaw` and `pitch`, updated
by mouse-drag (`yaw += dx * 0.004`, `pitch -= dy * 0.004`, clamped to
`[-1.4, 1.4]` radians so the camera can't flip over the pole). Every frame,
the forward and right vectors are derived from those two angles:

```
forward = (cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw))
right   = normalize(cross(forward, (0,1,0)))
```

This is the standard spherical-to-Cartesian yaw/pitch camera: `yaw` rotates
the look direction around the world Y axis, and `pitch` tilts it up/down
within the vertical plane containing that direction; `cos(pitch)` scales the
X/Z contribution down as the camera looks more toward the poles, which is
what keeps `forward` a unit vector. WASD/arrow keys move `g_pos` along
`forward`/`right` scaled by `4 units/sec * dt`, so movement speed is
frame-rate-independent. The view matrix is then:

```
view = lookAt(g_pos, g_pos + forward, (0,1,0))
proj = perspective(radians(60), aspect, 0.1, 100)
MVP  = proj * view * model     (model is per-object: translate * rotate * scale)
```

## Fractal tree (background prop)

The tree added as an optional background element (see README) uses the same
recursive branching rule documented for Topic 9 Activity 1: each branch emits
a line segment, then — if recursion depth remains — forks into `branches`
children, each rotated `angleDeg` away from the parent's direction and evenly
spaced in azimuth around it, shrinking length by a constant `shrink` factor
per generation. It is generated once at startup (not every frame) since it is
static set dressing, not the interactive subject of the scene.

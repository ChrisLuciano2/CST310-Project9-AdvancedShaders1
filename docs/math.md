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

## Keyboard camera (position + yaw/pitch/roll)

The camera stores a position `g_pos` and three Euler angles in degrees —
`yaw`, `pitch`, `roll` — updated only by discrete keyboard events (`key_cb`
in `main.cpp`), matching the Project 9 Resource Guide's control table
exactly: plain arrow keys slide `g_pos` by 1 unit along a world axis,
Shift+Up/Down slide along Z, Ctrl+arrows change yaw/pitch by 2°, `,`/`.`
change roll by 2°, and `r` resets all of it to the startup defaults. There is
no per-frame polling or mouse input — every change is a fixed step applied
once per key event, so movement speed is entirely a function of how many
times a key is pressed (or auto-repeated by the OS), not frame time.

Every frame, the current yaw/pitch/roll are turned into a rotation matrix and
applied to the default look direction and up vector:

```
R = Ry(yaw) * Rx(pitch) * Rz(roll)      (roll applied first, in local space,
                                          then pitch, then yaw — see below)
forward = R * (0, 0, -1)
up      = R * (0, 1,  0)
```

Order matters: building `R` as `Ry * Rx * Rz` and multiplying it onto a
column vector applies `Rz` (roll) first, then `Rx` (pitch), then `Ry` (yaw) —
so roll tilts the camera *before* pitch/yaw reorient the whole thing, which
is what keeps roll acting like "tilt your head" rather than rolling around a
world-space axis that drifts as you turn. Passing the *rotated* `up` vector
(not the world up) into `lookAt` is specifically what makes roll visible: a
view matrix's up vector defines which way is "top of screen," so rotating it
is the whole effect. The view matrix is then:

```
view = lookAt(g_pos, g_pos + forward, up)
proj = perspective(radians(60), aspect, 0.1, 100)
MVP  = proj * view * model     (model is per-object: translate * rotate * scale)
```

## Basic shaders

All three techniques below run inside one shared fragment shader
(`scene.frag`), selected per draw call by an integer `uShaderMode` uniform,
rather than four separate shader programs — a single "uber-shader" with a
mode branch is simpler to manage for this small a set of effects.

### Sphere — Fresnel rim

```
diff = max(dot(N, L), 0)
fres = (1 - max(dot(N, V), 0)) ^ 3
color = mix(base * (0.18 + 0.82*diff), white, fres * 0.55)
```

`V` is the normalized vector from the surface point to the camera
(`normalize(eyePos - worldPos)`). `dot(N, V)` is near 1 where the surface
faces the camera directly and near 0 at the silhouette edge (grazing
angles), so `1 - dot(N,V)` is small head-on and large at the rim; raising it
to a power sharpens that falloff into a narrow bright band right at the
silhouette. This is the classic Fresnel-effect approximation (real Fresnel
reflectance is more involved) and is a common cheap stand-in for "this
surface is reflective" before implementing real environment mapping.

### Cylinder — procedural bump mapping

```
h(v)  = sin(v * 40) * 0.05                       (procedural "height map")
dh/dv = cos(v * 40) * 40 * 0.05                   (its exact derivative)
T = normalize(dFdx(worldPos))                     (screen-space tangent)
B = normalize(cross(N, T));  T = normalize(cross(B, N))
N' = normalize(N - B * (dh/dv) * strength)
diff = max(dot(N', L), 0)
```

Rather than take a screen-space finite difference of the height value itself
(`dFdx(h)`/`dFdy(h)`), which shrinks toward zero as a surface covers more
screen pixels and was empirically invisible in an earlier version of this
shader (see `docs/../Topic9_LabQuestions/Topic9_LabQ3_BumpMapping.docx` for
the side-by-side proof), this uses the height field's closed-form derivative
with respect to `v` and a tunable `strength` constant, keeping the ridge
effect's visible strength independent of camera distance.

### Cube — procedural panel pattern

```
g = abs(fract(uv * 4 - 0.5) - 0.5)
seam = smoothstep(0, 0.05, min(g.x, g.y))
color = mix(base * 0.55, base, seam) * (0.18 + 0.82 * diff)
```

`fract(uv*4 - 0.5) - 0.5` repeats a sawtooth four times across each face,
centered so its zero-crossings fall at even panel boundaries; taking the
absolute value and the minimum of the two axes finds how close a fragment is
to *either* a horizontal or vertical seam line, and `smoothstep` turns that
distance into a soft 0→1 mask. The result darkens narrow bands at regular UV
intervals, reading as routed panel seams — inexpensive surface structure
ahead of true parallax (height-map-driven UV displacement) mapping.

## Fractal tree (background prop)

The tree added as an optional background element (see README) uses the same
recursive branching rule documented for Topic 9 Activity 1: each branch emits
a line segment, then — if recursion depth remains — forks into `branches`
children, each rotated `angleDeg` away from the parent's direction and evenly
spaced in azimuth around it, shrinking length by a constant `shrink` factor
per generation. It is generated once at startup (not every frame) since it is
static set dressing, not the interactive subject of the scene.

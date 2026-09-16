# Ray Tracer

A C++17 CPU ray tracer following the "Ray Tracing in One Weekend" path:
spheres, diffuse/metal/dielectric materials, antialiasing, a positionable
camera with depth of field, and multithreaded rendering.

## Build

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
```

## Render

The program writes a PPM image to stdout:

```
./build/raytracer > renders/output.ppm
```

Convert to PNG (requires ImageMagick or Pillow):

```
magick renders/output.ppm renders/output.png
# or: python3 -c "from PIL import Image; Image.open('renders/output.ppm').save('renders/output.png')"
```

## Layout

- `include/vec3.h` — 3D vector math (also used as `point3` and `color`)
- `include/ray.h` — ray class
- `include/interval.h` — simple `[min, max]` interval helper
- `include/color.h` — gamma-corrected PPM color output
- `include/hittable.h` / `hittable_list.h` — intersection abstraction + scene container
- `include/sphere.h` — sphere geometry
- `include/material.h` — `lambertian` (diffuse), `metal`, `dielectric` (glass) materials
- `include/camera.h` — camera, antialiasing, defocus blur, multithreaded render loop
- `src/main.cpp` — scene setup (currently the classic "many spheres" final scene)

## Tuning

In `src/main.cpp`, on the `camera` object:

- `image_width` / `aspect_ratio` — output resolution
- `samples_per_pixel` — antialiasing / noise reduction (higher = cleaner, slower)
- `max_depth` — max ray bounces
- `vfov`, `lookfrom`, `lookat`, `vup` — camera framing
- `defocus_angle`, `focus_dist` — depth of field

Rendering is parallelized across all available CPU cores automatically.

## Renders

See `renders/` for milestone images produced while building this out:
gradient background → surface normals → diffuse antialiasing → metal/glass
materials → final multi-sphere scene with depth of field.

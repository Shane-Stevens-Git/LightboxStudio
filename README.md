# Lightbox Studio

A 3D lighting and materials sandbox that runs in your browser. It's a single HTML
file built on [Three.js](https://threejs.org/), so there's nothing to install and no build step.

**Open it:** https://shane-stevens-git.github.io/LightboxStudio/

Or download [`sandbox/lightbox-studio.html`](sandbox/lightbox-studio.html) and open it in a browser.
It loads Three.js from a CDN, so it needs an internet connection.

## What you can do

- **Build a scene:** add boxes, spheres, cylinders, cones, capsules, and tori, plus point lights and a sun. Move, rotate, and scale them with a gizmo (`W` / `E` / `R`).
- **Play with light and materials:** edit materials and textures, make objects glow and act as lights, turn on bloom, and adjust shadow softness.
- **Set the stage:** pick a ground (plain, checker, dirt, grass, cement) and a backdrop (studio, clear sky, cloudy, overcast, sunset, night, a solid color, or your own image).
- **Edit faster:** multi-select, snapping, grouping, array and radial duplicates, undo and redo, camera presets, and a shortcuts cheat sheet (`?`).
- **Bring your own models:** import `.glb` / `.gltf` files.
- **Save your work:** save and load scenes as `.json` files, or start from a template (Studio Trio, Still Life, Neon Night).

## Where it started: a C++ ray tracer (early experiment)

This project began as a C++17 ray tracer following the
["Ray Tracing in One Weekend"](https://raytracing.github.io/) path. It's **no longer being developed**
and is kept here for reference; Lightbox Studio is the main project now.

The renderer supports spheres and triangle meshes, diffuse / metal / glass materials, antialiasing,
depth of field, a BVH for speed, and multithreaded rendering. Lightbox Studio's
**Export for Ray Tracer** button writes a `scene.json` that it can render.

Build and render (needs CMake and a C++17 compiler):

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4

./build/raytracer > renders/output.ppm              # the built-in many-spheres scene
./build/raytracer scene.json > renders/output.ppm   # a scene exported from Lightbox Studio
```

Convert the PPM to PNG with ImageMagick (`magick renders/output.ppm renders/output.png`) or Pillow.
The `renders/` folder has milestone images: gradient, surface normals, diffuse, metal and glass, then the final scene.

Renderer code lives in `include/` (vectors, rays, materials, camera, BVH, JSON parsing) and `src/main.cpp`.

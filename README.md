# Path Tracer – Physically Based Rendering Engine

Interactive GPU path tracer implemented with OpenGL and GLSL, exploring modern physically-based rendering techniques.

![render](outputs/animation/output4.gif)

## Overview

This project is a fully GPU-based path tracing engine designed to simulate realistic light transport using **Monte Carlo integration**.

It progressively converges toward the solution of the rendering equation by accumulating samples over time, allowing interactive exploration of physically-based materials and lighting.

Key features include:

* Physically-based shading models
* Advanced sampling techniques
* Acceleration structures for complex scenes
* Basic animation system

---

## Gallery

All renders are available in the [`outputs/`](outputs/) directory.

| Diffuse (EON)        | Metals (GGX)         | Dielectrics                 |
| -------------------- | -------------------- | --------------------------- |
| ![](outputs/orenNayar.png) | ![](outputs/GGX3%20(1).png) | ![](outputs/dielectric%20(1).png) |

| Glossy Materials        | BVH Visualization    |
| ----------------------- | -------------------- |
| ![](outputs/GGX1%20(1).png) | ![](outputs/bvh.png) |

---

## Features

### Rendering

* **Monte Carlo Path Tracing**

  * Progressive rendering with temporal accumulation
  * Russian Roulette termination

* **Multi-Importance Sampling (MIS)**

  * Combines direct light sampling and global illumination
  * Faster convergence, reduced noise

* **Physically-Based Materials**

  * Diffuse (Lambert, Oren-Nayar / EON)
  * Metals (Cook-Torrance + GGX)
  * Dielectrics (refraction, Fresnel)
  * Glossy materials (hybrid models)

* **Volumetric Effects**

  * Beer-Lambert absorption for colored dielectrics

* **Stability Improvements**

  * Firefly reduction via throughput clamping

---

### Geometry

* Triangle mesh rendering
* GPU-friendly **Bounding Volume Hierarchy (BVH)**
* Support for complex 3D models

---

### Animation

* Keyframe-based animation system
* Linear interpolation between frames
* Offline animation rendering

---

## Technical Highlights

### Rendering Equation

The engine is based on the classical rendering equation:

$L_o = L_e + \int_{\Omega} f_r L_i (\omega_i \cdot n)\, d\omega_i$

Estimated using stochastic sampling (Monte Carlo).

---

## Build & Run

### Requirements

* Windows
* MinGW (g++ with C++17 support)
* OpenGL 4.3+
* GLFW (provided in `lib/`)
* Make (mingw32-make recommended)

### Build & Run (Makefile)

```bash
make
./myprogram.exe
```

### Alternative: Build with VSCode

If you're using VSCode, you can simply press:

```text
Ctrl + Shift + B
```

This uses the provided `tasks.json`.

---

### Notes

* Make sure `glfw3.dll` is in the root directory (same folder as `myprogram.exe`)
* If `make` is not recognized, ensure MinGW is added to your PATH
* All dependencies (ImGui, lodepng, tinyobjloader) are included in the repository

---

## Controls (example)

* Camera movement: `WASD`
* Mouse: Look around
* UI: Toggle with P

---

## Project Structure

```text
Path-Tracer/
├── .vscode/
├── include/
├── lib/
├── models/                 # 3D models
├── outputs/                # Rendered images / animations
├── src/
│   ├── shaders/
│   ...
...
├── myprogram.exe
├── Makefile
```

---

## Future Improvements

* Depth of field
* Adaptive sampling (variance-based)
* Better BVH construction (SAH)
* Improved caustics (photon mapping)
* Physics-based animation
* Scene loading system

---

## Author

**Baptiste Girardin** \
Télécom Paris \
[baptiste.girardin37@gmail.com](mailto:baptiste.girardin37@gmail.com) \
[baptiste.girardin@telecom-paris.fr](mailto:baptiste.girardin@telecom-paris.fr)

---

## References

* Cook-Torrance BRDF
* GGX Microfacet Model
* Oren-Nayar / EON diffuse model
* Monte Carlo Rendering
* BVH acceleration structures

---

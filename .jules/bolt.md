## 2026-10-06 - Optimize GML Variable Resolution Ordering
**Learning:** In interpreted GML steps, `get_var()` and `set_var()` are called tens of thousands of times per second. Placing custom instance variable array iteration (`s->vars`) and common built-ins (`x`, `y`, `hspeed`, `vspeed`, `image_index`, `sprite_index`) BEFORE global resource array scans (`p->rt->sprites`, `p->rt->objects`) avoids costly linear string matches across game resource lists during variable lookup.
**Action:** Always place high-frequency instance variable checks ahead of global/resource lookup loops in interpreter/evaluator hot paths.

## 2026-10-07 - Fast 2D AABB Early Exit in Line Collisions
**Learning:** Precomputing line segment bounding boxes before iterating instances in `gml_collision_line` avoids running expensive parametric ray clipping (`line_aabb_overlap`) for instances outside the segment's 2D extent. However, strict inequalities (`o->x + ow < min_lx || o->x > max_lx`) must be used for AABB disjointness checks to prevent false rejections on vertical or horizontal lines where `min_lx == max_lx`.
**Action:** Always precompute query bounding boxes outside instance collision loops, and use strict inequalities (`<` and `>`) for AABB range disjointness tests.

## 2026-10-08 - 2D Bounding Box Pre-Filter & Inverse Radii for Collision Ellipse
**Learning:** In `gml_collision_ellipse`, adding a 2D AABB bounding box pre-filter (`if (o->x + ow < x1 || o->x > x2 || ...)`) and precomputing inverse radii (`1.0 / rx`, `1.0 / ry`) outside the instance iteration loop bypasses double divisions and distance math for instances outside the ellipse bounds. Furthermore, a 1D horizontal distance early exit (`dx_sq > 1.0`) avoids Y-axis calculations for non-colliding instances.
**Action:** Always pre-filter geometry collision tests with fast AABB bounding box checks and pre-calculate inverted scale factors outside loops.

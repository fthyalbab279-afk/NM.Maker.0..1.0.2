## 2026-10-06 - Optimize GML Variable Resolution Ordering
**Learning:** In interpreted GML steps, `get_var()` and `set_var()` are called tens of thousands of times per second. Placing custom instance variable array iteration (`s->vars`) and common built-ins (`x`, `y`, `hspeed`, `vspeed`, `image_index`, `sprite_index`) BEFORE global resource array scans (`p->rt->sprites`, `p->rt->objects`) avoids costly linear string matches across game resource lists during variable lookup.
**Action:** Always place high-frequency instance variable checks ahead of global/resource lookup loops in interpreter/evaluator hot paths.

## 2026-10-07 - Fast 2D AABB Early Exit in Line Collisions
**Learning:** Precomputing line segment bounding boxes before iterating instances in `gml_collision_line` avoids running expensive parametric ray clipping (`line_aabb_overlap`) for instances outside the segment's 2D extent. However, strict inequalities (`o->x + ow < min_lx || o->x > max_lx`) must be used for AABB disjointness checks to prevent false rejections on vertical or horizontal lines where `min_lx == max_lx`.
**Action:** Always precompute query bounding boxes outside instance collision loops, and use strict inequalities (`<` and `>`) for AABB range disjointness tests.

#ifndef AABB_H
#define AABB_H

#include "interval.h"
#include "ray.h"
#include "vec3.h"

// Axis-aligned bounding box: a min/max point3 pair, used by bvh_node to
// quickly rule out rays that can't possibly hit what's inside the box.
class aabb {
public:
    interval x, y, z;

    // Default box is empty (matches interval's default-empty behavior).
    aabb() {}

    aabb(const interval& x, const interval& y, const interval& z)
        : x(x), y(y), z(z) {}

    // Build the box from two opposite corners; the corners don't need to
    // already be in min/max order per axis.
    aabb(const point3& a, const point3& b) {
        x = (a.x() <= b.x()) ? interval(a.x(), b.x()) : interval(b.x(), a.x());
        y = (a.y() <= b.y()) ? interval(a.y(), b.y()) : interval(b.y(), a.y());
        z = (a.z() <= b.z()) ? interval(a.z(), b.z()) : interval(b.z(), a.z());
    }

    // Smallest box that contains both box0 and box1.
    aabb(const aabb& box0, const aabb& box1) {
        x = interval(std::fmin(box0.x.min, box1.x.min), std::fmax(box0.x.max, box1.x.max));
        y = interval(std::fmin(box0.y.min, box1.y.min), std::fmax(box0.y.max, box1.y.max));
        z = interval(std::fmin(box0.z.min, box1.z.min), std::fmax(box0.z.max, box1.z.max));
    }

    const interval& axis_interval(int n) const {
        if (n == 1) return y;
        if (n == 2) return z;
        return x;
    }

    // Slab test: intersect the ray against each axis's interval and shrink
    // ray_t down to the overlap of all three; if that overlap is empty at
    // any point, the ray misses the box.
    bool hit(const ray& r, interval ray_t) const {
        const point3& origin = r.origin();
        const vec3& direction = r.direction();

        for (int axis = 0; axis < 3; axis++) {
            const interval& ax = axis_interval(axis);
            double adinv = 1.0 / direction[axis];

            double t0 = (ax.min - origin[axis]) * adinv;
            double t1 = (ax.max - origin[axis]) * adinv;

            if (t0 > t1) std::swap(t0, t1);

            if (t0 > ray_t.min) ray_t.min = t0;
            if (t1 < ray_t.max) ray_t.max = t1;

            if (ray_t.max <= ray_t.min)
                return false;
        }
        return true;
    }

    // Index of the box's longest axis (0=x, 1=y, 2=z) — bvh_node splits
    // along this axis so each half narrows the box as much as possible.
    int longest_axis() const {
        if (x.size() > y.size())
            return x.size() > z.size() ? 0 : 2;
        else
            return y.size() > z.size() ? 1 : 2;
    }

    static const aabb empty, universe;
};

inline const aabb aabb::empty = aabb(interval::empty, interval::empty, interval::empty);
inline const aabb aabb::universe = aabb(interval::universe, interval::universe, interval::universe);

#endif

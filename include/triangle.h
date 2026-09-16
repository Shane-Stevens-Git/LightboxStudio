#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "aabb.h"
#include "hittable.h"
#include "vec3.h"

// A single triangle, given as three world-space vertices. This is the
// one new primitive Phase 3 needed: rather than writing separate
// oriented-box and cylinder intersection routines (both of which can be
// arbitrarily rotated in Lightbox Studio), box/cylinder/imported glTF
// meshes are all triangulated client-side at export time and rendered
// here as plain triangle soups sitting inside the same BVH as everything
// else.
class triangle : public hittable {
public:
    triangle(const point3& v0, const point3& v1, const point3& v2, shared_ptr<material> mat)
        : v0(v0), v1(v1), v2(v2), mat(mat) {
        point3 min_p(std::fmin(v0.x(), std::fmin(v1.x(), v2.x())),
                     std::fmin(v0.y(), std::fmin(v1.y(), v2.y())),
                     std::fmin(v0.z(), std::fmin(v1.z(), v2.z())));
        point3 max_p(std::fmax(v0.x(), std::fmax(v1.x(), v2.x())),
                     std::fmax(v0.y(), std::fmax(v1.y(), v2.y())),
                     std::fmax(v0.z(), std::fmax(v1.z(), v2.z())));
        // Pad a touch on every axis so a triangle that's perfectly flat
        // against one axis (very common — e.g. a box face) still gets a
        // bounding box with nonzero thickness everywhere, which keeps the
        // BVH's slab test well-behaved.
        const double pad = 1e-4;
        vec3 padding(pad, pad, pad);
        bbox = aabb(min_p - padding, max_p + padding);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Moller-Trumbore ray-triangle intersection.
        vec3 edge1 = v1 - v0;
        vec3 edge2 = v2 - v0;
        vec3 pvec = cross(r.direction(), edge2);
        double det = dot(edge1, pvec);

        const double eps = 1e-8;
        if (std::fabs(det) < eps)
            return false; // Ray is parallel to the triangle's plane.

        double inv_det = 1.0 / det;
        vec3 tvec = r.origin() - v0;
        double u = dot(tvec, pvec) * inv_det;
        if (u < 0.0 || u > 1.0)
            return false;

        vec3 qvec = cross(tvec, edge1);
        double v = dot(r.direction(), qvec) * inv_det;
        if (v < 0.0 || u + v > 1.0)
            return false;

        double t = dot(edge2, qvec) * inv_det;
        if (!ray_t.surrounds(t))
            return false;

        rec.t = t;
        rec.p = r.at(t);
        vec3 outward_normal = unit_vector(cross(edge1, edge2));
        rec.set_face_normal(r, outward_normal);
        rec.mat = mat;

        return true;
    }

    aabb bounding_box() const override { return bbox; }

private:
    point3 v0, v1, v2;
    shared_ptr<material> mat;
    aabb bbox;
};

#endif

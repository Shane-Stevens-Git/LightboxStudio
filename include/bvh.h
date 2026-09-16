#ifndef BVH_H
#define BVH_H

#include "aabb.h"
#include "hittable.h"
#include "hittable_list.h"
#include "rtweekend.h"

#include <algorithm>
#include <vector>

// A bounding volume hierarchy: a binary tree of aabbs over the scene's
// objects. Instead of testing every ray against every object (O(n) per
// ray, what plain hittable_list does), a ray first tests the current
// node's box — if it misses the box, everything inside is skipped in one
// check, so the whole tree walk is roughly O(log n) per ray for a
// reasonably balanced scene.
//
// Built once, up front, from the full object list (this is the classic
// Ray Tracing in One Weekend approach): pick the box's longest axis,
// sort the objects along it, and split at the median into two halves,
// recursing until a leaf holds 1-2 objects.
class bvh_node : public hittable {
public:
    bvh_node(hittable_list list) : bvh_node(list.objects, 0, list.objects.size()) {}

    bvh_node(std::vector<shared_ptr<hittable>>& objects, size_t start, size_t end) {
        bbox = aabb::empty;
        for (size_t i = start; i < end; i++)
            bbox = aabb(bbox, objects[i]->bounding_box());

        int axis = bbox.longest_axis();
        auto comparator = (axis == 0) ? box_x_compare
                         : (axis == 1) ? box_y_compare
                                       : box_z_compare;

        size_t object_span = end - start;

        if (object_span == 1) {
            left = right = objects[start];
        } else if (object_span == 2) {
            left = objects[start];
            right = objects[start + 1];
        } else {
            std::sort(objects.begin() + start, objects.begin() + end, comparator);

            size_t mid = start + object_span / 2;
            left = make_shared<bvh_node>(objects, start, mid);
            right = make_shared<bvh_node>(objects, mid, end);
        }
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (!bbox.hit(r, ray_t))
            return false;

        bool hit_left = left->hit(r, ray_t, rec);
        bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);

        return hit_left || hit_right;
    }

    aabb bounding_box() const override { return bbox; }

private:
    shared_ptr<hittable> left;
    shared_ptr<hittable> right;
    aabb bbox;

    static bool box_compare(const shared_ptr<hittable>& a, const shared_ptr<hittable>& b, int axis_index) {
        auto a_axis_interval = a->bounding_box().axis_interval(axis_index);
        auto b_axis_interval = b->bounding_box().axis_interval(axis_index);
        return a_axis_interval.min < b_axis_interval.min;
    }

    static bool box_x_compare(const shared_ptr<hittable>& a, const shared_ptr<hittable>& b) {
        return box_compare(a, b, 0);
    }
    static bool box_y_compare(const shared_ptr<hittable>& a, const shared_ptr<hittable>& b) {
        return box_compare(a, b, 1);
    }
    static bool box_z_compare(const shared_ptr<hittable>& a, const shared_ptr<hittable>& b) {
        return box_compare(a, b, 2);
    }
};

#endif

#include "bvh.h"
#include "camera.h"
#include "color.h"
#include "hittable_list.h"
#include "json.h"
#include "material.h"
#include "rtweekend.h"
#include "sphere.h"
#include "triangle.h"
#include "vec3.h"

#include <fstream>
#include <iostream>
#include <sstream>

hittable_list final_scene() {
    hittable_list world;

    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            double choose_mat = random_double();
            point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    color albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    color albedo = color::random(0.5, 1);
                    double fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    return world;
}

// ---------------------------------------------------------------------
// Phase 3: load a scene exported from Lightbox Studio (see
// sandbox/lightbox-studio.html's "Export for Ray Tracer" and the schema
// notes in the project docs). Every field below has a fallback, so a
// scene file only needs to specify what it wants to override.
// ---------------------------------------------------------------------

vec3 json_vec3(const json_value& v, const vec3& fallback = vec3(0, 0, 0)) {
    if (!v.is_array() || v.arr_val.size() < 3) return fallback;
    return vec3(v.arr_val[0].as_double(), v.arr_val[1].as_double(), v.arr_val[2].as_double());
}

shared_ptr<material> material_from_json(const json_value& m) {
    std::string type = m.at("type").as_string("lambertian");

    if (type == "metal") {
        color albedo = json_vec3(m.at("albedo"), color(0.8, 0.8, 0.8));
        double fuzz = m.at("fuzz").as_double(0.0);
        return make_shared<metal>(albedo, fuzz);
    }
    if (type == "dielectric") {
        double ior = m.at("ior").as_double(1.5);
        return make_shared<dielectric>(ior);
    }
    if (type == "diffuse_light") {
        color emit = json_vec3(m.at("emit"), color(1, 1, 1));
        return make_shared<diffuse_light>(emit);
    }
    // "lambertian" or unrecognized -- fall back to diffuse rather than
    // fail the whole scene over one bad material entry.
    color albedo = json_vec3(m.at("albedo"), color(0.7, 0.7, 0.7));
    return make_shared<lambertian>(albedo);
}

hittable_list scene_from_json(const json_value& root, int& sphere_count, int& triangle_count) {
    hittable_list world;
    sphere_count = 0;
    triangle_count = 0;

    const json_value& objs = root.at("objects");
    if (!objs.is_array()) return world;

    for (const auto& o : objs.arr_val) {
        std::string type = o.at("type").as_string();
        shared_ptr<material> mat = material_from_json(o.at("material"));

        if (type == "sphere") {
            point3 center = json_vec3(o.at("center"));
            double radius = o.at("radius").as_double(0.5);
            world.add(make_shared<sphere>(center, radius, mat));
            sphere_count++;
        } else if (type == "mesh") {
            const json_value& tris = o.at("triangles");
            if (!tris.is_array()) continue;
            for (const auto& tri : tris.arr_val) {
                if (!tri.is_array() || tri.arr_val.size() < 3) continue;
                point3 v0 = json_vec3(tri.arr_val[0]);
                point3 v1 = json_vec3(tri.arr_val[1]);
                point3 v2 = json_vec3(tri.arr_val[2]);
                world.add(make_shared<triangle>(v0, v1, v2, mat));
                triangle_count++;
            }
        }
    }

    return world;
}

bool render_from_scene_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Could not open scene file: " << path << "\n";
        return false;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();

    json_value root;
    try {
        root = parse_json(buffer.str());
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse scene file: " << e.what() << "\n";
        return false;
    }

    int sphere_count = 0, triangle_count = 0;
    hittable_list world = scene_from_json(root, sphere_count, triangle_count);
    world = hittable_list(make_shared<bvh_node>(world));

    camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 1200;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    cam.vfov = 20;
    cam.lookfrom = point3(13, 2, 3);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);
    cam.defocus_angle = 0;
    cam.focus_dist = 10.0;

    const json_value& r = root.at("render");
    if (!r.is_null()) {
        cam.image_width = static_cast<int>(r.at("imageWidth").as_double(cam.image_width));
        cam.aspect_ratio = r.at("aspectRatio").as_double(cam.aspect_ratio);
        cam.samples_per_pixel = static_cast<int>(r.at("samplesPerPixel").as_double(cam.samples_per_pixel));
        cam.max_depth = static_cast<int>(r.at("maxDepth").as_double(cam.max_depth));
    }
    const json_value& c = root.at("camera");
    if (!c.is_null()) {
        cam.lookfrom = json_vec3(c.at("lookfrom"), cam.lookfrom);
        cam.lookat = json_vec3(c.at("lookat"), cam.lookat);
        cam.vup = json_vec3(c.at("vup"), cam.vup);
        cam.vfov = c.at("vfov").as_double(cam.vfov);
        cam.defocus_angle = c.at("defocusAngle").as_double(cam.defocus_angle);
        cam.focus_dist = c.at("focusDist").as_double(cam.focus_dist);
    }

    std::cerr << "Loaded scene: " << path << " (" << sphere_count << " spheres, "
              << triangle_count << " triangles)\n";

    cam.render(world, std::cout);
    return true;
}

int main(int argc, char** argv) {
    // No argument: render the original built-in "many spheres" scene,
    // exactly as before. Pass a scene.json path (exported from Lightbox
    // Studio) to render that instead.
    if (argc > 1) {
        return render_from_scene_file(argv[1]) ? 0 : 1;
    }

    hittable_list world = final_scene();
    world = hittable_list(make_shared<bvh_node>(world));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 1200;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;

    cam.vfov = 20;
    cam.lookfrom = point3(13, 2, 3);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.6;
    cam.focus_dist = 10.0;

    cam.render(world, std::cout);
    return 0;
}

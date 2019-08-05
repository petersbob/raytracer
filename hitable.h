#ifndef HITABLEH
#define HITABLEH

#include "ray.h"
#include "aabb.h"
#include "float.h"

class material;

struct hit_record {
    float t;
    vec3 p;
    vec3 normal;
    material *mat_ptr;
    float u, v;
};

class hitable {
    public:
    virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const = 0;
    virtual bool bounding_box(float t0, float t1, aabb& box) const = 0;
};

int box_x_compare (const void * a, const void *b) {
    aabb box_left, box_right;
    hitable *ah = *(hitable**)a;
    hitable *bh = *(hitable**)a;
    if (!ah->bounding_box(0,0, box_left) || !bh->bounding_box(0,0, box_right))
        std::cerr << "no bounding box in bvh_onde constructor\n";
    if (box_left.min().x() - box_right.min().x() < 0.0 )
        return -1;
    else 
        return 1;
}

int box_y_compare (const void * a, const void *b) {
    aabb box_left, box_right;
    hitable *ah = *(hitable**)a;
    hitable *bh = *(hitable**)a;
    if (!ah->bounding_box(0,0, box_left) || !bh->bounding_box(0,0, box_right))
        std::cerr << "no bounding box in bvh_onde constructor\n";
    if (box_left.min().y() - box_right.min().y() < 0.0 )
        return -1;
    else 
        return 1;
}

int box_z_compare (const void * a, const void *b) {
    aabb box_left, box_right;
    hitable *ah = *(hitable**)a;
    hitable *bh = *(hitable**)a;
    if (!ah->bounding_box(0,0, box_left) || !bh->bounding_box(0,0, box_right))
        std::cerr << "no bounding box in bvh_onde constructor\n";
    if (box_left.min().z() - box_right.min().z() < 0.0 )
        return -1;
    else 
        return 1;
}

class bvh_node : public hitable {
    public:
        bvh_node() {}
        bvh_node(hitable **l, int n , float time0, float time1);
        virtual bool hit(const ray& r, float tmin, float tmax, hit_record& rec) const;
        virtual bool bounding_box(float t0, float t1, aabb& box) const;
        hitable *left;
        hitable *right;
        aabb box;
};

bool bvh_node::bounding_box(float t0, float t1, aabb& b) const {
    b = box;
    return true;
}

bvh_node::bvh_node(hitable **l, int n, float time0, float time1) {
    int axis = int(3*drand48());
    if (axis == 0)
        qsort(l, n, sizeof(hitable *), box_x_compare);
    else if (axis == 1)
        qsort(l, n, sizeof(hitable *), box_y_compare);
    else
        qsort(l, n, sizeof(hitable *), box_z_compare);
    
    if (n == 1) {
        left = right = l[0];
    } else if (n == 2) {
        left = l[0];
        right = l[1];
    } else {
        left = new bvh_node(l, n/2, time0, time1);
        right = new bvh_node(l + n/2, n-n/2, time0, time1);
    }
    aabb box_left, box_right;
    if(!left->bounding_box(time0, time1, box_left) || !right->bounding_box(time0,time1, box_right))
        std::cerr << "no bounding box in bvh_node constructor\n";
    box = surrounding_box(box_left, box_right);
}

bool bvh_node::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
    if (box.hit(r, t_min, t_max)) {
        hit_record left_rec, right_rec;
        bool hit_left = left->hit(r, t_min, t_max, left_rec);
        bool hit_right = right->hit(r, t_min, t_max, right_rec);
        if (hit_left && hit_right) {
            if (left_rec.t < right_rec.t)
                rec = left_rec;
            else
                rec = right_rec;
            return true;
        } else if (hit_left) {
            rec = left_rec;
            return true;
        } else if (hit_right) {
            rec = right_rec;
            return true;
        } else
            return false;
    } else
        return false;
}

class flip_normals : public hitable {
    public:
        flip_normals(hitable *p) : ptr(p) {}
        virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
            if (ptr->hit(r, t_min, t_max, rec)) {
                rec.normal = -rec.normal;
                return true;
            } else {
                return false;
            }
        }
        virtual bool bounding_box(float t0, float t1, aabb& box) const {
            return ptr->bounding_box(t0, t1, box);
        }

        hitable *ptr;
};

class translate : public hitable {
    public:
        translate(hitable *p, const vec3& displacement) : ptr(p), offset(displacement) {}
        virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const;
        virtual bool bounding_box(float t0, float t1, aabb& box) const;
        hitable *ptr;
        vec3 offset;
};

bool translate::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
    ray moved_r(r.origin() - offset, r.direction(), r.time());
    if (ptr->hit(moved_r, t_min, t_max, rec)) {
        rec.p += offset;
        return true;
    } else {
        return false;
    }
}

bool translate::bounding_box(float t0, float t1, aabb& box) const {
    if (ptr->bounding_box(t0, t1, box)) {
        box = aabb(box.min() + offset, box.max() + offset);
        return true;
    } else {
        return false;
    }
}

class rotate_y : public hitable {
    public:
        rotate_y(hitable *p, float angle);
        virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const;
        virtual bool bounding_box(float t0, float t1, aabb& box) const {
            box = bbox; return hasbox;
        }
        hitable *ptr;
        float sin_theta;
        float cos_theta;
        bool hasbox;
        aabb bbox;
};

rotate_y::rotate_y(hitable *p, float angle) : ptr(p) {
    float radians = (M_PI / 180) * angle;
    sin_theta = sin(radians);
    cos_theta = cos(radians);
    hasbox = ptr->bounding_box(0, 1, bbox);
    vec3 min(FLT_MAX, FLT_MAX, FLT_MAX);
    vec3 max(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (int i=0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k=0; k < 2; k++) {
                float x = i*bbox.max().x() + (1-i)*bbox.min().x();
                float y = j*bbox.max().y() + (1-j)*bbox.min().y();
                float z = k*bbox.max().z() + (1-k)*bbox.min().z();
                float newx = cos_theta*x + sin_theta*z;
                float newz = -sin_theta*x + cos_theta*z;
                vec3 tester(newx, y, newz);
                for (int c = 0; c < 3; c++) {
                    if (tester[c] > max[c])
                        max[c] = tester[c];
                    if (tester[c] < min[c])
                        min[c] = tester[c];
                }
            }
        }
    }
    bbox = aabb(min, max);
}

bool rotate_y::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
    vec3 origin = r.origin();
    vec3 direction = r.direction();
    origin[0] = cos_theta*r.origin()[0] - sin_theta*r.origin()[2];
    origin[2] = sin_theta*r.origin()[0] + cos_theta*r.origin()[2];
    direction[0] = cos_theta*r.direction()[0] - sin_theta*r.direction()[2];
    direction[2] = sin_theta*r.direction()[0] + cos_theta*r.direction()[2];
    ray rotated_r(origin, direction, r.time());

    if (ptr->hit(rotated_r, t_min, t_max, rec)) {
        vec3 p = rec.p;
        vec3 normal = rec.normal;
        p[0] = cos_theta*rec.p[0] + sin_theta*rec.p[2];
        p[2] = -sin_theta*rec.p[0] + cos_theta*rec.p[2];
        normal[0] = cos_theta*rec.normal[0] + sin_theta*rec.normal[2];
        normal[2] = -sin_theta*rec.normal[0] + cos_theta*rec.normal[2];
        rec.p = p;
        rec.normal = normal;
        return true;
    } else {
        return false;
    }
}

#endif
#include <fstream>
#include <iostream>
#include <string>
#include <time.h>
#include <vector>

#include "ray.h"
#include "sphere.h"
#include "rectangle.h"
#include "box.h"
#include "hitable_list.h"
#include "float.h"
#include "camera.h"
#include "material.h"
#include "parallel.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct Options {
    std::string fileName = "image.jpg";
    int nSamples = 1;
    int xResolution = 600;
    int yResolution = 300;
};

vec3 color(const ray& r, hitable *world, int depth) {
    hit_record rec;
    if (world->hit(r, 0.001,FLT_MAX, rec)) {
        ray scattered_ray;
        vec3 attenuation = vec3(0.5,0.5,0.5);
        vec3 emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);
        if (depth < 50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered_ray)) {
            return emitted + attenuation*color(scattered_ray, world, depth+1);
        } else {
            return emitted;
        }
    } else {
        return vec3(0,0,0);
    }
}

hitable *random_scene(unsigned char **tex_data) {
    vec3 colors[6] = {
            vec3(0.37,0.62,0.58),
            vec3(0.24,0.21,0.22),
            vec3(0.45,0.21,0.20),
            vec3(0.71,0.38,0.22),
            vec3(0.69,0.63,0.64),
            vec3(0.89,0.85,0.82),
    };

    int n = 500;
    hitable **list = new hitable*[n+1];
    list[0] =  new sphere(vec3(0,-1000,0), 1000, new diffuse_light(new constant_texture(vec3(1.1,1.1,1.1))));

    int i = 1;
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            float choose_mat = drand48();
            vec3 center(a+0.9*drand48(),0.2,b+0.9*drand48());
            vec3 color;
            color = colors[ int(drand48()*5) ];

            if ((center-vec3(4,0.2,0)).length() > 0.9) { 
                if (choose_mat < 0.3) {  // diffuse
                    list[i++] = new sphere(center, 0.2, new lambertian(new constant_texture(color)));
                }
                else if (choose_mat < 0.6) { // metal
                    list[i++] = new sphere(center, 0.2, new metal(vec3(0.5*(1 + drand48()), 0.5*(1 + drand48()), 0.5*(1 + drand48())),  0.5*drand48()));
                }
                else {  // glass
                    list[i++] = new sphere(center, 0.2, new dielectric(1.5));
                }
            }
        }
    }

    list[i++] = new sphere(vec3(0, 1, 0), 1.0, new dielectric(1.5));

    int nx, ny, nn;
    *tex_data = stbi_load("textures/earth.jpg", &nx, &ny, &nn, 0);
    if (tex_data == NULL) {
        std::cout << "Error: texture could not be loaded!" << std::endl;
        return NULL;
    }

    material *mat = new lambertian(new image_texture(*tex_data, nx, ny));
    list[i++] = new sphere(vec3(4, 1, 0), 1.0, mat);

    list[i++] = new sphere(vec3(-4, 1, 0), 1.0, new metal(colors[4], 0.0));
    return new bvh_node(list,i,0.0, 1.0);
}

hitable *cornell_box() {
    hitable **list = new hitable*[8];
    int i = 0;
    material *red = new lambertian( new constant_texture(vec3(0.65, 0.05, 0.05)) );
    material *white = new lambertian( new constant_texture(vec3(0.73, 0.73, 0.73)) );
    material *green = new lambertian( new constant_texture(vec3(0.12, 0.45, 0.15)) );
    material *light = new diffuse_light( new constant_texture(vec3(15, 15, 15)) );
    list[i++] = new flip_normals(new yz_rect(0, 555, 0, 555, 555, green));
    list[i++] = new yz_rect(0, 555, 0, 555, 0, red);
    list[i++] = new xz_rect(213, 343, 227, 332, 554, light);
    list[i++] = new flip_normals(new xz_rect(0, 555, 0, 555, 555, white));
    list[i++] = new xz_rect(0, 555, 0, 555, 0, white);
    list[i++] = new flip_normals(new xy_rect(0, 555, 0, 555, 555, white));

    list[i++] = new translate(new rotate_y(new box(vec3(0, 0, 0), vec3(165, 165, 165), white), -18), vec3(130,0,65));
    list[i++] = new translate(new rotate_y(new box(vec3(0, 0, 0), vec3(165, 330, 165), white), 15), vec3(265,0,295));

    return new hitable_list(list,i);
}

int main(int argc, char *argv[]) {
    Options options;

    for (int i=1; i<argc;i++) {
        std::string argString = argv[i];
        if (argString.substr(0,11) == "--fileName="){
            options.fileName = argString.substr(11,argString.length());
        } else if (argString.substr(0,11) == "--nSamples=") {
            options.nSamples = stoi(argString.substr(11,argString.length()));
        } else if (argString.substr(0,14) == "--xResolution=") {
            options.xResolution = stoi(argString.substr(14,argString.length()));
        } else if (argString.substr(0,14) == "--yResolution=") {
            options.yResolution = stoi(argString.substr(14,argString.length()));
        } else {
            std::cout << "Error: parameter \"" << argString << "\" unknown!" << std::endl;
            return 0;
        }
    }

    srand(time(0));

    std::cout<< "Samples: " << options.nSamples << std::endl;
    std::cout<< "Resolution " << options.xResolution << " " << options.yResolution << std::endl;
    std::cout<< "Creating image " << options.fileName << "..." << std::endl;

    unsigned char *tex_data;
    //hitable *world = random_scene(&tex_data);
    hitable *world = cornell_box();
    // if (tex_data == NULL || world == NULL) {
    //     std::cout << "Error: creating scene has failed" << std::endl;
    //     return 0;
    // }

    vec3 lookfrom(278,278,-800);
    vec3 lookat(278,278,0);
    float dist_to_focus = 10;
    float aperture = 0.0;

    camera cam(lookfrom, lookat, vec3(0,1,0), 40, float(options.xResolution)/float(options.yResolution), aperture, dist_to_focus, 0, 1);

    char* image;
    image = new char[options.xResolution*options.yResolution*3];

    parallel_for_each(0, options.yResolution, [=,&image,&cam](int j){
        for (int i=0; i < options.xResolution; i++) {
                vec3 col(0,0,0);
                for (int s=0; s < options.nSamples; s++) {
                    float u = float(i + random_float()) / float(options.xResolution);
                    float v = float(j + random_float()) / float(options.yResolution);
                    ray r = cam.get_ray(u, v);
                    vec3 p = r.point_at_parameter(2.0);
                    col += color(r, world, 0);
                }
                col /= float(options.nSamples);
                col = vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

                image[(j*options.xResolution*3) + (i*3)] = char(255.99*col[0]);
                image[(j*options.xResolution*3) + (i*3+1)] = int(255.99*col[1]);
                image[(j*options.xResolution*3) + (i*3+2)] = int(255.99*col[2]);
            }
    });

    // stbi_image_free(tex_data);

    int fileNameSize = options.fileName.size();
    char cFileName[fileNameSize+1];
    options.fileName.copy(cFileName, fileNameSize + 1);
    cFileName[fileNameSize] = '\0';
    
    stbi_flip_vertically_on_write(1);
    int success = stbi_write_jpg(cFileName, options.xResolution, options.yResolution, 3, image, 100);
    if (!success) {
        std::cout << "Error: writing to file failed!" << std::endl;
    }
}
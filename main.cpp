#include <fstream>
#include <iostream>
#include <string>
#include <time.h>
#include <vector>

#include "ray.h"
#include "sphere.h"
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
        if (depth < 50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered_ray)) {
            return attenuation*color(scattered_ray, world, depth+1);
        } else {
            return vec3(0,0,0);
        }
    } else {
        vec3 unit_direction = unit_vector(r.direction());
        float t = 0.5*(unit_direction.y() + 1.0);
        return (1.0-t)*vec3(1.0,1.0,1.0) + t*vec3(0.5,0.7,1.0);
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
    list[0] =  new sphere(vec3(0,-1000,0), 1000, new lambertain(new checker_texture(new constant_texture(colors[1]), new constant_texture(colors[2]))));

    int i = 1;
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            float choose_mat = drand48();
            vec3 center(a+0.9*drand48(),0.2,b+0.9*drand48());
            vec3 color;
            color = colors[ int(drand48()*5) ];

            if ((center-vec3(4,0.2,0)).length() > 0.9) { 
                if (choose_mat < 0.3) {  // diffuse
                    list[i++] = new sphere(center, 0.2, new lambertain(new constant_texture(color)));
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
    *tex_data = stbi_load("earth.jpg", &nx, &ny, &nn, 0);
    if (tex_data == NULL) {
        std::cout << "Error: texture could not be loaded!" << std::endl;
        return NULL;
    }

    material *mat = new lambertain(new image_texture(*tex_data, nx, ny));
    list[i++] = new sphere(vec3(4, 1, 0), 1.0, mat);

    list[i++] = new sphere(vec3(-4, 1, 0), 1.0, new metal(colors[4], 0.0));
    return new bvh_node(list,i,0.0, 1.0);
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
    hitable *world = random_scene(&tex_data);
    if (tex_data == NULL || world == NULL) {
        std::cout << "Error: creating scene has failed" << std::endl;
        return 0;
    }

    vec3 lookfrom(13,2,3);
    vec3 lookat(0,0,0);
    float dist_to_focus = 10;
    float aperture = 0.1;

    camera cam(lookfrom, lookat, vec3(0,1,0), 20, float(options.xResolution)/float(options.yResolution), aperture, dist_to_focus, 0, 1);

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

    stbi_image_free(tex_data);

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
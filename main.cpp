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

struct Options {
    std::string fileName = "image.ppm";
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

hitable *random_scene() {
    int n = 500;
    hitable **list = new hitable*[n+1];
    list[0] =  new sphere(vec3(0,-1000,0), 1000, new lambertain(vec3(0.5, 0.5, 0.5)));
    int i = 1;
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            float choose_mat = drand48();
            vec3 center(a+0.9*drand48(),0.2,b+0.9*drand48()); 
            if ((center-vec3(4,0.2,0)).length() > 0.9) { 
                if (choose_mat < 0.8) {  // diffuse
                    list[i++] = new sphere(center, 0.2, new lambertain(vec3(drand48()*drand48(), drand48()*drand48(), drand48()*drand48())));
                }
                else if (choose_mat < 0.95) { // metal
                    list[i++] = new sphere(center, 0.2,
                            new metal(vec3(0.5*(1 + drand48()), 0.5*(1 + drand48()), 0.5*(1 + drand48())),  0.5*drand48()));
                }
                else {  // glass
                    list[i++] = new sphere(center, 0.2, new dielectric(1.5));
                }
            }
        }
    }

    list[i++] = new sphere(vec3(0, 1, 0), 1.0, new dielectric(1.5));
    list[i++] = new sphere(vec3(-4, 1, 0), 1.0, new lambertain(vec3(0.4, 0.2, 0.1)));
    list[i++] = new sphere(vec3(4, 1, 0), 1.0, new metal(vec3(0.7, 0.6, 0.5), 0.0));

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
        }


    }

    srand(time(0));

    std::ofstream outfile;
    outfile.open(options.fileName);
    std::cout<< "Samples: " << options.nSamples << std::endl;
    std::cout<< "Resolution " << options.xResolution << " " << options.yResolution << std::endl;
    std::cout<< "Creating image " << options.fileName << "..." << std::endl;

    outfile << "P3" << std::endl;
    outfile << options.xResolution << " " << options.yResolution << std::endl;
    outfile << "255" << std::endl;

    hitable *list[4];
    list[0] = new sphere(vec3(0,0,-1), 0.5, new lambertain(vec3(0.1,0.2,0.5)));
    list[1] = new sphere(vec3(0,-100.5,-1), 100, new lambertain(vec3(0.8,0.8,0.0)));
    list[2] = new sphere(vec3(1,0,-1), 0.5, new metal(vec3(0.8,0.6,0.2),0.1));
    list[3] = new sphere(vec3(-1,0,-1), 0.5, new dielectric(1.5));
    hitable *world = new hitable_list(list, 4);
    world = random_scene();

    vec3 lookfrom(13,2,3);
    vec3 lookat(0,0,0);
    float dist_to_focus = 10;
    float aperture = 0.1;

    camera cam(lookfrom, lookat, vec3(0,1,0), 20, float(options.xResolution)/float(options.yResolution), aperture, dist_to_focus);

    std::vector< std::vector<int> > image(options.yResolution, std::vector<int> (options.xResolution*3, 0));

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

                image[j][i*3] = int(255.99*col[0]);
                image[j][i*3+1] = int(255.99*col[1]);
                image[j][i*3+2] = int(255.99*col[2]);
            }
    });

    for (int j=options.yResolution-1; j >= 0; j--) {
        for (int i=0; i < options.xResolution; i++) {
            outfile << image[j][i*3] << " " << image[j][i*3+1] << " " << image[j][i*3+2] << std::endl;
        }
    }   

    outfile.close();
}
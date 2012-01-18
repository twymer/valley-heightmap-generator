#include "SDL/SDL.h"
#include <time.h> /* For random seeding */
#include <math.h> /* For powers in random gen */
#include <stdio.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <libnoise/noise.h>
#include "noiseutils.h"

#define DEPTH 5
#define W_WIDTH 640
#define W_HEIGHT 480

#define REDUCE_RANGE_CONSTANT .5

using namespace noise;

typedef struct _point {
    int x;
    int y;
} point;

float random_by_depth(int depth) {
    float reduce_by = pow(REDUCE_RANGE_CONSTANT, depth + 1);
    float random = (-1+2*((float)rand())/RAND_MAX) * reduce_by;
    float max = 300;
    return max * random;
}

point* perturb_point(point* points, int before, int after, int depth) {
    if(depth >= DEPTH) {
        return points;
    }
    //printf("Perturbing point between %i and %i.\n", before, after);
    //printf("Currently at depth %i.\n", depth);

    point* midpoint;
    midpoint = (point*)malloc(sizeof(point));
    midpoint->x = (points[before].x + points[after].x) / 2;
    midpoint->y = (points[before].y + points[after].y) / 2;

    float x_rand = random_by_depth(depth);
    float y_rand = random_by_depth(depth);
    //printf("Random offsets are %f and %f.\n", x_rand, y_rand);
    /*midpoint->x += x_rand / 4;*/
    midpoint->y += y_rand;

    int current = (before + after) / 2;
    points[current] = *midpoint;

    points = perturb_point(points, before, current, depth + 1);
    points = perturb_point(points, current, after, depth + 1);
    return points;
}

point* form_line(int depth, point start, point end) {
    int total_points = depth * depth;
    point* points;
    points = (point*)malloc(sizeof(point) * total_points);
    points[0] = start;
    points[total_points - 1] = end;

    return perturb_point(points, 0, total_points - 1, 0);
}

float valley_function(int x) {
    return x * x + x;
}

// http://mathworld.wolfram.com/Point-LineDistance2-Dimensional.html
float distance_to_line(point p0, point p1, point p2) {
    float d = (p2.x - p1.x) * (p1.y - p0.y) - (p1.x - p0.x) * (p2.y - p1.y);
    d = abs(d) / sqrt((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y));
    return d;
}

int main() {
    // Initialize noise objects
    module::Perlin perlin;
    perlin.SetOctaveCount(6);
    perlin.SetFrequency(1.0);
    perlin.SetFrequency(0.5);

    utils::NoiseMap height_map;
    utils::NoiseMapBuilderPlane height_map_builder;
    height_map_builder.SetSourceModule(perlin);
    height_map_builder.SetDestNoiseMap(height_map);
    height_map_builder.SetDestSize(256, 256);
    height_map_builder.SetBounds(2.0, 6.0, 1.0, 5.0);
    height_map_builder.Build();

    // Do work

    /* random seed */
    srand((unsigned int)time(NULL));

    // Setup line
    point start, end;
    start.x = 5;
    start.y = 100;
    end.x = 250;
    end.y = 100;
    point* points;
    int total_points = DEPTH * DEPTH;
    points = form_line(DEPTH, start, end);

    int x, y;
    for(x = 0; x < 512; x++) {
        for(y = 0; y < 512; y++) {
            point current_point;
            current_point.x = x;
            current_point.y = y;
            float min_dist = 99999;

            // Find out how close this point is to a line
            int i;
            for(i = 0; i < total_points; i++) {
                point p0 = points[i];
                point p1 = points[i+1];
                float dist = minimum_distance(current_point, p0, p1);
                if(dist < min_dist) {
                    min_dist = dist;
                }
            }

            if(min_dist < 5) {
                height_map.SetValue(x, y, 0);
            } else {
                printf("min dist: %f, x: %i, y: %i\n", min_dist, x, y);
            }
        }
    }

    // Render the heightmap
    utils::RendererImage renderer;
    utils::Image image;
    renderer.SetSourceNoiseMap(height_map);
    renderer.SetDestImage(image);
    renderer.Render();

    utils::WriterBMP writer;
    writer.SetSourceImage(image);
    writer.SetDestFilename("heightmap.bmp");
    writer.WriteDestFile();

    /* HERE BE OPENGL DRAGONS */

    if(SDL_Init(SDL_INIT_EVERYTHING) == -1) {
        printf("Failed\n");
        return 0;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Surface* screen = SDL_SetVideoMode(W_WIDTH, W_HEIGHT, 16, SDL_OPENGL | SDL_RESIZABLE);

    glEnable(GL_TEXTURE_2D);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glViewport(0, 0, W_WIDTH, W_HEIGHT);

    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0.0f, W_WIDTH, W_HEIGHT, 0.0f, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glLineWidth(1);
    glBegin(GL_LINE_STRIP);
        int i;
        for(i = 0; i < total_points; i++) {
            glVertex2i(points[i].x, points[i].y);
        }
    glEnd();

    SDL_GL_SwapBuffers();
    SDL_Delay(3000);
}

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

typedef struct _vec2 {
    float x;
    float y;
} vec2;

vec2 operator-(const vec2 &v0, const vec2 &v1) {
    vec2 v2;
    v2.x = v0.x - v1.x;
    v2.y = v0.y - v1.y;
    return v2;
}

vec2 operator+(const vec2 &v0, const vec2 &v1) {
    vec2 v2;
    v2.x = v0.x + v1.x;
    v2.y = v0.y + v1.y;
    return v2;
}

vec2 operator*(const float &f, const vec2 &v) {
    vec2 product;
    product.x = f * v.x;
    product.y = f * v.y;
    return product;
}

float random_by_depth(int depth) {
    float reduce_by = pow(REDUCE_RANGE_CONSTANT, depth + 1);
    float random = (-1+2*((float)rand())/RAND_MAX) * reduce_by;
    float max = 300;
    return max * random;
}

vec2* perturb_point(vec2* points, int before, int after, int depth) {
    if(depth >= DEPTH) {
        return points;
    }
    //printf("Perturbing point between %i and %i.\n", before, after);
    //printf("Currently at depth %i.\n", depth);

    vec2* midpoint;
    midpoint = (vec2*)malloc(sizeof(vec2));
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

vec2* form_line(int depth, vec2 start, vec2 end) {
    int total_points = depth * depth;
    vec2* points;
    points = (vec2*)malloc(sizeof(vec2) * total_points);
    points[0] = start;
    points[total_points - 1] = end;

    return perturb_point(points, 0, total_points - 1, 0);
}

float valley_function(float x) {
    float val = 2 * ((2 * x) * (2 * x)) / 3.5 - 1.3;
    if(val > -1) {
        return val;
    } else {
        return -1;
    }
}

float length_squared(vec2 v, vec2 w) {
    return (w.x - v.x) * (w.x - v.x) + (w.y - v.y) * (w.y - v.y);
}

float distance(vec2 p0, vec2 p1) {
    return abs(p0.x - p1.x) + abs(p0.y - p1.y);
}

float dot(vec2 v, vec2 w) {
    return v.x * w.x + v.y * w.y;
}

// http://mathworld.wolfram.com/vec2-LineDistance2-Dimensional.html
// http://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment
float distance_to_line(vec2 v, vec2 w, vec2 p) {
    // Return minimum distance between line segment vw and vec2 p
    const float l2 = length_squared(v, w);  // i.e. |w-v|^2 -  avoid a sqrt
    if (l2 == 0.0) return distance(p, v);   // v == w case
    // Consider the line extending the segment, parameterized as v + t (w - v).
    // We find projection of vec2 p onto the line. 
    // It falls where t = [(p-v) . (w-v)] / |w-v|^2
    const float t = dot(p - v, w - v) / l2;
    if (t < 0.0) return distance(p, v);       // Beyond the 'v' end of the segment
    else if (t > 1.0) return distance(p, w);  // Beyond the 'w' end of the segment
    const vec2 projection = v + t * (w - v);  // Projection falls on the segment
    return distance(p, projection);
}

int main() {
    // Initialize noise objects
    module::Perlin perlin;
    perlin.SetOctaveCount(6);
    perlin.SetFrequency(1.0);
    perlin.SetPersistence(0.3);

    utils::NoiseMap height_map;
    utils::NoiseMapBuilderPlane height_map_builder;
    height_map_builder.SetSourceModule(perlin);
    height_map_builder.SetDestNoiseMap(height_map);
    height_map_builder.SetDestSize(256, 256);
    height_map_builder.SetBounds(6.0, 10.0, 1.0, 5.0);
    height_map_builder.Build();

    // Do work

    /* random seed */
    srand((unsigned int)time(NULL));

    // Setup line
    vec2 start, end;
    start.x = 5;
    start.y = 100;
    end.x = 250;
    end.y = 100;
    vec2* points;
    int total_points = DEPTH * DEPTH;
    points = form_line(DEPTH, start, end);

    int x, y;
    for(x = 0; x < 512; x++) {
        for(y = 0; y < 512; y++) {
            vec2 current_point;
            current_point.x = x;
            current_point.y = y;
            float min_dist = 99999;

            // Find out how close this point is to a line
            int i;
            for(i = 0; i < total_points - 1; i++) {
                vec2 p0 = points[i];
                vec2 p1 = points[i+1];
                float dist = distance_to_line(p0, p1, current_point);
                if(dist < min_dist) {
                    min_dist = dist;
                }
            }

            if(min_dist < 40) {
                float previous_value = height_map.GetValue(x, y);
                float function_value = valley_function(min_dist / 20);
                printf("function value: %f\n", function_value);
                if(function_value < previous_value) {
                    height_map.SetValue(x, y, function_value);
                }
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

    // Now add color and make a second image
    renderer.ClearGradient ();
    renderer.AddGradientPoint (-1.0000, utils::Color (  0,   0, 128, 255)); // deeps
    renderer.AddGradientPoint (-0.2500, utils::Color (  0,   0, 255, 255)); // shallow
    renderer.AddGradientPoint ( 0.0000, utils::Color (  0, 128, 255, 255)); // shore
    renderer.AddGradientPoint ( 0.0625, utils::Color (240, 240,  64, 255)); // sand
    renderer.AddGradientPoint ( 0.1250, utils::Color ( 32, 160,   0, 255)); // grass
    renderer.AddGradientPoint ( 0.3750, utils::Color (224, 224,   0, 255)); // dirt
    renderer.AddGradientPoint ( 0.7500, utils::Color (128, 128, 128, 255)); // rock
    renderer.AddGradientPoint ( 1.0000, utils::Color (255, 255, 255, 255)); // snow
    renderer.EnableLight ();
    renderer.SetLightContrast (3.0); // Triple the contrast
    renderer.SetLightBrightness (2.0); // Double the brightness
    renderer.Render();

    writer.SetSourceImage(image);
    writer.SetDestFilename("color.bmp");
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
    //SDL_Delay(3000);
}

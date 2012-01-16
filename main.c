#include "SDL/SDL.h"
#include <stdlib.h> /* Required for random */
#include <time.h> /* For random seeding */

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#define W_WIDTH 640
#define W_HEIGHT 480

#define DEPTH 5

typedef struct _point {
    int x;
    int y;
} point;

point* perturb_point(point* points, int before, int after, int depth) {
    if(depth >= DEPTH) {
        return points;
    }
    printf("Perturbing point between %i and %i.\n", before, after);
    printf("Currently at depth %i.\n", depth);

    point* midpoint;
    midpoint = malloc(sizeof(point));
    midpoint->x = (points[before].x + points[after].x) / 2;
    midpoint->y = (points[before].y + points[after].y) / 2;

    int x_rand = rand() % ((200 / (depth + 1)) - (100 / (depth + 1)));
    int y_rand = rand() % ((200 / (depth + 1)) - (100 / (depth + 1)));
    printf("Random offsets are %i and %i.\n", x_rand, y_rand);
    midpoint->x += x_rand;
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
    points = malloc(sizeof(point) * total_points);
    points[0] = start;
    points[total_points - 1] = end;

    return perturb_point(points, 0, total_points - 1, 0);
}

main(int argc, char* argv[]) {
    if(SDL_Init(SDL_INIT_EVERYTHING) == -1) {
        printf("Failed\n");
        return;
    }

    /* random seed */
    srand((unsigned int)time(NULL));

    point start, end;
    start.x = 5;
    start.y = 5;
    end.x = 400;
    end.y = 400;
    point* points;
    int total_points = DEPTH * DEPTH;
    points = form_line(DEPTH, start, end);

    /* HERE BE OPENGL DRAGONS */

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
    SDL_Delay(2000);

    /* cleanup */

    free(points);
}

#include <iostream>
#include <fstream>
#include <math.h>
#include <vector>
#include <SDL2/SDL.h>
using namespace std;

#define PI (acos(-1))
#define W 640
#define H 480
SDL_Renderer * ren = NULL;

typedef struct _xy {
    float x;
    float y;
    short hsz = 4;
    _xy(float x=0, float y=0)
        : x(x), y(y) {}
    _xy operator+(const _xy& a) {
        return _xy(x+a.x, y+a.y);
    }
    _xy operator-(const _xy& a) {
        return _xy(x-a.x, y-a.y);
    }
    void round(int g) {
        x = roundf(x/(float)g)*g;
        y = roundf(y/(float)g)*g;
    }
    bool handleColl(const _xy pos) {
        return (pos.x < x+hsz && pos.x > x-hsz && pos.y < y+hsz && pos.y > y-hsz);
    }
    void drawHandle(const _xy cpos) {
        struct _xy p = *this-cpos;
        SDL_RenderDrawLine(ren, (W/2)+p.x+(hsz-1), (H/2)-p.y+(hsz-1), (W/2)+p.x+(hsz-1), (H/2)-p.y-(hsz-1));
        SDL_RenderDrawLine(ren, (W/2)+p.x+(hsz-1), (H/2)-p.y-(hsz-1), (W/2)+p.x-(hsz-1), (H/2)-p.y-(hsz-1));
        SDL_RenderDrawLine(ren, (W/2)+p.x-(hsz-1), (H/2)-p.y-(hsz-1), (W/2)+p.x-(hsz-1), (H/2)-p.y+(hsz-1));
        SDL_RenderDrawLine(ren, (W/2)+p.x-(hsz-1), (H/2)-p.y+(hsz-1), (W/2)+p.x+(hsz-1), (H/2)-p.y+(hsz-1));
    }
} xy;

struct _mouse {
    xy p, last;
    short buttonstate;
    bool drag = false;
    xy * grab = NULL;
    void update() {
        int tx, ty;
        buttonstate = SDL_GetMouseState(&tx, &ty);
        last = p;
        p.x = (float)tx-(W/2);
        p.y = (H/2)-(float)ty;
    }
    bool l () {return buttonstate & SDL_BUTTON_LMASK;}
} m;

struct _Map {
    vector <xy> points;
    void addPoint(float x, float y) {
        xy n(x, y);
        points.push_back(n);
    }
    xy getPoint(unsigned int i) {
        if (i < points.size()) return points[i];
        xy n(0, 0); return n;
    }
    xy * getPointPoint(unsigned int i) {
        if (i < points.size()) return &points[i];
        return NULL;
    }
} Map;

#undef main

int main () {
    //init
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * win = SDL_CreateWindow("Hello?", 160, 120, W, H, SDL_WINDOW_SHOWN);
    ren = SDL_CreateRenderer(win, -1, 0);

    int grid = 16;
    xy cam(0,0), mouse;
    Map.addPoint(0,0);
    Map.addPoint(40,40);
    Map.addPoint(40,40);

    bool quit = false;
    while (!quit) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(ren);

        SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);
        //draw grid
        if (grid != 1) {
            SDL_SetRenderDrawColor(ren, 48, 48, 48, SDL_ALPHA_OPAQUE);
            int j = ((W/2)-(int)cam.x)%grid;
            if (j < 0) j += grid;
            for (int i = 0; i < (W / grid); i++) {
                SDL_RenderDrawLine(ren, j, 0, j, H);
                j += grid;
            }
            j = ((H/2)+(int)cam.y)%grid;
            if (j < 0) j += grid;
            for (int i = 0; i < (H / grid); i++) {
                SDL_RenderDrawLine(ren, 0, j, W, j);
                j += grid;
            }
        }{
            SDL_SetRenderDrawColor(ren, 80, 80, 80, SDL_ALPHA_OPAQUE);
            int j = ((W/2)-(int)cam.x)%128;
            if (j < 0) j += 128;
            for (int i = 0; i < (W / 128); i++) {
                SDL_RenderDrawLine(ren, j, 0, j, H);
                j += 128;
            }
            j = ((H/2)+(int)cam.y)%128;
            if (j < 0) j += 128;
            for (int i = 0; i < (H / 128); i++) {
                SDL_RenderDrawLine(ren, 0, j, W, j);
                j += 128;
            }
        }
        SDL_SetRenderDrawColor(ren, 48, 128, 128, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(ren, (W/2)-(int)cam.x, 0, (W/2)-(int)cam.x, H);
        SDL_RenderDrawLine(ren, 0, (H/2)+(int)cam.y, W, (H/2)+(int)cam.y);
        //draw points
        SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);
        for (int i = 0; i < Map.points.size(); i++) {
            Map.getPoint(i).drawHandle(cam);
        }

        //keyboard
        m.update();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.key.keysym.sym) {
                case SDLK_RIGHTBRACKET: if (e.type == SDL_KEYDOWN) grid *= 2; break;
                case SDLK_LEFTBRACKET: if (e.type == SDL_KEYDOWN) if (grid > 1) grid /= 2; break;
                case SDLK_RETURN:
                case SDLK_ESCAPE: quit = (e.type == SDL_KEYDOWN); break;
                default: quit = (e.type == SDL_QUIT);
            }
        }

        //movement
        if (m.l()) {
            if (m.drag) {
                if (m.grab) {
                    if (m.grab == &cam) cam = cam - (m.p - m.last);
                    else {
                        *m.grab = m.p + cam;
                        m.grab->round(grid);
                    }
                }
            } else {
                m.grab = &cam;
                m.drag = true;
                for (int i = 0; i < Map.points.size(); i++) {
                    if (Map.getPoint(i).handleColl(m.p+cam)) {
                        m.grab = Map.getPointPoint(i);
                        break;
                    }
                }
            }
        } else m.drag = m.l();

        SDL_RenderPresent(ren);
        SDL_Delay(1);
    }
    return 0;
}

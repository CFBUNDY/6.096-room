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

#undef main

int main () {
    //init
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * win = SDL_CreateWindow("Hello?", 160, 120, W, H, SDL_WINDOW_SHOWN);
    ren = SDL_CreateRenderer(win, -1, 0);

    xy cam(0,0), point(0,0), mouse;

    bool quit = false;
    while (!quit) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(ren);

        SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);
        if (point.handleColl(m.p+cam)) {
            if (m.l()) SDL_SetRenderDrawColor(ren, 255, 255, 0, SDL_ALPHA_OPAQUE);
            else SDL_SetRenderDrawColor(ren, 255, 0, 0, SDL_ALPHA_OPAQUE);
        }
        point.drawHandle(cam);

        //keyboard
        m.update();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.key.keysym.sym) {
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
                    else *m.grab = *m.grab + (m.p - m.last);
                }
            } else {
                if (point.handleColl(m.p+cam)) m.grab = &point;
                else m.grab = &cam;
                m.drag = true;
            }
        } else m.drag = m.l();

        SDL_RenderPresent(ren);
        SDL_Delay(1);
    }
    return 0;
}

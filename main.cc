#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
using namespace std;

#define W 640
#define H 480
SDL_Renderer * ren = NULL;

typedef struct _xy {
    float x;
    float y;
} xy;

typedef struct _camera {
    xy p;
    float dir;
} camera;

class Player {
    camera pos;
    public:
    Player(float x, float y, float dir) {
        pos.p.x = x;
        pos.p.y = y;
        pos.dir = dir;
    }
    camera getpos() {
        return pos;
    }
    void movement(float frwd, float side, float turn) {
        pos.dir += turn;
        pos.p.x += side;
        pos.p.y += frwd;
    }
};

class Cell {
    vector<xy> points;
    public:
    void addpoint(float x, float y){
        xy n;
        n.x = x;
        n.y = y;
        points.push_back(n);
    }
    void drawfrom(camera c){
        vector<xy>::iterator it = points.begin();
        xy last = *it;
        for (it++; it != points.end(); it++) {
            SDL_RenderDrawLine(ren, last.x-c.p.x+(W/2), last.y-c.p.y+(H/2), it->x-c.p.x+(W/2), it->y-c.p.y+(H/2));
            last = *it;
        }
        it = points.begin();
        SDL_RenderDrawLine(ren, last.x-c.p.x+(W/2), last.y-c.p.y+(H/2), it->x-c.p.x+(W/2), it->y-c.p.y+(H/2));
    }
};

int main () {
    Player you(0, 0, 0);
    Cell singlecell;
    singlecell.addpoint(-60, -60);
    singlecell.addpoint(-80, 20);
    singlecell.addpoint(80, 40);
    bool keys[6] = {0,0,0,0,0,0};

    //init
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * win = SDL_CreateWindow("Hello?", 160, 120, W, H, SDL_WINDOW_SHOWN);
    ren = SDL_CreateRenderer(win, -1, 0);
    bool quit = false;
    while (!quit) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(ren);

        SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(ren, (W/2)+5, (H/2)-5, (W/2)-5, (H/2)+5);
        SDL_RenderDrawLine(ren, (W/2)-5, (H/2)-5, (W/2)+5, (H/2)+5);
        singlecell.drawfrom(you.getpos());

        SDL_RenderPresent(ren);

        //keyboard
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_KEYUP:
                case SDL_KEYDOWN:
                    switch (e.key.keysym.sym) {
                        case SDLK_w: keys[0] = (e.type == SDL_KEYDOWN); break;
                        case SDLK_s: keys[1] = (e.type == SDL_KEYDOWN); break;
                        case SDLK_a: keys[2] = (e.type == SDL_KEYDOWN); break;
                        case SDLK_d: keys[3] = (e.type == SDL_KEYDOWN); break;
                        case SDLK_q: keys[4] = (e.type == SDL_KEYDOWN); break;
                        case SDLK_e: keys[5] = (e.type == SDL_KEYDOWN); break;
                    }
                    quit = (e.key.keysym.sym == SDLK_RETURN);
                    break;
                default: break;
            }
        }
        you.movement((keys[1]-keys[0])*0.1, (keys[3]-keys[2])*0.1, 0);
        SDL_Delay(1);
    }
    return 0;
}

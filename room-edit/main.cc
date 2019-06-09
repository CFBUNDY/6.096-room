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

#undef main

int main () {
    //init
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * win = SDL_CreateWindow("Hello?", 160, 120, W, H, SDL_WINDOW_SHOWN);
    ren = SDL_CreateRenderer(win, -1, 0);
    bool quit = false;
    while (!quit) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(ren);

        SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);

        //keyboard
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if ( e.key.keysym.sym == SDLK_RETURN ||
                 e.key.keysym.sym == SDLK_ESCAPE ) quit = (e.type == SDL_KEYDOWN);
            else quit = (e.type == SDL_QUIT);
        }

        SDL_RenderPresent(ren);
        SDL_Delay(1);
    }
    return 0;
}

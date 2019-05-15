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
    _xy(float x=0, float y=0)
        : x(x), y(y) {}
    _xy operator+(const _xy& a) {
        return _xy(x+a.x, y+a.y);
    }
    _xy operator-(const _xy& a) {
        return _xy(x-a.x, y-a.y);
    }
} xy;

float pointDistance (xy p1, xy p2) {
    return sqrt(pow((p1.x-p2.x),2.0)+pow((p1.y-p2.y),2.0));
}

float dotProduct (xy a, xy b) {
    return a.x*b.x+a.y*b.y;
}

// >0 : clockwise
// =0 : co-linear
// <0 : counter-clockwise
float pointOrientation (xy a, xy b, xy c) {
    return (b.x - a.x)*(c.y - b.y) - (c.x - b.x)*(b.y - a.y);
}

typedef struct _line {
    xy p1;
    xy p2;
    _line(xy p1, xy p2)
        : p1(p1), p2(p2) {}
    void draw() {
        SDL_RenderDrawLine(ren, (W/2)+p1.x, (H/2)-p1.y, (W/2)+p2.x, (H/2)-p2.y);
    }
    float length() {
        return pointDistance(p1, p2);
    }
} line;

typedef struct _portal {
    int target;
    float da;
    xy dxy;
    _portal (int target, float dx, float dy, float da)
    : target(target), dxy(dx,dy), da(da) {}
} portal;

typedef struct _camera {
    xy p;
    float dir;
    _camera operator+(const portal& a) {
        _camera nc;
        nc.p = p+a.dxy;
        nc.dir = dir+a.da;
        return nc;
    }
} camera;

xy pointFrom (camera view, xy point) {
    xy proj = point - view.p;
    float tx = proj.x*cos(view.dir) - proj.y*sin(view.dir);
    proj.y = proj.x*sin(view.dir) + proj.y*cos(view.dir);
    proj.x = tx;
    return proj;
} //returns where point appears from view

xy closestPointOnSegment (line l, xy p) {
    float length = l.length();
    float scalar = dotProduct((p-l.p1), (l.p2-l.p1))/length;
    if (scalar < 0) {
        return l.p1;
    } else if (scalar > length) {
        return l.p2;
    } else {
        xy closest = l.p2 - l.p1;
        closest.x *= scalar/length;
        closest.y *= scalar/length;
        closest = closest + l.p1;
    }
}

class Cell {
    vector<int> points;
    vector<portal> portals;
    //points added in clockwise rotation
    public:
    void addpoint(int mappoint, int c = -1, float dx = 0, float dy = 0, float da = 0){
        points.push_back(mappoint);
        portal p(c, dx, dy, da);
        portals.push_back(p);
    }
    int cellSize () {return points.size();}
    line getSegment (int i);
    portal getPortal (int i) {
        return portals[i%portals.size()];
    }
};

//vector <Cell> cMap;
struct _Map {
    vector <xy> points;
    vector <Cell> c;
    void addPoint(float x, float y) {
        xy n(x, y);
        points.push_back(n);
    }
    xy getPoint(unsigned int i) {
        if (i < points.size()) return points[i];
        xy n(0, 0); return n;
    }
} Map;

line Cell::getSegment (int i) {
    line l(Map.getPoint(points[i%points.size()]), Map.getPoint(points[(i+1)%points.size()]));
    return l;
}

class Player {
    camera pos;
    float movespeed = 0.1, turnspeed = 0.005, width = 4;
    int curCell;
    public:
    Player(float x = 0, float y = 0, float dir = 0, int current = 0) {
        pos.p.x = x;
        pos.p.y = y;
        pos.dir = dir;
        curCell = current;
    }
    camera getpos() {
        return pos;
    }
    int getcell() {
        return curCell;
    }
    void movement(float frwd, float side, float turn) {
        //change position
        pos.dir += turn*turnspeed;
        pos.p.x += (frwd*sin(pos.dir)+side*cos(pos.dir))*movespeed;
        pos.p.y += (frwd*cos(pos.dir)-side*sin(pos.dir))*movespeed;
        //collision
        for (int i = 0; i < Map.c[curCell].cellSize(); i++) {
            line l = Map.c[curCell].getSegment(i);
            portal p = Map.c[curCell].getPortal(i);
            if (p.target != -1) { // check if changing current cell
                if (pointOrientation(l.p1, l.p2, pos.p) > 0) {
                    curCell = p.target;
                    pos.p = pos.p + p.dxy;
                    pos.dir += p.da;
                    break;
                }
            } else {
                xy cl = closestPointOnSegment(l, pos.p);
                if (pointOrientation(l.p1, l.p2, pos.p) > 0) {
                    xy n = cl-pos.p;
                    float nl = pointDistance(cl, pos.p);
                    nl = (nl+width)/nl;
                    n.x = n.x*nl;
                    n.y = n.y*nl;
                    pos.p = pos.p+n;
                } else if (pointDistance(cl, pos.p) < width) {
                    xy n = cl-pos.p;
                    float nl = pointDistance(cl, pos.p);
                    nl = width/nl;
                    n.x = n.x*nl;
                    n.y = n.y*nl;
                    pos.p = cl-n;
                }
            }
        }
    }
};

void drawroom(camera c, int cel) { // move this out of player functions to use different cameras
    for (int j = 0; j < Map.c[cel].cellSize(); j++) {
        line l = Map.c[cel].getSegment(j);
        portal p = Map.c[cel].getPortal(j);
        if (pointOrientation(l.p1, l.p2, c.p) < 0) {
            if (p.target != -1) {
                drawroom(c+p, p.target);
            } else {
                l.p1 = pointFrom(c, l.p1);
                l.p2 = pointFrom(c, l.p2);
                l.draw();
            }
        }
    }
}

#undef main
//if this becomes a problem, rename 'main' to 'WinMain' for windows version

int main () {
    Map.addPoint(-64, -32);
    Map.addPoint(-64, 32);
    Map.addPoint(64, 64);
    Map.addPoint(64, -64);
    Map.addPoint(-144, -64);
    Map.addPoint(-144, 64);
    Map.addPoint(-80, 32);
    Map.addPoint(-80, -32);
    Cell singlecell;//, secondcell;
    Cell * secondcell = new Cell;
    singlecell.addpoint(0, 1, -16);
    singlecell.addpoint(1);
    singlecell.addpoint(2);
    singlecell.addpoint(3);
    //secondcell->addpoint(1, 0);
    //secondcell->addpoint(0);
    secondcell->addpoint(4);
    secondcell->addpoint(5);
    secondcell->addpoint(6, 0, 16);
    secondcell->addpoint(7);
    Map.c.push_back(singlecell);
    Map.c.push_back(*secondcell);
    delete secondcell;
    Player you;
    bool keys[128] = {0};
    enum mode_ {play, edit, cube} mode = play;
    int frame = 0;

    //init
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * win = SDL_CreateWindow("Hello?", 160, 120, W, H, SDL_WINDOW_SHOWN);
    ren = SDL_CreateRenderer(win, -1, 0);
    bool quit = false;
    while (!quit) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(ren);

        SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);
        switch (mode) {
            case play:
            SDL_RenderDrawLine(ren, (W/2)+4, (H/2), (W/2)-4, (H/2));
            SDL_RenderDrawLine(ren, (W/2), (H/2)-4, (W/2), (H/2)+4);
            drawroom(you.getpos(), you.getcell());
            break;
            case edit:
            for (int i = 0; i < Map.points.size(); i++) {
                xy p = Map.getPoint(i);
                SDL_RenderDrawLine(ren, (W/2)+p.x+2, (H/2)-p.y+2, (W/2)+p.x-2, (H/2)-p.y+2);
                SDL_RenderDrawLine(ren, (W/2)+p.x-2, (H/2)-p.y+2, (W/2)+p.x-2, (H/2)-p.y-2);
                SDL_RenderDrawLine(ren, (W/2)+p.x-2, (H/2)-p.y-2, (W/2)+p.x+2, (H/2)-p.y-2);
                SDL_RenderDrawLine(ren, (W/2)+p.x+2, (H/2)-p.y-2, (W/2)+p.x+2, (H/2)-p.y+2);
            }
            for (int i = 0; i < Map.c.size(); i++) {
                for (int j = 0; j < Map.c[i].cellSize(); j++) {
                    line l = Map.c[i].getSegment(j);
                    SDL_RenderDrawLine(ren, (W/2)+l.p1.x, (H/2)-l.p1.y, (W/2)+l.p2.x, (H/2)-l.p2.y);
                }
            }
            break;
            case cube: {
            int FL = 512;
            float pch = double(frame)/512;
            float yaw = double(frame)/768;
            float rol = double(frame)/1024;
            float cube [8][3] = {{32, 32, 32}, {-32, 32, 32}, {32, -32, 32}, {-32, -32, 32}, {32, 32, -32}, {-32, 32, -32}, {32, -32, -32}, {-32, -32, -32}};
            for (int i = 0; i < 8; i++) {
                float t = cube[i][0]*cos(yaw)-cube[i][2]*sin(yaw);
                cube[i][2] = cube[i][0]*sin(yaw)+cube[i][2]*cos(yaw);
                cube[i][0] = t;
            }
            for (int i = 0; i < 8; i++) {
                float t = cube[i][1]*cos(pch)-cube[i][2]*sin(pch);
                cube[i][2] = cube[i][1]*sin(pch)+cube[i][2]*cos(pch);
                cube[i][1] = t;
            }
            for (int i = 0; i < 8; i++) {
                float t = cube[i][0]*cos(rol)-cube[i][1]*sin(rol);
                cube[i][1] = cube[i][0]*sin(rol)+cube[i][1]*cos(rol);
                cube[i][0] = t;
            }
            for (int i = 0; i < 8; i++) {
                cube[i][2] = cube[i][2]-512;
            }
            SDL_RenderDrawLine(ren, (W/2)+(FL/cube[0][2])*cube[0][0], (H/2)-(FL/cube[0][2])*cube[0][1], (W/2)+(FL/cube[1][2])*cube[1][0], (H/2)-(FL/cube[1][2])*cube[1][1]);
            SDL_RenderDrawLine(ren, (W/2)+(FL/cube[0][2])*cube[0][0], (H/2)-(FL/cube[0][2])*cube[0][1], (W/2)+(FL/cube[2][2])*cube[2][0], (H/2)-(FL/cube[2][2])*cube[2][1]);
            SDL_RenderDrawLine(ren, (W/2)+(FL/cube[0][2])*cube[0][0], (H/2)-(FL/cube[0][2])*cube[0][1], (W/2)+(FL/cube[4][2])*cube[4][0], (H/2)-(FL/cube[4][2])*cube[4][1]);
            SDL_RenderDrawLine(ren, (W/2)+(FL/cube[3][2])*cube[3][0], (H/2)-(FL/cube[3][2])*cube[3][1], (W/2)+(FL/cube[1][2])*cube[1][0], (H/2)-(FL/cube[1][2])*cube[1][1]);
            SDL_RenderDrawLine(ren, (W/2)+(FL/cube[3][2])*cube[3][0], (H/2)-(FL/cube[3][2])*cube[3][1], (W/2)+(FL/cube[2][2])*cube[2][0], (H/2)-(FL/cube[2][2])*cube[2][1]);
            SDL_RenderDrawLine(ren, (W/2)+(FL/cube[3][2])*cube[3][0], (H/2)-(FL/cube[3][2])*cube[3][1], (W/2)+(FL/cube[7][2])*cube[7][0], (H/2)-(FL/cube[7][2])*cube[7][1]);
            SDL_RenderDrawLine(ren, (W/2)+(FL/cube[5][2])*cube[5][0], (H/2)-(FL/cube[5][2])*cube[5][1], (W/2)+(FL/cube[1][2])*cube[1][0], (H/2)-(FL/cube[1][2])*cube[1][1]);
            SDL_RenderDrawLine(ren, (W/2)+(FL/cube[5][2])*cube[5][0], (H/2)-(FL/cube[5][2])*cube[5][1], (W/2)+(FL/cube[4][2])*cube[4][0], (H/2)-(FL/cube[4][2])*cube[4][1]);
            SDL_RenderDrawLine(ren, (W/2)+(FL/cube[5][2])*cube[5][0], (H/2)-(FL/cube[5][2])*cube[5][1], (W/2)+(FL/cube[7][2])*cube[7][0], (H/2)-(FL/cube[7][2])*cube[7][1]);
            SDL_RenderDrawLine(ren, (W/2)+(FL/cube[6][2])*cube[6][0], (H/2)-(FL/cube[6][2])*cube[6][1], (W/2)+(FL/cube[2][2])*cube[2][0], (H/2)-(FL/cube[2][2])*cube[2][1]);
            SDL_RenderDrawLine(ren, (W/2)+(FL/cube[6][2])*cube[6][0], (H/2)-(FL/cube[6][2])*cube[6][1], (W/2)+(FL/cube[4][2])*cube[4][0], (H/2)-(FL/cube[4][2])*cube[4][1]);
            SDL_RenderDrawLine(ren, (W/2)+(FL/cube[6][2])*cube[6][0], (H/2)-(FL/cube[6][2])*cube[6][1], (W/2)+(FL/cube[7][2])*cube[7][0], (H/2)-(FL/cube[7][2])*cube[7][1]);
            break;
            }
            default: break;
        }

        //SDL_RenderPresent(ren);

        //keyboard
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_KEYUP:
                case SDL_KEYDOWN:
                    for (int i = '0'; i < '{'; i++) {
                        if (i == ':') i = 'a';
                        if (e.key.keysym.sym == i) keys[i] = (e.type == SDL_KEYDOWN);
                    }
                    switch (e.key.keysym.sym) {
                        case SDLK_RETURN:
                        case SDLK_ESCAPE: quit = (e.type == SDL_KEYDOWN); break;
                    }; break;
                default: quit = (e.type == SDL_QUIT); break;
            }
        }
        if (keys['z']) mode = play;
        if (keys['x']) mode = edit;
        if (keys['c']) mode = cube;
        switch (mode) {
            case play: you.movement(keys['w']-keys['s'], keys['d']-keys['a'], keys['e']-keys['q']); break;
            default: break;
        }
        SDL_RenderPresent(ren);
        SDL_Delay(1);
        frame++;
    }
    return 0;
}

#include <iostream>
#include <fstream>
#include <math.h>
#include <vector>
#include <chrono>
//chrono requires -std=c++11
#include <SDL2/SDL.h>
using namespace std;

#define PI (acos(-1))
#define W 640
#define H 480
SDL_Renderer * ren = NULL;
int mcs = 0;

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

// <0 : clockwise
// =0 : co-linear
// >0 : counter-clockwise
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
    bool clipByLine(const _line clip);
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
    _camera displace (const xy& point, const portal& a) {
        _camera nc;
        nc.p = p - point;
        float tx = nc.p.x*cos(-a.da) - nc.p.y*sin(-a.da);
        nc.p.y = nc.p.x*sin(-a.da) + nc.p.y*cos(-a.da);
        nc.p.x = tx;
        nc.p = nc.p + point + a.dxy;
        nc.dir = dir+a.da;
        return nc;
    }
} camera;

xy lineIntersect (line l1, line l2) {
    float dnm = (l1.p1.x-l1.p2.x)*(l2.p1.y-l2.p2.y)-(l1.p1.y-l1.p2.y)*(l2.p1.x-l2.p2.x);
    xy res(0,0);
    if (dnm == 0) return res;
    res.x = ((((l1.p1.x*l1.p2.y)-(l1.p2.x*l1.p1.y))*(l2.p1.x-l2.p2.x))-((l1.p1.x-l1.p2.x)*((l2.p1.x*l2.p2.y)-(l2.p2.x*l2.p1.y))))/dnm;
    res.y = ((((l1.p1.x*l1.p2.y)-(l1.p2.x*l1.p1.y))*(l2.p1.y-l2.p2.y))-((l1.p1.y-l1.p2.y)*((l2.p1.x*l2.p2.y)-(l2.p2.x*l2.p1.y))))/dnm;
    return res;
}

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

bool line::clipByLine (const line clip) {
    if ((pointOrientation(clip.p1, clip.p2, p1) > 0) && (pointOrientation(clip.p1, clip.p2, p2) > 0)) return false;
    if (pointOrientation(clip.p1, clip.p2, p1) > 0) {
        p1 = lineIntersect(*this, clip);
    }
    if (pointOrientation(clip.p1, clip.p2, p2) > 0) {
        p2 = lineIntersect(*this, clip);
    }
    return true;
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
    float movespeed = 100, turnspeed = PI, width = 4;
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
        pos.dir += turn*(float(mcs)/1000000)*turnspeed;
        pos.p.x += (frwd*sin(pos.dir)+side*cos(pos.dir))*(float(mcs)/1000000)*movespeed;
        pos.p.y += (frwd*cos(pos.dir)-side*sin(pos.dir))*(float(mcs)/1000000)*movespeed;
        //collision
        for (int i = 0; i < Map.c[curCell].cellSize(); i++) {
            line l = Map.c[curCell].getSegment(i);
            portal p = Map.c[curCell].getPortal(i);
            if (p.target != -1) { // check if changing current cell
                if (pointOrientation(l.p1, l.p2, pos.p) > 0) {
                    curCell = p.target;
                    pos = pos.displace(l.p1, p);
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

void drawroom(camera c, int cel, line * view = NULL) {
    for (int j = 0; j < Map.c[cel].cellSize(); j++) {
        line l = Map.c[cel].getSegment(j);
        portal p = Map.c[cel].getPortal(j);
        if (pointOrientation(l.p1, l.p2, c.p) < 0) {
            if (view) {
            /*SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
            line help(view->p1, view->p2);
            help.p1 = pointFrom(c, help.p1);
            help.p2 = pointFrom(c, help.p2);
            help.draw();
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);*/
                {line clip(c.p, view->p1);
                if (not l.clipByLine(clip)) goto dont;}
                {line clip(view->p2, c.p);
                if (not l.clipByLine(clip)) goto dont;}
            }
            if (p.target != -1) {
                l.p1 = l.p1+p.dxy;
                l.p2 = l.p2+p.dxy-l.p1;
                float tx = l.p2.x*cos(-p.da) - l.p2.y*sin(-p.da);
                l.p2.y = l.p2.x*sin(-p.da) + l.p2.y*cos(-p.da);
                l.p2.x = tx;
                l.p2 = l.p2+l.p1;
                drawroom(c.displace(Map.c[cel].getSegment(j).p1, p), p.target, &l);
            } else {
                l.p1 = pointFrom(c, l.p1);
                l.p2 = pointFrom(c, l.p2);
                l.draw();
            }
        }
        dont: 0; //there's probably a better way to exit that, but for now,
    }
}

#undef main
//if this becomes a problem, rename 'main' to 'WinMain' for windows version

int main () {
    Map.addPoint(-40, 140);
    Map.addPoint(20, 140);
    Map.addPoint(-40, 100);
    Map.addPoint(-20, 80);
    Map.addPoint(20, 80);
    Map.addPoint(-100, 40);
    Map.addPoint(-60, 40);
    Map.addPoint(-20, 40);
    Map.addPoint(20, 40);
    Map.addPoint(-40, 20);
    Map.addPoint(-100, 0);
    Map.addPoint(-60, -40);
    Map.addPoint(-40, -40);
    Map.addPoint(20, -40);
    Cell * newcell = new Cell;
    newcell->addpoint(7, 1, 0, 40);
    newcell->addpoint(8);
    newcell->addpoint(13);
    newcell->addpoint(12, 2);
    newcell->addpoint(9);
    Map.c.push_back(*newcell);
    delete newcell;
    newcell = new Cell;
    newcell->addpoint(0);
    newcell->addpoint(1);
    newcell->addpoint(4, 0, 0, -40);
    newcell->addpoint(3);
    newcell->addpoint(2, 2, -20, -60, -(PI/2));
    Map.c.push_back(*newcell);
    delete newcell;
    newcell = new Cell;
    newcell->addpoint(5, 1, 60, 100, (PI/2));
    newcell->addpoint(6);
    newcell->addpoint(9, 0);
    newcell->addpoint(12);
    newcell->addpoint(11);
    newcell->addpoint(10);
    Map.c.push_back(*newcell);
    delete newcell;
    Player you;
    xy ecam;
    bool keys[128] = {0};
    enum mode_ {play, edit, cube} mode = play;
    float timer = 0;
    
    //init
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * win = SDL_CreateWindow("Hello?", 160, 120, W, H, SDL_WINDOW_SHOWN);
    ren = SDL_CreateRenderer(win, -1, 0);
    auto lastclock = chrono::steady_clock::now();
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
            for (int i = 0; i < Map.points.size(); i++) { //points
                xy p = Map.getPoint(i);
                p = p - ecam;
                SDL_RenderDrawLine(ren, (W/2)+p.x+2, (H/2)-p.y+2, (W/2)+p.x-2, (H/2)-p.y+2);
                SDL_RenderDrawLine(ren, (W/2)+p.x-2, (H/2)-p.y+2, (W/2)+p.x-2, (H/2)-p.y-2);
                SDL_RenderDrawLine(ren, (W/2)+p.x-2, (H/2)-p.y-2, (W/2)+p.x+2, (H/2)-p.y-2);
                SDL_RenderDrawLine(ren, (W/2)+p.x+2, (H/2)-p.y-2, (W/2)+p.x+2, (H/2)-p.y+2);
            }
            for (int i = 0; i < Map.c.size(); i++) { //walls
                for (int j = 0; j < Map.c[i].cellSize(); j++) {
                    line l = Map.c[i].getSegment(j);
                    l.p1 = l.p1 - ecam;
                    l.p2 = l.p2 - ecam;
                    if (Map.c[i].getPortal(j).target != -1) {
                        SDL_SetRenderDrawColor(ren, 0, 0, 255, SDL_ALPHA_OPAQUE);
                        l.draw();
                        l.p2 = l.p1 + Map.c[i].getPortal(j).dxy;
                        SDL_SetRenderDrawColor(ren, 0, 255, 255, SDL_ALPHA_OPAQUE);
                        l.draw();
                    } else {
                        l.draw();
                    }
                    SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);
                }
            }
            { //player
                xy p (8*sin(you.getpos().dir),8*cos(you.getpos().dir));
                line l (you.getpos().p, p); l.p2 = l.p2+l.p1;
                l.p1 = l.p1 - ecam;
                l.p2 = l.p2 - ecam;
                SDL_SetRenderDrawColor(ren, 0, 255, 0, SDL_ALPHA_OPAQUE);
                SDL_RenderDrawLine(ren, (W/2)+l.p1.x+1, (H/2)-l.p1.y+1, (W/2)+l.p1.x-1, (H/2)-l.p1.y-1);
                SDL_RenderDrawLine(ren, (W/2)+l.p1.x-1, (H/2)-l.p1.y+1, (W/2)+l.p1.x+1, (H/2)-l.p1.y-1);
                l.draw();
                SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);
            }
            break;
            case cube: {
            int FL = 512;
            float pch = double(timer)*2;
            float yaw = double(timer)*1.5;
            float rol = double(timer);
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
        mcs = chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - lastclock).count();
        lastclock = chrono::steady_clock::now();

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
        if (keys['x']) {
            ecam = you.getpos().p;
            mode = edit;
        }
        if (keys['c']) mode = cube;
        switch (mode) {
            case play: you.movement(keys['w']-keys['s'], keys['d']-keys['a'], keys['e']-keys['q']); break;
            case edit: {
                xy cmov((float)mcs*(keys['d']-keys['a'])/10000, (float)mcs*(keys['w']-keys['s'])/10000);
                ecam = ecam + cmov;
                break;
            }
            default: break;
        }

        SDL_RenderPresent(ren);
        //SDL_Delay(1);
        timer += (float)mcs/1000000;
    }
    return 0;
}

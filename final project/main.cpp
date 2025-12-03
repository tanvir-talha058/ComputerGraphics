// city_after_rain_refined.cpp
// City After Rain — Refined Cinematic Edition
// Features: directional lighting, day-night cycle, improved rain physics, blurred reflections,
// smarter traffic & pedestrian logic, simplified bloom, camera timeline.
// Compile: g++ city_after_rain_refined.cpp -o city_after_rain_refined -lGL -lGLU -lglut -std=c++11

#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iostream>

constexpr double PI = 3.14159265358979323846;

// ---------------------- TUNABLE PARAMETERS ----------------------
int WIN_W = 1280;
int WIN_H = 780;
const int FRAME_MS = 16;
float TIME_SCALE = 1.0f;            // speed of simulated time
int RAIN_PARTICLES = 900;           // drop count (reduce if slow)
int MAX_SPLASHES = 160;
bool ENABLE_CINEMATIC = true;
bool ENABLE_BLOOM = true;
bool ENABLE_GRAIN = true;
// ----------------------------------------------------------------

float simTime = 0.0f; // seconds
bool raining = true;
bool autoCamera = true;
bool cinematic = true;
bool dayMode = false; // toggled with 'd'

// Simple directional light (sun/moon)
struct Sun {
    float angle; // overhead rotation 0..2pi
} sun{0.9f};

const int GROUND_Y = 140;

// ----- low-level draw primitives (DDA, midpoint) -----
void putPixel(int x, int y) { glVertex2i(x, y); }

void drawLineDDA(int x1,int y1,int x2,int y2){
    int dx = x2-x1, dy=y2-y1;
    int steps = std::max(abs(dx), abs(dy));
    if(steps==0){ glBegin(GL_POINTS); putPixel(x1,y1); glEnd(); return; }
    float x=x1,y=y1, xi=dx/(float)steps, yi=dy/(float)steps;
    glBegin(GL_POINTS);
    for(int i=0;i<=steps;i++){ putPixel((int)(x+0.5f),(int)(y+0.5f)); x+=xi; y+=yi; }
    glEnd();
}

void drawCircleMidpoint(int cx,int cy,int r){
    int x=0,y=r; int d=1-r;
    glBegin(GL_POINTS);
    auto plot8=[&](int px,int py){
        putPixel(cx+px, cy+py); putPixel(cx-px, cy+py);
        putPixel(cx+px, cy-py); putPixel(cx-px, cy-py);
        putPixel(cx+py, cy+px); putPixel(cx-py, cy+px);
        putPixel(cx+py, cy-px); putPixel(cx-py, cy-px);
    };
    while(x<=y){
        plot8(x,y);
        if(d<0) d+=2*x+3; else { d+=2*(x-y)+5; y--; }
        x++;
    }
    glEnd();
}

void drawFilledCircle(int cx,int cy,int r){
    for(int dy=-r;dy<=r;++dy){
        int dx = (int)floor(sqrt((double)r*r - dy*dy));
        glBegin(GL_POINTS);
        for(int x=-dx;x<=dx;++x) putPixel(cx+x, cy+dy);
        glEnd();
    }
}

void drawFilledRect(int x,int y,int w,int h){
    glBegin(GL_POINTS);
    for(int yy=y; yy<y+h; ++yy) for(int xx=x; xx<x+w; ++xx) putPixel(xx,yy);
    glEnd();
}

void drawRectAlpha(int x,int y,int w,int h, float r,float g,float b,float a){
    glColor4f(r,g,b,a);
    glBegin(GL_QUADS);
    glVertex2i(x,y); glVertex2i(x+w,y); glVertex2i(x+w,y+h); glVertex2i(x,y+h);
    glEnd();
}

// utility: clamp
float clampf(float v,float lo,float hi){ return v<lo?lo:(v>hi?hi:v); }

// ------------------ Scene objects ------------------
struct Building {
    int x,y,w,h;
    float baseR, baseG, baseB;
    bool brightWindows;
    int roofType;
};
std::vector<Building> buildings;

void buildCity(){
    buildings.clear();
    int x = 0;
    while(x < WIN_W*2){
        int w = 70 + (rand()%140);
        int h = 160 + (rand()%320);
        if(x + w > WIN_W*2) w = WIN_W*2 - x;
        Building b; b.x=x; b.y=GROUND_Y; b.w=w; b.h=h;
        b.baseR = 0.12f + (rand()%6)*0.06f;
        b.baseG = 0.12f + (rand()%5)*0.05f;
        b.baseB = 0.16f + (rand()%6)*0.04f;
        b.brightWindows = (rand()%3==0);
        b.roofType = rand()%3;
        buildings.push_back(b);
        x += w + 12;
    }
}

// shading helper: simulate simple lambert diffuse using sun direction
void applyDirectionalTint(float &r,float &g,float &b, float nx, float ny, float nz){
    // sun direction from sun.angle (y axis range)
    float sx = cosf(sun.angle);
    float sy = sinf(sun.angle);
    // normal dot light (assume light comes from above and slightly tilt)
    float dot = clampf(nx*sx + ny*sy + nz*0.6f, 0.0f, 1.0f);
    // when night, lower base and use bluish ambient
    float amb = dayMode ? 0.25f : 0.08f;
    float diffuse = amb + (0.75f * dot);
    // apply tint
    r *= diffuse; g *= diffuse; b *= diffuse;
    // color grade slight teal shadows / warm highlights (simple)
    if(!dayMode){
        r *= 0.95f; g *= 1.05f; b *= 1.12f;
    }
}

// draw building with simple shading + window lights at night
void drawBuilding(const Building &b, bool mirrored=false, float alpha=1.0f){
    if(mirrored){
        glColor4f(b.baseR*0.5f, b.baseG*0.5f, b.baseB*0.5f, alpha*0.35f);
        drawFilledRect(b.x, GROUND_Y - b.h, b.w, b.h);
        return;
    }
    // compute approximate normal tilt for building front (face normal = (0,0,1))
    float r = b.baseR, g = b.baseG, bl = b.baseB;
    applyDirectionalTint(r,g,bl, 0.0f, 0.0f, 1.0f);
    glColor3f(r,g,bl);
    drawFilledRect(b.x, b.y, b.w, b.h);

    // windows
    glBegin(GL_POINTS);
    for(int wy=12; wy < b.h; wy += 22){
        for(int wx=10; wx < b.w; wx += 18){
            bool lit = b.brightWindows && (rand()%9==0);
            if(!dayMode) lit = lit || (rand()%18==0); // more lights at night
            float wr = lit ? 1.0f : 0.45f;
            float wg = lit ? 0.95f : 0.45f;
            float wb = lit ? 0.7f : 0.35f;
            glColor3f(wr, wg, wb);
            putPixel(b.x + wx, b.y + wy);
        }
    }
    glEnd();
}

// ------------------ Clouds ------------------
struct Cloud { float x,y; float speed; int size; float depth; };
std::vector<Cloud> clouds;
void initClouds(){
    clouds.clear();
    for(int i=0;i<14;i++){
        Cloud c; c.x = rand()%(WIN_W*2); c.y = WIN_H - 120 - (rand()%220);
        c.speed = 0.06f + (rand()%12)*0.02f; c.size = 50 + (rand()%80); c.depth = 0.2f + (rand()%80)/100.0f;
        clouds.push_back(c);
    }
}
void drawCloud(const Cloud &c){
    float base = 0.6f * c.depth;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for(int k=0;k<5;k++){
        float ox = (k-2)*(c.size*0.18f);
        float oy = (k%2==0?6.0f:-6.0f);
        int r = int(c.size*0.42f + k*4);
        glColor4f(0.9f,0.92f,0.94f, base*(1.0f - k*0.08f));
        drawFilledCircle((int)(c.x + ox),(int)(c.y + oy), r);
    }
    glDisable(GL_BLEND);
}
void updateClouds(float dt){
    for(auto &c:clouds){ c.x += c.speed * (1.0f + c.depth*0.6f) * dt*60.0f; if(c.x - c.size > WIN_W*2) c.x = -c.size; }
}

// ------------------ Rain physics ------------------
struct Drop {
    float x,y;
    float vx, vy;
    float len;
    bool alive;
};
std::vector<Drop> drops;

struct Splash {
    float x,y;
    float radius;
    float life;
};
std::vector<Splash> splashes;

void initDrops(int count){
    drops.clear(); drops.reserve(count);
    for(int i=0;i<count;i++){
        Drop d; d.x = rand()%(WIN_W*2); d.y = WIN_H - (rand()%WIN_H);
        d.vx = -2.0f + (rand()%5); d.vy = -7.0f - (rand()%8); d.len = 8 + (rand()%12); d.alive=true;
        drops.push_back(d);
    }
}

void updateRain(float dt){
    for(auto &d : drops){
        d.x += d.vx * dt * 60.0f;
        d.y += d.vy * dt * 60.0f;
        // wind wobble
        d.vx += ( (rand()%100)-50 ) * 0.0003f;
        if(d.y < GROUND_Y){
            // spawn splash
            if((int)splashes.size() < MAX_SPLASHES && (rand()%3==0)){
                Splash s; s.x = d.x; s.y = GROUND_Y - 18 + (rand()%12); s.radius = 1.0f; s.life = 1.0f;
                splashes.push_back(s);
            }
            // respawn
            d.x = rand()%(WIN_W*2); d.y = WIN_H - (rand()%150);
            d.vx = -2.0f + (rand()%5); d.vy = -7.0f - (rand()%6); d.len = 8 + (rand()%10);
        }
        if(d.x < -50) d.x = WIN_W*2 + 50;
        if(d.x > WIN_W*2 + 50) d.x = -50;
    }
    // update splashes
    for(auto &s : splashes){
        s.radius += 0.6f;
        s.life -= 0.018f;
    }
    splashes.erase(std::remove_if(splashes.begin(), splashes.end(), [](const Splash &s){ return s.life <= 0.0f; }), splashes.end());
}

void drawRain(){
    glColor3f(0.78f,0.84f,1.0f);
    for(auto &d : drops){
        int x2 = (int)(d.x + d.vx * (d.len / fabs(d.vy)));
        int y2 = (int)(d.y + d.vy * (d.len / fabs(d.vy)));
        drawLineDDA((int)d.x, (int)d.y, x2, y2);
    }
}

void drawSplashes(){
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for(auto &s : splashes){
        float a = s.life * 0.6f;
        glColor4f(0.6f,0.82f,1.0f, a);
        int steps = 80;
        glBegin(GL_POINTS);
        for(int i=0;i<steps;i++){
            float th = (2.0f*(float)PI*i)/steps;
            int px = (int)(s.x + s.radius * cosf(th));
            int py = (int)(s.y + (s.radius*0.5f) * sinf(th));
            putPixel(px, py);
        }
        glEnd();
    }
    glDisable(GL_BLEND);
}

// ------------------ Vehicles with smoother physics ------------------
struct Vehicle {
    float x,y;
    float speed;
    int dir; // 1 right, -1 left
    float targetSpeed;
};
std::vector<Vehicle> cars;
std::vector<Vehicle> bikes;

void spawnVehicles(){
    cars.clear(); bikes.clear();
    for(int i=0;i<10;i++){
        Vehicle v; v.x = rand()%(WIN_W*2); v.y = 72; v.speed = 1.6f + (rand()%30)/20.0f; v.dir = (rand()%2)?1:-1; v.targetSpeed = v.speed;
        cars.push_back(v);
    }
    for(int i=0;i<6;i++){
        Vehicle v; v.x = rand()%(WIN_W*2); v.y = 72 + (rand()%8); v.speed = 2.0f + (rand()%30)/20.0f; v.dir = (rand()%2)?1:-1; v.targetSpeed = v.speed;
        bikes.push_back(v);
    }
}

// simple traffic light point(s) for cars to stop
std::vector<float> trafficLightsX;

void initTraffic(){
    trafficLightsX.clear();
    trafficLightsX.push_back(WIN_W*0.5f);
    trafficLightsX.push_back(WIN_W*1.1f);
}

void updateVehicles(float dt){
    // basic behavior: accelerate towards targetSpeed; random slowdowns
    for(auto &v : cars){
        if(rand()%1000 < 3) v.targetSpeed = clampf(0.5f + (rand()%40)/20.0f, 0.5f, 3.0f);
        // check lights / stopping
        for(auto tx : trafficLightsX){
            float approach = (v.dir==1) ? (tx - v.x) : (v.x - tx);
            if(approach > 0 && approach < 120){
                // if global sun.angle indicates red (simulate) then stop (for demo)
                if( (int)(sun.angle*10) % 17 < 6 ) { // opportunistic red phases
                    v.targetSpeed = 0.0f;
                }
            }
        }
        // smooth accel / decel
        if(v.speed < v.targetSpeed) v.speed = std::min(v.targetSpeed, v.speed + 0.04f * dt * 60.0f);
        else v.speed = std::max(v.targetSpeed, v.speed - 0.06f * dt * 60.0f);
        v.x += v.speed * v.dir * dt * 60.0f;
        if(v.x < -300) v.x = WIN_W*2 + 300;
        if(v.x > WIN_W*2 + 300) v.x = -300;
    }
    for(auto &v : bikes){
        if(rand()%1000 < 4) v.targetSpeed = clampf(0.8f + (rand()%40)/20.0f, 0.8f, 4.0f);
        if(v.speed < v.targetSpeed) v.speed = std::min(v.targetSpeed, v.speed + 0.05f * dt * 60.0f);
        else v.speed = std::max(v.targetSpeed, v.speed - 0.07f * dt * 60.0f);
        v.x += v.speed * v.dir * dt * 60.0f;
        if(v.x < -300) v.x = WIN_W*2 + 300;
        if(v.x > WIN_W*2 + 300) v.x = -300;
    }
}

void drawVehicle(const Vehicle &v){
    // motion trail (cinematic)
    if(cinematic){
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for(int i=1;i<=5;i++){
            float a = 0.08f*(1.0f - i*0.12f);
            float dx = -v.dir * i * (v.speed*6.0f);
            drawRectAlpha((int)(v.x + dx), (int)(v.y+8), 18, 6, 0.9f, 0.3f, 0.25f, a);
        }
        glDisable(GL_BLEND);
    }
    // body
    glColor3f(0.92f,0.24f,0.22f);
    drawFilledRect((int)v.x, (int)v.y, 80, 26);
    // wheels
    glColor3f(0.08f,0.08f,0.08f);
    drawFilledCircle((int)(v.x+16),(int)(v.y-6),8);
    drawFilledCircle((int)(v.x+64),(int)(v.y-6),8);
    // headlight cones
    if(v.dir==1) {
        // additive cone
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        for(int i=0;i<8;i++){
            float a = 0.08f * (1.0f - i/8.0f);
            drawRectAlpha((int)(v.x+80 + i*6), (int)(v.y+4), 36, 18 + i*2, 1.0f,0.98f,0.8f, a);
        }
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDisable(GL_BLEND);
    } else {
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        for(int i=0;i<8;i++){
            float a = 0.08f * (1.0f - i/8.0f);
            drawRectAlpha((int)(v.x - 36 - i*6), (int)(v.y+4), 36, 18 + i*2, 1.0f,0.98f,0.8f, a);
        }
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDisable(GL_BLEND);
    }
}

// ------------------ Pedestrians + pathfinding-ish behavior ------------------
struct Person {
    float x,y;
    int dir;
    float speed;
    float phase;
    bool waiting;
    float goalX;
};
std::vector<Person> people;

void spawnPeople(int n=16){
    people.clear();
    for(int i=0;i<n;i++){
        Person p; p.x = rand()%(WIN_W*2); p.y = GROUND_Y + 12 + (rand()%6);
        p.dir = (rand()%2)?1:-1; p.speed = 0.35f + (rand()%8)*0.03f; p.phase = (rand()%100)/20.0f; p.waiting=false;
        p.goalX = p.x + ( (rand()%2)? 120 : -120 );
        people.push_back(p);
    }
}

void updatePeople(float dt){
    for(auto &p : people){
        // simple local repulsion to avoid overlap
        float push = 0.0f;
        for(auto &o : people){
            if(&o==&p) continue;
            float dx = o.x - p.x;
            if(fabs(dx) < 14.0f){
                push += (dx>0? -0.02f : 0.02f);
            }
        }
        // crosswalk behaviour: if near traffic light and not safe, wait
        bool nearLight = false, canCross = true;
        for(auto tx : trafficLightsX){
            float d = fabs(p.x - tx);
            if(d < 60){ nearLight = true; if(rand()%17 < 10) canCross=false; } // simplified random gating for realism
        }
        if(nearLight && !canCross) { p.waiting = true; /* don't move */ }
        else {
            p.waiting = false;
            // move toward goal (if reached, pick a new goal)
            float dirSign = (p.goalX > p.x) ? 1.0f : -1.0f;
            p.x += (p.speed + push) * dirSign * dt * 60.0f;
            if(fabs(p.goalX - p.x) < 8.0f){ p.goalX = p.x + ( (rand()%2)? 90 : -90 ); }
        }
        // wrap
        if(p.x < -60) p.x = WIN_W*2 + 40;
        if(p.x > WIN_W*2 + 60) p.x = -40;
    }
}

void drawPerson(const Person &p){
    float swing = sinf(simTime*6.0f + p.phase) * 8.0f;
    drawFilledCircle((int)p.x, (int)(p.y + 18), 6);
    glColor3f(0.95f,0.95f,0.98f);
    drawLineDDA((int)p.x, (int)(p.y+12), (int)p.x, (int)(p.y-8));
    drawLineDDA((int)p.x, (int)(p.y+6), (int)(p.x + (int)(swing*0.6f) * p.dir), (int)(p.y+2));
    drawLineDDA((int)p.x, (int)(p.y+6), (int)(p.x - (int)(swing*0.6f) * p.dir), (int)(p.y+2));
    drawLineDDA((int)p.x, (int)(p.y-8), (int)(p.x + (int)(swing*0.9f) * p.dir), (int)(p.y-20));
    drawLineDDA((int)p.x, (int)(p.y-8), (int)(p.x - (int)(swing*0.9f) * p.dir), (int)(p.y-20));
}

// ------------------ Camera timeline (simple keyframes) ------------------
float cameraX=0.0f, cameraZoom=1.0f, camTargetX=0.0f, camTargetZoom=1.0f;
bool cameraAuto = true;
void updateCamera(float dt){
    if(cameraAuto){
        // looped timeline: sweep across center and back
        float cycle = fmod(simTime*0.03f, 1.0f); // long slow cycle
        camTargetX = (sin(cycle*2.0f*PI)*0.5f + 0.5f) * WIN_W * 0.8f; // sweep 0..0.8*WIN_W
        camTargetZoom = 1.0f + 0.06f * sin(simTime*0.2f);
    }
    cameraX += (camTargetX - cameraX) * 0.02f;
    cameraZoom += (camTargetZoom - cameraZoom) * 0.02f;
}

// ------------------ Sky, day-night cycle, bloom helpers ------------------
void drawSky(){
    // time-of-day interpolation
    float dayPhase = (sinf(sun.angle)+1.0f)/2.0f; // 0..1
    // mix colors
    for(int i=0;i<8;i++){
        float t = i/8.0f;
        // base colors shift with dayPhase
        float r = 0.02f + t*(0.06f + 0.15f*dayPhase);
        float g = 0.04f + t*(0.06f + 0.08f*dayPhase);
        float b = 0.08f + t*(0.08f + 0.06f*dayPhase);
        glColor3f(r,g,b);
        glBegin(GL_QUADS);
        int y0 = GROUND_Y + (i*(WIN_H-GROUND_Y)/8);
        int y1 = GROUND_Y + ((i+1)*(WIN_H-GROUND_Y)/8);
        glVertex2i(0,y0); glVertex2i(WIN_W*2,y0); glVertex2i(WIN_W*2,y1); glVertex2i(0,y1);
        glEnd();
    }
    // sun/moon core with bloom
    float cx = WIN_W*1.8f * ((cosf(sun.angle)*0.5f)+0.5f); // sweep across sky
    float cy = WIN_H - 200 + sinf(sun.angle)*60.0f;
    // core
    glColor3f(1.0f,0.94f,0.8f);
    drawFilledCircle((int)cx, (int)cy, 26);
    // bloom (additive multiple passes)
    if(ENABLE_BLOOM){
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        for(int k=1;k<=6;k++){
            float a = 0.08f * (1.0f - k/8.0f);
            drawFilledCircle((int)cx, (int)cy, 26 + k*6);
            glColor4f(1.0f,0.94f,0.8f, a);
        }
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDisable(GL_BLEND);
    }
}

// film grain (fast randomized points)
void drawFilmGrain(float intensity){
    if(!ENABLE_GRAIN || intensity <= 0.001f) return;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f,0.0f,0.0f, intensity);
    glBegin(GL_POINTS);
    int grains = 900;
    for(int i=0;i<grains;i++){
        int x = rand()%WIN_W;
        int y = rand()%WIN_H;
        if(rand()%100 < 55) putPixel(x,y);
    }
    glEnd();
    glDisable(GL_BLEND);
}

// letterbox bars
void drawLetterbox(float h){
    if(h<1) return;
    glColor3f(0.01f,0.01f,0.01f);
    drawFilledRect(0, WIN_H - (int)h, WIN_W, (int)h);
    drawFilledRect(0, 0, WIN_W, (int)h);
}

// ------------------ Render world frame ------------------
void renderWorld(){
    drawSky();
    // clouds back
    for(auto &c: clouds) if(c.depth < 0.5f) drawCloud(c);
    // buildings front
    for(auto &b: buildings) drawBuilding(b,false,1.0f);
    // reflections: blurred layered
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for(int layer=0; layer<3; ++layer){
        float alpha = 0.25f / (1+layer*0.8f);
        for(auto &b: buildings) drawBuilding({b.x,b.y,b.w,b.h,b.baseR,b.baseG,b.baseB,b.brightWindows,b.roofType}, true, alpha);
    }
    glDisable(GL_BLEND);

    // puddles
    glColor3f(0.03f,0.05f,0.08f); drawFilledCircle(260,120,48); drawFilledCircle(620,118,78); drawFilledCircle(980,118,44);

    // splashes
    drawSplashes();

    // road/ground sheen
    glColor3f(0.12f,0.12f,0.14f); drawFilledRect(0,0,WIN_W*2,GROUND_Y);
    glColor3f(0.18f,0.18f,0.20f); drawFilledRect(0,GROUND_Y,WIN_W*2,22);
    glColor3f(0.10f,0.10f,0.12f); drawFilledRect(0,40,WIN_W*2,100);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawRectAlpha(0,58,WIN_W*2,18, 0.22f,0.30f,0.38f, 0.20f);
    glDisable(GL_BLEND);

    // traffic lights indicator (draw crude poles)
    for(auto tx : trafficLightsX){
        glColor3f(0.12f,0.12f,0.12f);
        drawFilledRect((int)tx-6, 90, 12, 60);
        // simple light: alternate with simTime to feel alive
        int idx = (int)(simTime*1.5f) % 3;
        if(idx==0) glColor3f(0.1f,0.8f,0.1f); else if(idx==1) glColor3f(1.0f,0.9f,0.0f); else glColor3f(1.0f,0.2f,0.2f);
        drawFilledCircle((int)tx, 180, 5);
    }

    // vehicles
    for(auto &v : cars) drawVehicle(v);
    for(auto &v : bikes) drawVehicle(v);

    // people
    for(auto &p : people) drawPerson(p);

    // rain overlay
    if(raining){
        drawRain();
    }

    // clouds front
    for(auto &c: clouds) if(c.depth >= 0.5f) drawCloud(c);
}

// ------------------ Display + camera transform ------------------
void display(){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    // center, scale, then translate world for cameraX
    glTranslatef(WIN_W/2.0f, WIN_H/2.0f, 0.0f);
    glScalef(cameraZoom, cameraZoom, 1.0f);
    glTranslatef(-WIN_W/2.0f - cameraX, -WIN_H/2.0f, 0.0f);

    renderWorld();

    glPopMatrix();

    // cinematic overlays
    if(cinematic){
        drawLetterbox(40.0f);
        // vignette - quick darken edges
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        drawRectAlpha(0,0,WIN_W,WIN_H, 0.0f,0.0f,0.0f,0.0f); // placeholder (we keep subtle)
        glDisable(GL_BLEND);
        drawFilmGrain(dayMode?0.02f:0.06f);
    }

    glutSwapBuffers();
}

// ------------------ Animation tick ------------------
void animate(int v){
    float dt = FRAME_MS / 1000.0f;
    simTime += dt * TIME_SCALE;

    // day-night progress
    sun.angle += dt * 0.02f * TIME_SCALE;
    if(sun.angle > 2*PI) sun.angle -= 2*PI;
    dayMode = (cosf(sun.angle) > -0.2f); // crude day detect

    // update systems
    updateClouds(dt);
    if(raining) updateRain(dt);
    updateVehicles(dt);
    updatePeople(dt);
    updateCamera(dt);
    glutPostRedisplay();
    glutTimerFunc(FRAME_MS, animate, 0);
}

// ------------------ Input handlers ------------------
void keyboard(unsigned char key, int x, int y){
    switch(key){
        case 'r': raining = !raining; break;
        case 'd': sun.angle += 0.8f; break; // advance time
        case 't': cameraAuto = !cameraAuto; break;
        case 'c': cinematic = !cinematic; break;
        case 'p': spawnPeople(18); break;
        case 'b': spawnVehicles(); break;
        case '+': camTargetZoom = std::min(1.8f, camTargetZoom + 0.08f); break;
        case '-': camTargetZoom = std::max(0.6f, camTargetZoom - 0.08f); break;
        case 27: exit(0); break;
    }
}

void special(int key,int x,int y){
    if(!cameraAuto){
        if(key == GLUT_KEY_LEFT) camTargetX = std::max(0.0f, camTargetX - 40.0f);
        if(key == GLUT_KEY_RIGHT) camTargetX = std::min((float)WIN_W, camTargetX + 40.0f);
    }
}

// ------------------ Init & main ------------------
void initScene(){
    srand((unsigned)time(NULL));
    buildCity();
    initClouds();
    initDrops(RAIN_PARTICLES);
    initTraffic();
    spawnVehicles();
    spawnPeople(18);
    camTargetX = 0.0f; camTargetZoom = 1.0f; cameraX = 0.0f; cameraZoom = 1.0f;
}

void reshape(int w,int h){
    WIN_W = w; WIN_H = h;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc,char** argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("City After Rain — Refined Cinematic");
    initScene();
    glPointSize(1.2f);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutTimerFunc(FRAME_MS, animate, 0);
    glutMainLoop();
    return 0;
}

#ifndef STATS_HPP
#define STATS_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>

struct Stats {
    int numFrames;
    float timePassed;
    int fps;
    float frameTime;
    float CPUTime;
    float GPUTime;
};

class FPSCounter {
public:
    FPSCounter(){};
    FPSCounter(float updateInterval) : m_updateInterval(updateInterval) {};
    int get() const { return m_fps; }
    void update();
private:
    const float m_updateInterval = 0.25f;
    double m_lastTime = 0.0f;
    double m_updateTimer = 0.0f;
    int m_frameCount = 0;
    int m_fps = 0;
};

class CPUTimer {
public:
    CPUTimer(){};
    CPUTimer(float updateInterval) : m_updateInterval(updateInterval) {};
    float get() const { return m_time; }
    void begin();
    void end();
private:
    const float m_updateInterval = 0.25f;
    double m_lastTime = 0.0f;
    double m_updateTimer = 0.0f;
    int m_frameCount = 0;
    float m_time = 0;
};

class GPUTimer {
    static const int NUM_QUERIES = 4;
    GLuint queries[NUM_QUERIES];
    bool inFlight[NUM_QUERIES] = {false, false, false, false};
    int writeIndex = 0;
    double lastResultMs = 0.0;

public:
    void init(){ glGenQueries(NUM_QUERIES, queries); }
    void beginFrame();
    void endFrame();
    float getLastResultMs() const { return (float)lastResultMs; }
};

#endif
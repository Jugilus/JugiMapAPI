#ifndef JUGIMAP_TWEEN_H
#define JUGIMAP_TWEEN_H

#include <cmath>
#include <vector>
#include <string>

#include "jmCommon.h"



namespace jugimap {



class Tweens;


class Tween
{
public:
    friend class Tweens;

    enum class Mode{NORMAL, REVERSE, LOOP, LOOP_REVERSE};

    ~Tween();

    void Init(float _valueStart, float _valueEnd, float _durationS, Easing::Kind _easingKind);
    void Play();
    bool IsIdle() { return state==stateIDLE; }

    void Stop();
    void Pause();
    void Resume();

    float GetValue() { return value;}
    void SetEasingKind(Easing::Kind _kind) { easing.kind = _kind; }
    void SetMode(Tween::Mode _mode) { mode = _mode;}
    float GetStartValue(){ return valueStart;}
    float GetEndValue(){ return valueEnd;}

    float Update();

private:
    float value = 0;
    float valueStart = 0.0;
    float valueEnd = 0.0;
    int timeStartMS = 0;
    int timeEndMS = 0;
    int durationMS = 0;
    Easing easing;

    int state = stateNEVER_INITIALIZED;
    static const int stateNEVER_INITIALIZED = -1;
    static const int stateIDLE = 0;
    static const int statePLAYING = 1;
    static const int statePAUSED = 3;


    Mode mode = Mode::NORMAL;
    bool reverse = false;

    int stateStored = 0;
    int msTimeStored = 0;


};



class Tweens
{
public:
    friend class Tween;

    static void Update();
    static void RemoveTween(Tween * _tween);

private:
    static std::vector<Tween*>vTweens;        // LINK pointers

};







}

#endif



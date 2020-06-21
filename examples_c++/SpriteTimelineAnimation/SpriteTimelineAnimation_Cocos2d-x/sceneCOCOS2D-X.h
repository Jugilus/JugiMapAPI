#ifndef API_DEMO_TEST__SCENE_COCOS2D_H
#define API_DEMO_TEST__SCENE_COCOS2D_H


#include "jugimapCOCOS2D-X/jmCocos2d-x.h"
#include "engineIndependent/scene.h"





class DemoSceneCC : public DemoScene
{
public:

    bool Init() override;


private:

    void AddErrorMessageTextNode();

};


//=====================================================================================


// Events layer for mouse reading. used with ParallaxScrollingSceneCC.
class EventsLayer : public cocos2d::Layer
{
public:
    static EventsLayer * Create();
    bool Init();

private:
    void OnMouseDown(cocos2d::Event *_event);
    void OnMouseUp(cocos2d::Event *_event);
    void OnMouseMove(cocos2d::Event *_event);
    void OnMouseScroll(cocos2d::Event *_event);
    void OnKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
    void OnKeyReleased(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
};




#endif

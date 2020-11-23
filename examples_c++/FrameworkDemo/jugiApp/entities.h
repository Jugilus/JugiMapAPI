#ifndef API_DEMO_TEST__ENTITIES_H
#define API_DEMO_TEST__ENTITIES_H


#include "../jugimap/jugimap.h"



namespace jugiApp{


class ESimpleAnimatedObject;
class EWalkingFella;
class ERollingStone;
class EMovingPlatform;


class Entity
{
public:
    friend class ESimpleAnimatedObject;
    friend class EWalkingFella;
    friend class ERollingStone;
    friend class EMovingPlatform;

    Entity(){}
    Entity(const Entity &_src)
    {
        active = _src.active;
    }

    Entity& operator=(const Entity &_src)
    {
        if(this == &_src) return *this;

        active = _src.active;

        return *this;
    }

    virtual ~Entity(){}

    virtual bool Init(jugimap::Sprite *_sprite) = 0;
    virtual void Start() = 0;
    virtual void Update() = 0;
    virtual void SetPaused(bool _paused) = 0;
    virtual void Deactivate(){}
    virtual void OnDeleteFromScene(){}

    bool IsActive(){return active;}

private:

    bool active = false;
};


class Entities
{
public:


    bool Setup(jugimap::Map *map);
    void Start();
    void Update();
    void SetPaused(bool _paused);
    bool IsPaused(){ return paused; }
    void Delete();
    void AddEntity(Entity *e){list.push_back(e);}
    void DeleteEntity(Entity *e);
    std::vector<Entity*>& GetList(){ return list; }

private:
    std::vector<Entity*>list;
    bool paused = false;
};


extern Entities entities;



class ESimpleAnimatedObject : public Entity
{
public:

    bool Init(jugimap::Sprite *_sprite) override;
    void Start() override;
    void Update() override;
    void SetPaused(bool _paused) override;

private:
    jugimap::StandardSprite  *sprite = nullptr;                     // LINK to controlled sprite
    std::unique_ptr<jugimap::FrameAnimationInstance>aniDefault;
    jugimap::AnimationPlayer animationPlayer;

};



class EWalkingFella : public Entity
{
public:
    static std::vector<EWalkingFella*>templateFellas;
    static std::vector<EWalkingFella*>deactivatedFellas;      // LINKs to deactivated entities


    bool Init(jugimap::Sprite *_sprite) override;
    void Start() override;
    void Update() override;
    void SetPaused(bool _paused) override;
    void Deactivate() override;
    void OnDeleteFromScene() override;

    void CopyPropertiesFrom(EWalkingFella* _src);
    void CopyToTemplates();
    void Spawn();
    jugimap::StandardSprite  *GetControlledSprite(){return sprite;}


private:
    jugimap::StandardSprite  *sprite = nullptr;                     // LINK pointer


    //--- data
    float speed = 0.0;                                      // pixels per second
    int msTimerStart = -1;
    int msWaitTime = 0;
    std::unique_ptr<jugimap::FrameAnimationInstance>aniStandLeft;
    std::unique_ptr<jugimap::FrameAnimationInstance>aniStandRight;
    std::unique_ptr<jugimap::FrameAnimationInstance>aniWalkLeft;
    std::unique_ptr<jugimap::FrameAnimationInstance>aniWalkRight;
    jugimap::FrameAnimationInstance *aniActive = nullptr;               // LINK

    jugimap::AnimationPlayer animationPlayer;


    //--- controlling variables
    int state = 0;
    static const int stateSTAND = 1;
    static const int stateWALK = 2;

    int direction = 0;
    static const int dirLEFT = 1;
    static const int dirRIGHT = 2;

    //---
    int startState = 0;
    int startDirection = 0;

};



//==========================================================================================


class ERollingStone : public Entity
{
public:


    bool Init(jugimap::Sprite *_sprite) override;
    void Start() override;
    void Update() override;
    void SetPaused(bool _paused) override;
    void OnDeleteFromScene() override;

private:
    jugimap::StandardSprite  *sprite = nullptr;      // LINK to controlled sprite

    //--- data
    int radius = 0;
    float speed = 0.0;
    jugimap::Tween tweenStoneRotation;

    //--- controlling variables
    int direction = 0;
    static const int dirLEFT = 1;
    static const int dirRIGHT = 2;

    //---
    int startDirection = 0;
};


//==========================================================================================


class EMovingPlatform : public Entity
{
public:
    ~EMovingPlatform() override;

    bool Init(jugimap::Sprite *_sprite) override;
    void Start() override;
    void Update() override;
    void SetPaused(bool _paused) override;
    void OnDeleteFromScene() override;


private:
    jugimap::StandardSprite  *sprite = nullptr;              // LINK to controlled sprite

    //--- data
    float speed = 0.0;
    jugimap::VectorShape* movementPath = nullptr;           // LINK pointer to vector shape
    jugimap::Tween tweenPathMovement;


    //--- controlling variables
    int state = 0;
    static const int stateMOVING_FORWARD = 1;
    static const int stateMOVING_BACK = 2;

    //---
    int startState = 0;

};

}


#endif

#include <chrono>
#include <assert.h>
#include "parallaxScene.h"



using namespace jugimap;





Mouse mouse;



//==============================================================================================


void CameraController::Init(WorldMapCamera *_cameraView, jugimap::VectorShape *_cameraPath, bool _ownCameraPath)
{
    camera = _cameraView;
    cameraPath = _cameraPath;
    ownCameraPath = _ownCameraPath;

    //----
    //cameraPath->CalculatePathLength();
    float speed = 200;                                       // 50 pixels per second
    float duration = cameraPath->GetPathLength() / speed;

    //----
    movingTween.Init(0.0, 1.0, duration, Easing::Kind::LINEAR);
    movingTween.SetMode(Tween::Mode::LOOP);
    movingTween.Play();

    Vec2f pos = cameraPath->GetPathPoint(0.0);   // start position
    camera->SetPointedPosition(pos, true);
}


void CameraController::Start()
{
    movingTween.Play();
}


void CameraController::Update()
{
    if(cameraPath==nullptr) return;
    //return;

    float p = movingTween.Update();
    Vec2f pos = cameraPath->GetPathPoint(p);
    camera->SetPointedPosition(pos);
}


void CameraController::SetPaused(bool _paused)
{

    if(cameraPath==nullptr) return;
    paused = _paused;

    if(paused){
        movingTween.Pause();
    }else{
        movingTween.Resume();
    }

}



//==============================================================================================


VectorShape* MakeWorldCameraOverviewPath_v1(WorldMapCamera &_camera)
{

    Vec2f min =  _camera.GetViewport().Size()/2.0;
    Vec2f max =  Vec2iToVec2f(_camera.GetWorldSize()) - _camera.GetViewport().Size()/2.0;
    if(max.x<min.x) max.x = min.x;
    if(max.y<min.y) max.y = min.y;
    if(settings.IsYCoordinateUp()){
        std::swap(min.y,max.y);
    }

    PolyLineShape *shape = new PolyLineShape();
    shape->vertices.push_back(Vec2f(min.x, max.y));
    shape->vertices.push_back(Vec2f(max.x, min.y));
    shape->vertices.push_back(max);
    shape->vertices.push_back(min);
    shape->vertices.push_back(Vec2f(min.x, max.y));
    shape->RebuildPath();

    VectorShape *cameraPath = new VectorShape(shape);

    return cameraPath;
}


//==============================================================================================


ParallaxScene::~ParallaxScene()
{

    if(worldMap) delete worldMap;
    if(backgroundMap) delete backgroundMap;
    if(worldSourceGraphics) delete worldSourceGraphics;
    if(backgroundSourceGraphics) delete backgroundSourceGraphics;
    if(testScreenMap) delete testScreenMap;

    delete aniA;
    delete aniB;
    delete aniC;

}



bool ParallaxScene::Init()
{

    //------------------------------------------------------------------------------
    // LOAD SOURCE GRAPHICS AND MAPS
    //------------------------------------------------------------------------------

    std::string worldMapFile = "media/maps/world.jmb";
    std::string worldSourceGraphicsFile = "media/maps/cfgWorld.sgb";
    std::string backgroundMapFile = "media/maps/background.jmb";
    std::string backgroundSourceGraphicsFile = "media/maps/cfgBackground.sgb";
    std::string testScreenMapFile = "media/maps/testScreenMap.jmb";

    bool includeTestScreenMap = false;          // set to true to include dummy testing screen map


    //---- world map
    worldSourceGraphics = new SourceGraphics();
    if(JugiMapBinaryLoader::LoadSourceGraphics(worldSourceGraphicsFile, worldSourceGraphics)==false){
        return false;
    }

    worldMap = objectFactory->NewMap();
    worldMap->_SetZOrderStart(zOrderWorldMap);
    if(JugiMapBinaryLoader::LoadMap(worldMapFile, worldMap, worldSourceGraphics)==false){
        return false;
    }


    //---- background
    backgroundSourceGraphics = new SourceGraphics();
    if(JugiMapBinaryLoader::LoadSourceGraphics(backgroundSourceGraphicsFile, backgroundSourceGraphics)==false){
        return false;
    }

    backgroundMap = objectFactory->NewMap();
    backgroundMap->_SetZOrderStart(zOrderBackgroundMap);
    if(JugiMapBinaryLoader::LoadMap(backgroundMapFile, backgroundMap, backgroundSourceGraphics)==false){
        return false;
    }


    //---- screen map
    if(includeTestScreenMap){
        testScreenMap = objectFactory->NewMap();
        testScreenMap->_SetZOrderStart(zOrderScreenMap);
        if(JugiMapBinaryLoader::LoadMap(testScreenMapFile, testScreenMap, backgroundSourceGraphics)==false){
            return false;
        }
    }



    //------------------------------------------------------------------------------
    // INITIALIZE MAPS
    //------------------------------------------------------------------------------

    // Initialize maps according to their kind - make sure to call correct Init function!
    // First initialize world map as its size is needed for parallax maps!
    worldMap->InitWorldMap();
    backgroundMap->InitParallaxMap(worldMap->GetWorldMapSize());

    // The parallax map has fixed height (no tiling) and y parallax factors set in the editor are valid for a designed size 1300x800.
    // If screen (viewport) size has different height we need to adjust these factors, otherwise parallax layers are not positioned correctly in y direction.
    backgroundMap->ModifyYParallaxFactorsForFixedMapHeight(Vec2i(1300,800));

    if(testScreenMap) testScreenMap->InitScreenMap(Vec2i(200, settings.GetScreenSize().y));

    // Initialize engine maps - creates engine objects (textures, sprites...) and get ready everything for action
    worldMap->InitEngineMap();
    backgroundMap->InitEngineMap();
    if(testScreenMap) testScreenMap->InitEngineMap();


    //------------------------------------------------------------------------------
    // INITIALIZE CAMERAS
    //------------------------------------------------------------------------------
    Vec2i screenSize = settings.GetScreenSize();
    if(testScreenMap){
        worldCamera.Init(Rectf(200,0, screenSize.x, screenSize.y), worldMap->GetWorldMapSize());
        screenCamera.Init(testScreenMap->GetScreenMapDesignSize(), Vec2f(0,0));
    }else{
        worldCamera.Init(Rectf(0,0, screenSize.x, screenSize.y), worldMap->GetWorldMapSize());
    }

    // Assign cameras to maps.
    backgroundMap->SetCamera(&worldCamera);
    worldMap->SetCamera(&worldCamera);
    if(testScreenMap) testScreenMap->SetCamera(&screenCamera);

    // Initialize camera controller which moves camera around the map
    cameraController.Init(&worldCamera, MakeWorldCameraOverviewPath_v1(worldCamera), true);


    //------------------------------------------------------------------------------
    // PREPARE ANIMATIONS - a simple method to animate that waterfall
    //-------------------------------------------------------------------------------
    std::vector<Sprite*>sprites;
    CollectSpritesWithSourceSpriteName(backgroundMap, sprites, "waterAniA");
    assert(sprites.empty()==false);
    assert(sprites.front()->GetSourceSprite()->GetFrameAnimations().empty()==false);
    if(sprites.size()==1){
        aniA = new FrameAnimationInstance(sprites.front()->GetSourceSprite()->GetFrameAnimations().front(), sprites.front());
    }else{
        aniA = new FrameAnimationInstance(sprites.front()->GetSourceSprite()->GetFrameAnimations().front(), sprites);
    }


    sprites.clear();
    CollectSpritesWithSourceSpriteName(backgroundMap, sprites, "waterAniB");
    assert(sprites.size()==1);
    assert(sprites.front()->GetSourceSprite()->GetFrameAnimations().empty()==false);
    if(sprites.size()==1){
        aniB = new FrameAnimationInstance(sprites.front()->GetSourceSprite()->GetFrameAnimations().front(), sprites.front());
    }else{
        aniB = new FrameAnimationInstance(sprites.front()->GetSourceSprite()->GetFrameAnimations().front(), sprites);
    }

    sprites.clear();
    CollectSpritesWithSourceSpriteName(backgroundMap, sprites, "waterAniC");
    assert(sprites.size()==1);
    assert(sprites.front()->GetSourceSprite()->GetFrameAnimations().empty()==false);
    if(sprites.size()==1){
        aniC = new FrameAnimationInstance(sprites.front()->GetSourceSprite()->GetFrameAnimations().front(), sprites.front());
    }else{
        aniC = new FrameAnimationInstance(sprites.front()->GetSourceSprite()->GetFrameAnimations().front(), sprites);
    }

    SetInitialized(true);

    return true;
}



void ParallaxScene::Start()
{
    cameraController.Start();

    if(aniPlayerA.Play(aniA) & AnimationPlayerFlags::ANIMATED_PROPERTIES_CHANGED){
        aniA->UpdateAnimatedSprites(true);
    }
    if(aniPlayerB.Play(aniB) & AnimationPlayerFlags::ANIMATED_PROPERTIES_CHANGED){
        aniB->UpdateAnimatedSprites(true);
    }
    if(aniPlayerC.Play(aniC) & AnimationPlayerFlags::ANIMATED_PROPERTIES_CHANGED){
        aniC->UpdateAnimatedSprites(true);
    }
}



void ParallaxScene::Update()
{

    //---- update global time variable
    jugimap::time.UpdatePassedTimeMS();


    //---- scene logic
    if(mouse.IsHit()){
        cameraController.SetPaused(!cameraController.IsPaused());
    }


    if(aniPlayerA.Update() & AnimationPlayerFlags::ANIMATED_PROPERTIES_CHANGED){
        aniA->UpdateAnimatedSprites(true);
    }
    if(aniPlayerB.Update() & AnimationPlayerFlags::ANIMATED_PROPERTIES_CHANGED){
        aniB->UpdateAnimatedSprites(true);
    }
    if(aniPlayerC.Update() & AnimationPlayerFlags::ANIMATED_PROPERTIES_CHANGED){
        aniC->UpdateAnimatedSprites(true);
    }


    cameraController.Update();


    //---- update engine sprites
    UpdateEngineObjects();
}



void ParallaxScene::UpdateEngineObjects()
{
    backgroundMap->UpdateEngineMap();
    worldMap->UpdateEngineMap();
    if(testScreenMap) testScreenMap->UpdateEngineMap();
}









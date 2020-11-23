#include <assert.h>
#include <algorithm>
#include "jmSprites.h"
#include "jmVectorShapes.h"
#include "jmUtilities.h"
#include "jmCamera.h"
#include "jmMap.h"
#include "jmLayers.h"

using namespace jugimap;



namespace jugimap {



void Layer::InitLayerParameters()
{

    for(Parameter &PV : parameters){

        /*
        if(PV.name=="alignX"){

            if(PV.value=="MIDDLE"){
                alignX = AlignX::MIDDLE;

            }else if(PV.value=="LEFT"){
                alignX = AlignX::LEFT;

            }else if(PV.value=="RIGHT"){
                alignX = AlignX::RIGHT;

            }

        }else if(PV.name=="alignY"){

            if(PV.value=="MIDDLE"){
                alignY = AlignY::MIDDLE;

            }else if(PV.value=="BOTTOM"){
                alignY = AlignY::BOTTOM;

            }else if(PV.value=="TOP"){
                alignY = AlignY::TOP;

            }

        }else if(PV.name=="alignOffsetX"){

            alignOffset.x = std::stoi(PV.value);


        }else if(PV.name=="alignOffsetY"){

            alignOffset.y = std::stoi(PV.value);
            if(settings.IsYCoordinateUp()){
                alignOffset.y = - alignOffset.y;
            }


        }else if(PV.name=="parallaxLayerMode"){

            if(PV.value=="NO CHANGE"){
                parallaxLayerMode = ParallaxLayerMode::NO_CHANGE;

            }else if(PV.value=="TILING X"){
                parallaxLayerMode = ParallaxLayerMode::TILING_X;

            }else if(PV.value=="TILING Y"){
                parallaxLayerMode = ParallaxLayerMode::TILING_Y;

            }else if(PV.value=="TILING XY"){
                parallaxLayerMode = ParallaxLayerMode::TILING_XY;

            }else if(PV.value=="SINGLE SPRITE STRETCH X"){
                parallaxLayerMode = ParallaxLayerMode::SINGLE_SPRITE_STRETCH_X;

            }else if(PV.value=="SINGLE SPRITE STRETCH Y"){
                parallaxLayerMode = ParallaxLayerMode::SINGLE_SPRITE_STRETCH_Y;

            }else if(PV.value=="SINGLE SPRITE STRETCH XY"){
                parallaxLayerMode = ParallaxLayerMode::SINGLE_SPRITE_STRETCH_XY;

            }

        }else if(PV.name=="parallaxFactorX"){
            parallaxFactor.x = std::stof(PV.value);

        }else if(PV.name=="parallaxFactorY"){
            parallaxFactor.y = std::stof(PV.value);


        }else if(PV.name=="screenLayerMode"){

            if(PV.value=="NO CHANGE"){
                screenLayerMode = ScreenLayerMode::NO_CHANGE;

            }else if(PV.value=="SINGLE SPRITE STRETCH X"){
                screenLayerMode = ScreenLayerMode::SINGLE_SPRITE_STRETCH_X;

            }else if(PV.value=="SINGLE SPRITE STRETCH Y"){
                screenLayerMode = ScreenLayerMode::SINGLE_SPRITE_STRETCH_Y;

            }else if(PV.value=="SINGLE SPRITE STRETCH XY"){
                screenLayerMode = ScreenLayerMode::SINGLE_SPRITE_STRETCH_XY;
            }

        }else if(PV.name=="attachToLayer"){
            linkedLayerName = PV.value;

        }
        */
    }

}



bool Layer::UpdateParallaxOffset()
{

    WorldMapCamera *camera = dynamic_cast<WorldMapCamera*>(GetMap()->GetCamera());
    assert(camera);
    Vec2i mapSize = camera->GetWorldSize();
    Vec2f pointedWorldPos = camera->GetPointedPosition();
    Rectf mapScrollableRect = camera->GetMapScrollableRect();

    float fX = 0.0;
    if(mapScrollableRect.GetWidth()>0){
        fX = (pointedWorldPos.x-mapScrollableRect.min.x)/(mapScrollableRect.GetWidth());
    }

    float fY = 0.0;
    if(mapScrollableRect.GetHeight()>0){
        fY = (pointedWorldPos.y-mapScrollableRect.min.y)/(mapScrollableRect.GetHeight());
    }

    Vec2f pRange(mapScrollableRect.GetWidth()*(1.0-parallaxFactor.x), mapScrollableRect.GetHeight()*(1.0-parallaxFactor.y));

    //----
    float xAlignLEFT = pRange.x * fX;
    float yAlignBOTTOM = pRange.y * fY;
    float xAlignRIGHT = mapSize.x - layersPlaneSize.x - pRange.x*(1-fX);
    float yAlignTOP = mapSize.y - layersPlaneSize.y - pRange.y*(1-fY);

    if(settings.IsYCoordinateUp()==false){
        yAlignTOP = pRange.y * fY;
        yAlignBOTTOM = mapSize.y - layersPlaneSize.y - pRange.y*(1-fY);
    }

    //----
    Vec2f parallaxOffsetCurrent;

    /*
    if(alignX==AlignX::LEFT){
        parallaxOffsetCurrent.x = xAlignLEFT;

    }else if(alignX==AlignX::MIDDLE){
        parallaxOffsetCurrent.x = (xAlignLEFT+xAlignRIGHT)/2.0;

    }else if(alignX==AlignX::RIGHT){
        parallaxOffsetCurrent.x = xAlignRIGHT;
    }

    if(alignY==AlignY::BOTTOM){
        parallaxOffsetCurrent.y = yAlignBOTTOM;

    }else if(alignY==AlignY::MIDDLE){
        parallaxOffsetCurrent.y = (yAlignBOTTOM+yAlignTOP)/2.0;

    }else if(alignY==AlignY::TOP){
        parallaxOffsetCurrent.y = yAlignTOP;
    }
    */

    // alignPosition.x = 0  ->  LEFT
    // alignPosition.x = 100  ->  RIGHT
    // alignPosition.y = 0  ->  TOP
    // alignPosition.y = 100  ->  BOTTOM


    parallaxOffsetCurrent.x = xAlignLEFT * (1.0 - alignPosition.x/100.0) +  xAlignRIGHT * (alignPosition.x/100.0);
    parallaxOffsetCurrent.y = yAlignTOP * (1.0 - alignPosition.y/100.0) +  yAlignBOTTOM * (alignPosition.y/100.0);


    parallaxOffsetCurrent = parallaxOffsetCurrent + alignOffset;

    //----
    //if(AreSameVec2f(parallaxOffsetCurrent, parallaxOffset)==false){
    if(parallaxOffsetCurrent.Equals(parallaxOffset)==false){
        parallaxOffset = parallaxOffsetCurrent;
        return true;
    }

    return false;
}


//==================================================================================================


SpriteLayer::SpriteLayer()
{
    _SetKind(LayerKind::SPRITE);
}


SpriteLayer::~SpriteLayer()
{
    for(Sprite *s : sprites){
        delete s;
    }
}


void SpriteLayer::InitEngineObjects()
{

    if(GetName()=="glow"){
        DummyFunction();
    }

    if(GetParentComposedSprite()){          // For map layers this was already called.
        InitLayerParameters();
    }

    //---
    for(Sprite *s : sprites){
        s->InitEngineObjects();
    }
}


void SpriteLayer::InitLayerParameters()
{

    Layer::InitLayerParameters();


    for(Parameter &PV : GetParameters()){
        if(PV.name=="alpha"){

            alpha = std::stod(PV.value);

        }else if(PV.name=="blending"){

            if(PV.value=="ALPHA"){
                blend = Blend::ALPHA;

            }else if(PV.value=="MULTIPLY"){
                blend = Blend::MULTIPLY;

            }else if(PV.value=="LINEAR DODGE"){
                blend = Blend::ADDITIVE;

            }else if(PV.value=="SOLID COVER"){
                blend = Blend::SOLID;
            }
        }
    }

}


void SpriteLayer::UpdateEngineObjects()
{

    if(GetMap()){

        if(GetMap()->GetType()==MapType::PARALLAX){

            if(GetLayerType()==LayerType::PARALLAX){
                if(UpdateParallaxOffset()){
                    AppendToSpritesChangeFlag(Sprite::Property::POSITION);
                };

            }else if(GetLayerType()==LayerType::PARALLAX_STRETCHING_SINGLE_SPRITE){
                if(UpdateStretchingSingleSpriteLayer()){
                    AppendToSpritesChangeFlag(Sprite::Property::TRANSFORMATION);
                }
            }


        }else if(GetMap()->GetType()==MapType::SCREEN){

            if(GetLayerType()==LayerType::SCREEN_STRETCHING_SINGLE_SPRITE){
                if(UpdateStretchingSingleSpriteLayer()){
                    AppendToSpritesChangeFlag(Sprite::Property::TRANSFORMATION);
                }
            }
        }
    }


    if(spritesChanged){

        if(spritesChangeFlag!=0){
            for(Sprite *s : sprites){
                s->AppendToChangeFlags(spritesChangeFlag);
            }
        }

        for(Sprite *s : sprites){
            s->UpdateEngineObjects();
        }
    }

    //---
    spritesChanged = false;
    spritesChangeFlag = 0;
}


void SpriteLayer::DrawEngineObjects()
{
    for(Sprite *s : sprites){
        s->DrawEngineSprite();
    }
}



void SpriteLayer::UpdateSingleSpriteStretch(Vec2i _designSize, bool _stretchX, bool _stretchY)
{
/*
    if(GetSprites().empty()) return;
    StandardSprite  *s = dynamic_cast<StandardSprite *>(GetSprites().front());
    if(s==nullptr || s->GetActiveImage()==nullptr) return;


    Vec2f spriteSize = Vec2iToVec2f(s->GetActiveImage()->GetSize());
    Vec2f spriteScale(1.0, 1.0);
    Vec2f spritePosition(_designSize.x/2.0, _designSize.y/2.0);
    Vec2f spriteHandle(spriteSize.x/2.0, spriteSize.y/2.0);

    //alignY = AlignY::MIDDLE;
    //alignOffset = Vec2f(20,20);

    //_stretchX = true;
    //_stretchY = false;
    //alignX = AlignX::LEFT;


    if(_stretchX && _stretchY){
        spriteScale.x = _designSize.x/spriteSize.x;
        spriteScale.y = _designSize.y/spriteSize.y;


    }else if(_stretchX){
        spriteScale.x = _designSize.x/spriteSize.x;

        if(GetAlignY()==AlignY::BOTTOM){
            if(settings.IsYCoordinateUp()){
                spritePosition.y = spriteHandle.y;
            }else{
                spritePosition.y = _designSize.y - spriteHandle.y;
            }

        }else if(GetAlignY()==AlignY::MIDDLE){


        }else if(GetAlignY()==AlignY::TOP){
            if(settings.IsYCoordinateUp()){
                spritePosition.y = _designSize.y - spriteHandle.y;
            }else{
                spritePosition.y = spriteHandle.y;
            }
        }

    }else if(_stretchY){
        spriteScale.y = _designSize.y/spriteSize.y;

        if(GetAlignX()==AlignX::LEFT){
            spritePosition.x = spriteHandle.x;

        }else if(GetAlignX()==AlignX::MIDDLE){

        }else if(GetAlignX()==AlignX::RIGHT){
            spritePosition.x = _designSize.x - spriteHandle.x;
        }
    }

    spritePosition = spritePosition + GetAlignOffset();


    if(spriteHandle.Equals(s->GetHandle())==false || spritePosition.Equals(s->GetPosition())==false || spriteScale.Equals(s->GetScale())==false){
        s->SetHandle(spriteHandle);
        s->SetPosition(spritePosition);
        s->SetScale(spriteScale);
        s->AppendToChangeFlags(Sprite::Property::TRANSFORMATION);
        s->CaptureForLerpDrawing();            // needed only for engines using 'lerp' tweening for drawing sprites
    }

*/
}



bool SpriteLayer::UpdateStretchingSingleSpriteLayer()
{

    if(GetSprites().empty()) return false;
    StandardSprite  *s = dynamic_cast<StandardSprite *>(GetSprites().front());
    if(s==nullptr || s->GetActiveImage()==nullptr) return false;


    Vec2f spriteSize = Vec2iToVec2f(s->GetActiveImage()->GetSize());
    Vec2f spriteHandle(spriteSize.x/2.0, spriteSize.y/2.0);
    Vec2f spritePosition;

    //---
    Vec2f rectSize;

    if(GetMap()->GetType()==MapType::PARALLAX){

        WorldMapCamera *camera = dynamic_cast<WorldMapCamera*>(GetMap()->GetCamera());
        assert(camera);

        if(GetStretchingVariant()==StretchingVariant::XY_TO_WORLD_SIZE){
            rectSize = Vec2f(camera->GetWorldSize().x, camera->GetWorldSize().y);
            spritePosition = rectSize/2.0;

        }else if(GetStretchingVariant()==StretchingVariant::XY_TO_VIEWPORT_SIZE){
            rectSize = camera->GetViewport().GetSize();
            spritePosition = camera->GetPointedPosition();
        }

    }else if(GetMap()->GetType()==MapType::SCREEN){
        rectSize = Vec2f(GetMap()->GetScreenMapDesignSize().x, GetMap()->GetScreenMapDesignSize().y);
        spritePosition = rectSize/2.0;
    }

    Vec2f spriteScale = rectSize / spriteSize;



    //alignY = AlignY::MIDDLE;
    //alignOffset = Vec2f(20,20);

    //_stretchX = true;
    //_stretchY = false;
    //alignX = AlignX::LEFT;


    /*
    if(_stretchX && _stretchY){
        spriteScale.x = _designSize.x/spriteSize.x;
        spriteScale.y = _designSize.y/spriteSize.y;


    }else if(_stretchX){
        spriteScale.x = _designSize.x/spriteSize.x;

        if(GetAlignY()==AlignY::BOTTOM){
            if(settings.IsYCoordinateUp()){
                spritePosition.y = spriteHandle.y;
            }else{
                spritePosition.y = _designSize.y - spriteHandle.y;
            }

        }else if(GetAlignY()==AlignY::MIDDLE){


        }else if(GetAlignY()==AlignY::TOP){
            if(settings.IsYCoordinateUp()){
                spritePosition.y = _designSize.y - spriteHandle.y;
            }else{
                spritePosition.y = spriteHandle.y;
            }
        }

    }else if(_stretchY){
        spriteScale.y = _designSize.y/spriteSize.y;

        if(GetAlignX()==AlignX::LEFT){
            spritePosition.x = spriteHandle.x;

        }else if(GetAlignX()==AlignX::MIDDLE){

        }else if(GetAlignX()==AlignX::RIGHT){
            spritePosition.x = _designSize.x - spriteHandle.x;
        }
    }
    */

    //spritePosition = spritePosition + GetAlignOffset();


    if(spriteHandle.Equals(s->GetHandle())==false || spritePosition.Equals(s->GetPosition())==false || spriteScale.Equals(s->GetScale())==false){
        s->SetHandle(spriteHandle);
        s->SetPosition(spritePosition);
        s->SetScale(spriteScale);
        s->CaptureForLerpDrawing();            // needed only for engines using 'lerp' tweening for drawing sprites

        return true;
    }

    return false;
}


void SpriteLayer::SetAlpha(float _alpha)
{
    alpha = _alpha;
    spritesChangeFlag |= Sprite::Property::ALPHA;
    spritesChanged = true;
}


void SpriteLayer::SetBlend(Blend _blend)
{
    blend = _blend;
    spritesChangeFlag |= Sprite::Property::BLEND;
    spritesChanged = true;
}


float SpriteLayer::GetAlpha()
{
    if(GetParentComposedSprite()){
        return alpha * GetParentComposedSprite()->GetAlpha();
    }else{
        return alpha;
    }
}


void SpriteLayer::AddSprite(Sprite* _sprite)
{
    sprites.push_back(_sprite);
    _sprite->_SetLayer(this);
    SetSpritesChanged(true);
}


StandardSprite * SpriteLayer::AddStandardSprite(SourceSprite *_sourceSprite, Vec2f _position)
{

    //StandardSprite *s = objectFactory->NewStandardSprite(_sourceSprite, _position, this);
    assert(_sourceSprite->GetKind()==SpriteKind::STANDARD);

    StandardSprite *s = objectFactory->NewStandardSprite();
    s->_SetSourceSprite(_sourceSprite);
    s->_SetLayer(this);
    s->SetPosition(_position);
    sprites.push_back(s);
    s->InitEngineObjects();

    return s;
}


TextSprite * SpriteLayer::AddTextSprite(const std::string &_textString, Font* _font, ColorRGBA _color, Vec2f _position, TextHandleVariant _textHandle)
{

    //TextSprite* ts = objectFactory->NewTextSprite(_textString, _font, _color, _position, _textHandle, this);
    TextSprite* ts = objectFactory->NewTextSprite();
    ts->_SetSourceSprite(SourceGraphics::dummyTextSourceSprite);
    ts->_SetLayer(this);
    ts->SetPosition(_position);
    ts->_SetRelativeHandle(GetRelativeHandleFromTextHandleVariant(_textHandle));
    ts->_SetFont(_font);
    ts->SetTextString(_textString);
    ts->SetColor(_color);
    sprites.push_back(ts);
    ts->InitEngineObjects();

    return ts;
}


void SpriteLayer::RemoveAndDeleteSprite(Sprite *_sprite)
{

    std::vector<Sprite*>::iterator i = sprites.begin();
    while (i != sprites.end())
    {
        if(_sprite == *i){
            i = sprites.erase(i);
        }else{
            i++;
        }
    }
    delete _sprite;
}


void SpriteLayer::UpdateBoundingBox()
{

    boundingBox = Rectf(999999.0, 999999.0, -999999.0, -999999.0);               // initial box

    for(Sprite *s : sprites){
        s->UpdateBoundingBox();
        boundingBox = boundingBox.Unite(s->GetBoundingBox());
    }
}


void SpriteLayer::CaptureForLerpDrawing()
{

    for(Sprite *s : GetSprites()){
        s->CaptureForLerpDrawing();
    }
}


int SpriteLayer::FindLastZOrder()
{

    if(GetSprites().empty()) return GetZOrder();

    int zOrder = GetZOrder();

    for(Sprite *s : GetSprites()){
        if(s->GetKind()==SpriteKind::COMPOSED){
            ComposedSprite *cs = static_cast<ComposedSprite*>(s);
            for(Layer* l : cs->GetLayers()){
                if(l->GetKind()==LayerKind::SPRITE){
                    int z = static_cast<SpriteLayer*>(l)->FindLastZOrder();
                    if(settings.GetZOrderStep()>0){
                        zOrder = std::max(zOrder, z);
                    }else{
                        zOrder = std::min(zOrder, z);
                    }
                }
            }
        }
    }

    return zOrder;
}


//==================================================================================================


VectorLayer::VectorLayer()
{
    _SetKind(LayerKind::VECTOR);
}


VectorLayer::~VectorLayer()
{
    for(VectorShape* vs : vectorShapes) delete vs;
    vectorShapes.clear();
}


void VectorLayer::InitLayerParameters()
{

    if(GetMap()==nullptr){          // For map layers this was already called.
        InitLayerParameters();
    }

    Layer::InitLayerParameters();
}


void VectorLayer::UpdateEngineObjects()
{

    if(GetMap()){
        if(GetMap()->GetType()==MapType::PARALLAX){

            //if(GetParallaxLayerMode()==ParallaxLayerMode::NO_CHANGE ||
            //         GetParallaxLayerMode()==ParallaxLayerMode::TILING_X ||
            //         GetParallaxLayerMode()==ParallaxLayerMode::TILING_Y ||
            //         GetParallaxLayerMode()==ParallaxLayerMode::TILING_XY )
            //{

            //if(GetParallaxLayerMode()==ParallaxLayerMode::STANDARD){
            if(GetLayerType()==LayerType::PARALLAX){
                UpdateParallaxOffset();
            }
        }
    }
}



void VectorLayer::UpdateBoundingBox()
{

    boundingBox = Rectf(999999.0, 999999.0, -999999.0, -999999.0);               // initial box

    for(VectorShape* vs : vectorShapes){

        if(vs->GetKind()==ShapeKind::POLYLINE){
            PolyLineShape *poly = static_cast<PolyLineShape*>(vs->GetGeometricShape());
            for(Vec2f &v : poly->vertices){
                boundingBox = boundingBox.Expand(v);
            }

        }else if(vs->GetKind()==ShapeKind::BEZIER_POLYCURVE){
            BezierShape *bezierPath = static_cast<BezierShape*>(vs->GetGeometricShape());
            for(BezierVertex &bv : bezierPath->vertices){
                boundingBox = boundingBox.Expand(bv.P);
            }

        }else if(vs->GetKind()==ShapeKind::ELLIPSE){
            EllipseShape *ellipse = static_cast<EllipseShape*>(vs->GetGeometricShape());
            boundingBox = boundingBox.Expand(Vec2f(ellipse->center.x-ellipse->a, ellipse->center.y-ellipse->b));
            boundingBox = boundingBox.Expand(Vec2f(ellipse->center.x+ellipse->a, ellipse->center.y+ellipse->b));

        }else if(vs->GetKind()==ShapeKind::SINGLE_POINT){
            SinglePointShape *sps = static_cast<SinglePointShape*>(vs->GetGeometricShape());
            boundingBox = boundingBox.Expand(sps->point);
        }
    }


}



}




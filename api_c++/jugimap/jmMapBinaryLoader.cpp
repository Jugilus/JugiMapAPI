#include <assert.h>
#include <utility>
#include <memory>
#include "jmCommonFunctions.h"
#include "jmSourceGraphics.h"
#include "jmMap.h"
#include "jmLayers.h"
#include "jmSprites.h"
#include "jmVectorShapes.h"
#include "jmVectorShapesUtilities.h"
#include "jmFrameAnimation.h"
#include "jmTimelineAnimation.h"
#include "jmObjectFactory.h"
#include "jmStreams.h"
#include "jmMapBinaryLoader.h"


using namespace std;


namespace jugimap{



//====================================================================================================

std::string JugiMapBinaryLoader::pathPrefix = "";
std::string JugiMapBinaryLoader::error = "";
std::vector<std::string> JugiMapBinaryLoader::messages;
//bool JugiMapBinaryLoader::usePixelCoordinatesForTileLayerSprites = true;
int JugiMapBinaryLoader::zOrderCounterStart = 1000;
int JugiMapBinaryLoader::zOrderCounter = 1000;         // starting depth value (the lowest layer)
//int JugiMapBinaryLoader::zOrderCounterStep = 10;





bool JugiMapBinaryLoader::LoadSourceGraphics(string _filePath, SourceGraphics *_sourceGraphics)
{

    if(objectFactory==nullptr) objectFactory = &genericObjectFactory;

    _filePath = pathPrefix + _filePath;



    std::unique_ptr<BinaryStreamReader>stream(objectFactory->NewBinaryFileStreamReader(_filePath));
    /*
    std::ifstream fs;
    int length = 0;
    fs.open(_filePath, ios::binary);

    if(fs.is_open()==false){
        return false;
    }else{
        fs.seekg(0, ios::end);
        length = fs.tellg();
        fs.seekg(0, ios::beg);
    }
    char *buff = new char[length];
    fs.read(buff, length);
    fs.close();
    BinaryBufferReader *stream = new BinaryBufferReader((unsigned char*)buff, length);
    */

    if(stream->IsOpen()==false){
        error = "Can not open file: " + _filePath;
        return false;
    }

    //--- format signature
    int signature = stream->ReadInt();
    if(signature!=SaveSignature::EXPORTED_SOURCE_GRAPHICS_FORMAT){
        error = string("Wrong format!");
        return false;
    }

    int version = stream->ReadInt();
    if(version!=SaveSignature::EXPORTED_SOURCE_GRAPHICS_FORMAT_VERSION){
        error = string("Wrong format version!");
        return false;
    }

    //---

    JugiMapBinaryLoader loader;
    loader.sourceGraphics = _sourceGraphics;
    //loader.indexBeginImageFiles = _indexBeginImageFiles;
    //loader.indexBeginSourceSprites = _indexBeginSourceSprites;
    loader.map = nullptr;



    int posStart = stream->Pos();
    int sizeCounter = stream->Size() - posStart;
    while(sizeCounter>0){
        int signature = stream->ReadInt();
        int sizeChunk = stream->ReadInt();
        if(sizeChunk>sizeCounter){
            error = "Load error! ChunkSize error for signature: " + std::to_string(signature);
            break;
        }
        int posChunk = stream->Pos();

        if(signature==SaveSignature::SOURCE_SETS){
            if(loader._LoadSourceGraphics(*stream.get(), sizeChunk)==false){
            //if(loader._LoadSourceGraphics(*stream, sizeChunk)==false){
                if(error=="") error = "LoadSourceGraphics error!";
                break;
            }

        }else{
            //cout << "Load - unknown signature:"<< signature;
            stream->Seek(posChunk+sizeChunk);
        }

        //---
        int dSize = stream->Pos()-posChunk;
        if(dSize!=sizeChunk){
            error = "Load error! Wrong chunkSize for signature:" + std::to_string(signature);
            break;
        }
        sizeCounter-=(dSize+8);   //8 for signature and chunk size
    }


    if(error!=""){
        return false;
    }

    return true;

}


bool JugiMapBinaryLoader::LoadMap(std::string _filePath, Map* _map, SourceGraphics *_sourceGraphics)
{

    if(objectFactory==nullptr) objectFactory = &genericObjectFactory;

    Reset();

    _filePath = pathPrefix + _filePath;


    std::unique_ptr<BinaryStreamReader>stream(objectFactory->NewBinaryFileStreamReader(_filePath));

    if(stream->IsOpen()==false){
        error = "Can not open file: " + _filePath;
        return false;
    }

    //--- format signature
    int signature = stream->ReadInt();
    if(signature!=SaveSignature::EXPORTED_MAP_FORMAT){
        error = string("Wrong format!");
        return false;
    }

    int version = stream->ReadInt();
    if(version!=SaveSignature::EXPORTED_MAP_FORMAT_VERSION){
        error = string("Wrong format version!");
        return false;
    }

    //---

    JugiMapBinaryLoader loader;
    loader.sourceGraphics = _sourceGraphics;
    //loader.indexBeginImageFiles = _indexBeginImageFiles;
    //loader.indexBeginSourceSprites = _indexBeginSourceSprites;
    if(objectFactory==nullptr) objectFactory = &genericObjectFactory;
    loader.map = _map;
    JugiMapBinaryLoader::zOrderCounterStart = JugiMapBinaryLoader::zOrderCounter = _map->GetZOrderStart();


    //----
    loader.map->name = _filePath;
    size_t indexLastSlash = loader.map->name.find_last_of("\\/");
    if (std::string::npos != indexLastSlash) loader.map->name.erase(0, indexLastSlash+1);
    size_t indexPeriod = loader.map->name.rfind('.');
    if (std::string::npos != indexPeriod) loader.map->name.erase(indexPeriod);

    //----
    int posStart = stream->Pos();
    int sizeCounter = stream->Size() - posStart;
    while(sizeCounter>0){
        int signature = stream->ReadInt();
        int sizeChunk = stream->ReadInt();
        if(sizeChunk>sizeCounter){
            error = "Load error! ChunkSize error for signature: " + std::to_string(signature);
            break;
        }
        int posChunk = stream->Pos();

        if(signature==SaveSignature::MAP){
            if(loader._LoadMap(*stream.get(), sizeChunk)==false){
            //if(loader._LoadMap(*stream, sizeChunk)==false){
                if(error=="") error = "Load/LoadMap error!";
                break;
            }

        }else{
            //cout << "Load - unknown signature:"<< signature;
            stream->Seek(posChunk+sizeChunk);
        }

        //---
        int dSize = stream->Pos()-posChunk;
        if(dSize!=sizeChunk){
            error = "Load error! Wrong chunkSize for signature:" + std::to_string(signature);
            break;
        }
        sizeCounter-=(dSize+8);   //8 for signature and chunk size
    }

    if(error!=""){
        return false;
    }

    //loader.AdjustMapWorldLayers();

    return true;
}


void JugiMapBinaryLoader::Reset()
{
    error = "";
    messages.clear();
    zOrderCounter = zOrderCounterStart;
}


bool JugiMapBinaryLoader::_LoadSourceGraphics(BinaryStreamReader &stream, int size)
{

    int posStart = stream.Pos();
    int sizeCounter = size;

    //---
    while(sizeCounter>0){
        int signature = stream.ReadInt();
        int sizeChunk = stream.ReadInt();
        if(sizeChunk>sizeCounter){
            error = "LoadSourceSets error! ChunkSize error for signature: " + std::to_string(signature);
            return false;
        }
        int posChunk = stream.Pos();

        if(signature==SaveSignature::SOURCE_IMAGE_FILES){

            int nGraphicFiles = stream.ReadInt();
            for(int i=0; i<nGraphicFiles; i++){

                int signature2 = stream.ReadInt();
                int sizeChunk2 = stream.ReadInt();
                int posChunk2 = stream.Pos();

                if(signature2==SaveSignature::SOURCE_IMAGE_FILE){

                    GraphicFile *gf = LoadGraphicFile(stream,sizeChunk2);
                    if(gf){
                        sourceGraphics->files.push_back(gf);
                    }else{
                        return false;
                    }

                }else{
                    messages.push_back("LoadSourceData - SOURCE_IMAGE_FILES - unknown signature: " + std::to_string(signature2));
                    stream.Seek(posChunk2+sizeChunk2);
                }
            }


        }else if(signature==SaveSignature::SOURCE_OBJECTS){

            int nSourceSprites = stream.ReadInt();
            for(int i=0; i<nSourceSprites; i++){

                int signature2 = stream.ReadInt();
                int sizeChunk2 = stream.ReadInt();
                int posChunk2 = stream.Pos();

                if(signature2==SaveSignature::SOURCE_OBJECT){
                    SourceSprite *ss = LoadSourceSprite(stream,sizeChunk2);
                    if(ss){
                        sourceGraphics->sourceSprites.push_back(ss);
                    }else{
                        return false;
                    }

                }else{
                    messages.push_back("LoadSourceData - SOURCE_OBJECTS - unknown signature: " + std::to_string(signature2));
                    stream.Seek(posChunk2+sizeChunk2);
                }
            }

        }else{
            messages.push_back("LoadSourceSets - unknown signature: " + std::to_string(signature));
            stream.Seek(posChunk+sizeChunk);
        }

        //---
        int dSize = stream.Pos()-posChunk;
        if(dSize!=sizeChunk){
            error = "LoadSourceSets error! Wrong chunkSize for signature:" + std::to_string(signature);
            return false;
        }
        sizeCounter-=(dSize+8);   //8 for two integers - 'signature' and 'size'
    }

    if(stream.Pos()-posStart!=size){
        error = "LoadSourceSets error! Wrong size: " + to_string(stream.Pos()-posStart) + "  saved: " + to_string(size);
        return false;
    }

    return true;
}


GraphicFile * JugiMapBinaryLoader::LoadGraphicFile(BinaryStreamReader &stream, int size)
{

    GraphicFile *gf = objectFactory->NewFile();


    int posStart = stream.Pos();
    int sizeCounter = size;

    //---
    while(sizeCounter>0){
        int signature = stream.ReadInt();
        int sizeChunk = stream.ReadInt();

        if(sizeChunk>sizeCounter){
            error = "LoadGraphicFile error! ChunkSize error for signature: " + std::to_string(signature);
            delete gf;
            return nullptr;
        }
        int posChunk = stream.Pos();

        if(signature==SaveSignature::VARIABLES){

            int fileKind = stream.ReadByte();
            if(fileKind==jmSINGLE_IMAGE){
                gf->kind = FileKind::SINGLE_IMAGE;

            }else if(fileKind==jmTILE_SHEET_IMAGE){
                gf->kind = FileKind::TILE_SHEET_IMAGE;

            }else if(fileKind==jmSPRITE_SHEET_IMAGE){
                gf->kind = FileKind::SPRITE_SHEET_IMAGE;

            }else if(fileKind==jmSPINE_FILE){
                gf->kind = FileKind::SPINE_FILE;

            }else if(fileKind==jmSPRITER_FILE){
                gf->kind = FileKind::SPRITER_FILE;
            }

            //gf->kind = stream.ReadByte();
            gf->relativeFilePath = stream.ReadString();
            gf->size.x = stream.ReadInt();
            gf->size.y = stream.ReadInt();
            gf->spineAtlasRelativeFilePath = stream.ReadString();


        }else if(signature==SaveSignature::SOURCE_IMAGES){

            int nItems = stream.ReadInt();
            for(int i=0; i<nItems; i++){
                GraphicItem *im = objectFactory->NewItem();
                im->graphicFile = gf;
                im->name = stream.ReadString();
                im->rect.min.x = stream.ReadInt();
                im->rect.min.y = stream.ReadInt();
                im->rect.max.x = im->rect.min.x + stream.ReadInt();     // width
                im->rect.max.y = im->rect.min.y + stream.ReadInt();     // height
                im->handle.x = stream.ReadInt();
                im->handle.y = stream.ReadInt();
                im->spineScale = stream.ReadFloat();

                if(settings.IsYCoordinateUp()){
                    im->handle.y = im->rect.Height() - im->handle.y;
                }

                int nCollisionShapes = stream.ReadInt();
                for(int k=0; k<nCollisionShapes; k++){

                    int signature2 = stream.ReadInt();
                    int sizeChunk2 = stream.ReadInt();
                    int posChunk2 = stream.Pos();
                    if(signature2==SaveSignature::VECTOR_SHAPE){
                        VectorShape *vs = LoadVectorShape(stream, sizeChunk2, false);
                        if(vs){
                            if(vs->probe){
                                im->probeShapes.push_back(vs);
                            }else{
                                // check out if shape geometry is ok for using in collision detection
                                bool skip = false;
                                if(vs->GetKind()==ShapeKind::POLYLINE){
                                    PolyLineShape* poly = static_cast<PolyLineShape*>(vs->GetGeometricShape());
                                    if(poly->vertices.size()<3){
                                        skip = true;
                                    }
                                    if(skip==false && poly->rectangle==false){
                                        // set 'convex' flag and make sure vertices order is CCW - both is required by 'box2d' library
                                        bool CW = false;
                                        poly->convex = IsPolygonConvex(poly->vertices,CW);
                                        if(poly->convex && CW){
                                            std::reverse(poly->vertices.begin(), poly->vertices.end());
                                        }
                                    }

                                }else if(vs->GetKind()==ShapeKind::ELLIPSE){
                                    // must be circle
                                    if(AreEqual(static_cast<EllipseShape*>(vs->GetGeometricShape())->a, static_cast<EllipseShape*>(vs->GetGeometricShape())->b)==false){
                                        skip = true;
                                    }
                                }else{
                                    skip = true;
                                }
                                if(skip==false){
                                    im->spriteShapes.push_back(vs);
                                }else{
                                    delete vs;
                                }

                            }
                            //im->collisionShapes.push_back(vs);
                        }else{
                            delete im;
                            delete gf;
                            return nullptr;
                        }

                    }else{
                        messages.push_back("LoadGraphicFile - VECTOR_SHAPE - unknown signature: " + std::to_string(signature2));
                        stream.Seek(posChunk2+sizeChunk2);
                    }
                }
                InitCollisionShapes(im);

                //----
                gf->items.push_back(im);
            }

        }else{
            messages.push_back("LoadGraphicFile - unknown signature: " + std::to_string(signature));
            stream.Seek(posChunk+sizeChunk);
        }

        //---
        int dSize = stream.Pos()-posChunk;
        if(dSize!=sizeChunk){
            error = "LoadGraphicFile error! Wrong chunkSize for signature:" + std::to_string(signature);
            delete gf;
            return nullptr;
        }
        sizeCounter-=(dSize+8);   //8 for two integers - 'signature' and 'size'
    }

    if(stream.Pos()-posStart!=size){
        error = "LoadGraphicFile error! Wrong size: " + to_string(stream.Pos()-posStart) + "  saved: " + to_string(size);
        delete gf;
        return nullptr;
    }

    return gf;
}


SourceSprite * JugiMapBinaryLoader::LoadSourceSprite(BinaryStreamReader &stream, int size)
{

    SourceSprite *ss = objectFactory->NewSourceSprite();


    int posStart = stream.Pos();
    int sizeCounter = size;

    //---
    while(sizeCounter>0){
        int signature = stream.ReadInt();
        int sizeChunk = stream.ReadInt();

        if(sizeChunk>sizeCounter){
            error = "LoadSourceSprite error! ChunkSize error for signature: " + std::to_string(signature);
            delete ss;
            return nullptr;
        }
        int posChunk = stream.Pos();

        if(signature==SaveSignature::VARIABLES){

            ss->name = stream.ReadString();


        }else if(signature==SaveSignature::SOURCE_IMAGES){

            int nItems = stream.ReadInt();
            for(int i=0; i<nItems; i++){
                int indexGraphicFile = stream.ReadInt();
                indexGraphicFile += indexBeginImageFiles;

                int indexGraphicItem = stream.ReadInt();

                if(indexGraphicFile==saveID_DUMMY_SIF){
                    if(indexGraphicItem==saveID_DUMMY_COMPOSED_SPRITE){
                        ss->kind = SpriteKind::COMPOSED;
                    }

                }else if(indexGraphicFile>=0 && indexGraphicFile<sourceGraphics->files.size()){

                    GraphicFile *sif = sourceGraphics->files[indexGraphicFile];

                    if(indexGraphicItem>=0 && indexGraphicItem<sif->items.size()){

                        ss->graphicItems.push_back(sif->items[indexGraphicItem]);

                        if(sif->kind==FileKind::SPINE_FILE){
                            ss->kind = SpriteKind::SPINE;

                        }else if(sif->kind==FileKind::SPRITER_FILE){
                            ss->kind = SpriteKind::SPRITER;

                        }else{
                            ss->kind = SpriteKind::STANDARD;
                        }

                    }else{
                        error = "LoadSourceSprite: "+ss->name+"  indexSourceImage: " + to_string(indexGraphicItem) + " out of range!";
                        assert(false);
                        delete ss;
                        return nullptr;
                    }
                }else{
                    error = "LoadSourceSprite: "+ss->name+" indexSourceImageFile: " + to_string(indexGraphicFile) + " out of range!";
                    assert(false);
                    delete ss;
                    return nullptr;
                }
            }


        }else if(signature==SaveSignature::PARAMETERS){

            int nParameters = stream.ReadInt();
            for(int i=0; i<nParameters; i++){
                Parameter pv;
                pv.kind = stream.ReadByte();
                pv.name = stream.ReadString();
                //----
                ss->parametersTMP.push_back(pv);
            }


        }else if(signature==SaveSignature::CONSTANT_PARAMETERS){

            int nConstantParameters = stream.ReadInt();
            for(int i=0; i<nConstantParameters; i++){
                Parameter pv;
                pv.kind = stream.ReadByte();
                pv.name = stream.ReadString();
                pv.value = stream.ReadString();
                //----
                ss->constantParameters.push_back(pv);
            }

        }else if(signature==SaveSignature::COMPOSED_SPRITE){

            ss->sourceComposedSprite = LoadComposedSpriteData(stream,sizeChunk);
            if(ss->sourceComposedSprite){
                // empty

            }else{
                delete ss;
                return nullptr;
            }

        }else if(signature==SaveSignature::FRAME_ANIMATIONS){

            int nFrameAnimations = stream.ReadInt();

            for(int i=0; i<nFrameAnimations; i++){
                int signature2 = stream.ReadInt();
                int sizeChunk2 = stream.ReadInt();
                int posChunk2 = stream.Pos();

                if(signature2==SaveSignature::FRAME_ANIMATION){
                    FrameAnimation *fa = LoadFrameAnimation(stream, sizeChunk2, ss);
                    if(fa){
                        ss->frameAnimations.push_back(fa);
                    }
                }else{
                    messages.push_back("LoadSourceSprite - FRAME_ANIMATIONS - unknown signature: " + std::to_string(signature2));
                    stream.Seek(posChunk2+sizeChunk2);
                }
            }

        }else if(signature==SaveSignature::TIMELINE_ANIMATIONS){

            int nAnimations = stream.ReadInt();

            for(int i=0; i<nAnimations; i++){
                int signature2 = stream.ReadInt();
                int sizeChunk2 = stream.ReadInt();
                int posChunk2 = stream.Pos();

                if(signature2==SaveSignature::TIMELINE_ANIMATION){
                    TimelineAnimation *ta = LoadTimelineAnimation(stream, sizeChunk2, ss);
                    if(ta){
                        ss->timelineAnimations.push_back(ta);
                    }
                }else{
                    messages.push_back("LoadSourceSprite - TIMELINE_ANIMATIONS - unknown signature: " + std::to_string(signature2));
                    stream.Seek(posChunk2+sizeChunk2);
                }
            }


        }else{
            messages.push_back("LoadSourceSprite - unknown signature: " + std::to_string(signature));
            stream.Seek(posChunk+sizeChunk);
        }

        //---
        int dSize = stream.Pos()-posChunk;
        if(dSize!=sizeChunk){
            error = "LoadSourceSprite error! Wrong chunkSize for signature:" + std::to_string(signature);
            delete ss;
            return nullptr;
        }
        sizeCounter-=(dSize+8);   //8 for two integers - 'signature' and 'size'
    }

    if(stream.Pos()-posStart!=size){
        error = "LoadSourceSprite error! Wrong size: " + to_string(stream.Pos()-posStart) + "  saved: " + to_string(size);
        delete ss;
        return nullptr;
    }

    return ss;

}


ComposedSprite *JugiMapBinaryLoader::LoadComposedSpriteData(BinaryStreamReader &stream, int size)
{

    ComposedSprite *cs = objectFactory->NewComposedSprite();
    loadedComposedSprite = cs;

    int posStart = stream.Pos();
    int sizeCounter = size;

    while(sizeCounter>0){
        int signature = stream.ReadInt();
        int sizeChunk = stream.ReadInt();
        if(sizeChunk>sizeCounter){
            error = "LoadComposedSprite error! ChunkSize error for signature: " + std::to_string(signature);
            delete cs;
            return nullptr;
        }
        int posChunk = stream.Pos();

        if(signature==SaveSignature::VARIABLES){
            cs->tileSize.x = stream.ReadInt();
            cs->tileSize.y = stream.ReadInt();
            cs->nTiles.x = stream.ReadInt();
            cs->nTiles.y = stream.ReadInt();
            cs->size.x = stream.ReadInt();
            cs->size.y = stream.ReadInt();
            cs->handle.x = stream.ReadInt();
            cs->handle.y = stream.ReadInt();

            if(settings.IsYCoordinateUp()){
                cs->handle.y = cs->size.y - cs->handle.y;
            }

        }else if(signature==SaveSignature::LAYERS){

            int nLayers = stream.ReadInt();
            for(int i=0; i<nLayers; i++){

                int childSignature = stream.ReadInt();
                int sizeChildChunk = stream.ReadInt();
                int posChildChunk = stream.Pos();

                if(childSignature==SaveSignature::LAYER){
                    SpriteLayer * layer = LoadLayer(stream, sizeChildChunk);
                    if(layer){
                        cs->layers.push_back(layer);
                    }else{
                        delete cs;
                        return nullptr;
                    }

                }else if(childSignature==SaveSignature::VECTOR_LAYER){
                    VectorLayer *vectorLayer = LoadVectorLayer(stream, sizeChildChunk);
                    if(vectorLayer){
                        cs->layers.push_back(vectorLayer);
                    }else{
                        delete cs;
                        return nullptr;
                    }

                }else{
                    messages.push_back("LoadComposedSprite - unknown signature: " + std::to_string(signature));

                    if(sizeChildChunk>sizeCounter){
                        error = "LoadComposedSprite child error! Chunk size error for unknown signature:" + std::to_string(childSignature);
                        delete cs;
                        return nullptr;
                    }
                    stream.Seek(posChildChunk+sizeChildChunk);
                }
            }


        }else{
            messages.push_back("LoadComposedSprite - unknown signature: " + std::to_string(signature));
            stream.Seek(posChunk+sizeChunk);
        }

        //---
        int dSize = stream.Pos()-posChunk;
        if(dSize!=sizeChunk){
            error = "LoadComposedSprite error! Wrong chunkSize for signature:" + std::to_string(signature);
            delete cs;
            return nullptr;
        }
        sizeCounter-=(dSize+8);   //8 za signature in size
    }

    if(stream.Pos()-posStart!=size){
        error = "LoadComposedSprite error! Wrong size: " + to_string(stream.Pos()-posStart) + "  saved: " + to_string(size);
        delete cs;
        return nullptr;
    }

    loadedComposedSprite = nullptr;
    return cs;

}


bool JugiMapBinaryLoader::_LoadMap(BinaryStreamReader &stream, int size)
{

    //loading = loadingMAP;

    int posStart = stream.Pos();
    int sizeCounter = size;

    while(sizeCounter>0){
        int signature = stream.ReadInt();
        int sizeChunk = stream.ReadInt();
        if(sizeChunk>sizeCounter){
            error = "LoadMap error! ChunkSize error for signature: " + std::to_string(signature);
            return false;
        }
        int posChunk = stream.Pos();

        if(signature==SaveSignature::VARIABLES){
            map->tileSize.x = stream.ReadInt();
            map->tileSize.y = stream.ReadInt();
            map->nTiles.x = stream.ReadInt();
            map->nTiles.y = stream.ReadInt();


        }else if(signature==SaveSignature::LAYERS){

            int nLayers = stream.ReadInt();
            for(int i=0; i<nLayers; i++){

                int childSignature = stream.ReadInt();
                int sizeChildChunk = stream.ReadInt();
                int posChildChunk = stream.Pos();

                if(childSignature==SaveSignature::LAYER){
                    SpriteLayer *layer = LoadLayer(stream, sizeChildChunk);
                    if(layer){
                        map->layers.push_back(layer);
                    }else{
                        return false;
                    }

                }else if(childSignature==SaveSignature::VECTOR_LAYER){
                    VectorLayer *vectorLayer = LoadVectorLayer(stream, sizeChildChunk);
                    if(vectorLayer){
                        //map->vectorLayers.push_back(vectorLayer);
                        map->layers.push_back(vectorLayer);
                    }else{
                        return false;
                    }

                    /*
                }else if(childSignature==SaveSignature::STATIC_LAYER){
                    SpriteLayer *sssl = LoadStaticSingleSpriteLayer(stream, sizeChildChunk);
                    if(sssl){
                        map->layers.push_back(sssl);
                    }else{
                        return false;
                    }
                    */

                }else{
                    messages.push_back("LoadMap - unknown signature: " + std::to_string(signature));

                    if(sizeChildChunk>sizeCounter){
                        error = "LoadMap board child error! Chunk size error for unknown signature:" + std::to_string(childSignature);
                        return false;
                    }
                    stream.Seek(posChildChunk+sizeChildChunk);
                }
            }

        }else if(signature==SaveSignature::PARAMETERS){

            LoadParameters(stream, map->parameters);


        }else{
            messages.push_back("LoadMap - unknown signature: " + std::to_string(signature));
            stream.Seek(posChunk+sizeChunk);
        }

        //---
        int dSize = stream.Pos()-posChunk;
        if(dSize!=sizeChunk){
            error = "LoadMap error! Wrong chunkSize for signature:" + std::to_string(signature);
            return false;
        }
        sizeCounter-=(dSize+8);   //8 za signature in size
    }

    if(stream.Pos()-posStart!=size){
        error = "LoadMap error! Wrong size: " + to_string(stream.Pos()-posStart) + "  saved: " + to_string(size);
        return false;
    }

    return true;
}


SpriteLayer *JugiMapBinaryLoader::LoadLayer(BinaryStreamReader &stream, int size)
{

    SpriteLayer *layer = objectFactory->NewSpriteLayer();
    if(map){
        layer->map = map;
        layer->zOrder = zOrderCounter;
        //zOrderCounter += zOrderCounterStep;
        zOrderCounter += settings.GetZOrderStep();
    }


    int posStart = stream.Pos();
    int sizeCounter = size;

    //---
    while(sizeCounter>0){
        int signature = stream.ReadInt();
        int sizeChunk = stream.ReadInt();

        if(sizeChunk>sizeCounter){
            error = "LoadLayer error! ChunkSize error for signature: " + std::to_string(signature);
            return nullptr;
        }
        int posChunk = stream.Pos();

        if(signature==SaveSignature::VARIABLES){

            layer->name = stream.ReadString();
            int layerKind = stream.ReadByte();     // tile layer or sprite layer
            if(layerKind==jmTILE_LAYER){
                //layer->kind = LayerKind::TILE;
                layer->editorTileLayer = true;

            }else if(layerKind==jmSPRITE_LAYER){
                layer->editorTileLayer = false;
            //    layer->kind = LayerKind::SPRITE;
            }
            layer->boundingBox.min.x = stream.ReadFloat();
            layer->boundingBox.min.y = stream.ReadFloat();
            layer->boundingBox.max.x = stream.ReadFloat();
            layer->boundingBox.max.y = stream.ReadFloat();

            if(settings.IsYCoordinateUp()){
                float yMin=0,yMax=0;
                if(loadedComposedSprite){                           // a child of loadedComposedSprite
                    yMin = loadedComposedSprite->size.y - layer->boundingBox.max.y;
                    yMax = loadedComposedSprite->size.y - layer->boundingBox.min.y;
                }else if(map){
                    yMin = map->nTiles.y * map->tileSize.y - layer->boundingBox.max.y;
                    yMax = map->nTiles.y * map->tileSize.y - layer->boundingBox.min.y;
                }else{
                    assert(false);
                }
                layer->boundingBox.min.y = yMin;
                layer->boundingBox.max.y = yMax;
            }


        }else if(signature==SaveSignature::OBJECTS){

            int nSprites = stream.ReadInt();
            for(int i=0; i<nSprites; i++){

                int indexSourceSprite = stream.ReadInt();
                indexSourceSprite += indexBeginSourceSprites;

                if(indexSourceSprite<0 || indexSourceSprite>=sourceGraphics->sourceSprites.size()){
                    error = "LoadLayer indexSourceObject: " + to_string(indexSourceSprite) + " out of range!";
                    assert(false);
                    delete layer;
                    return nullptr;
                }
                SourceSprite *ss = sourceGraphics->sourceSprites[indexSourceSprite];


                Sprite *s = objectFactory->NewSprite(ss->kind);
                s->sourceSprite = ss;
                //s->map = map;
                s->layer = layer;
                s->parentComposedSprite = layer->parentComposedSprite;

                //---
                if(LoadSpriteData(stream, s)==false){
                    assert(false);
                    delete s;
                    delete layer;
                    return nullptr;
                }

                //if(usePixelCoordinatesForTileLayerSprites && layer->editorTileLayer){
                if(layer->editorTileLayer){
                    if(loadedComposedSprite){
                        s->position.x = s->position.x*loadedComposedSprite->tileSize.x + loadedComposedSprite->tileSize.x/2.0;
                        s->position.y = s->position.y*loadedComposedSprite->tileSize.y + loadedComposedSprite->tileSize.y/2.0;
                    }else if(map){
                        s->position.x = s->position.x*map->tileSize.x + map->tileSize.x/2.0;
                        s->position.y = s->position.y*map->tileSize.y + map->tileSize.y/2.0;
                    }
                }

                if(settings.IsYCoordinateUp()){
                    if(loadedComposedSprite){   // sprite is child of loadedComposedSprite
                        //s->pos.y = loadedComposedSprite->nTiles.y * loadedComposedSprite->tileSize.y - s->pos.y;
                        s->position.y = loadedComposedSprite->size.y - s->position.y;
                    }else if(map){
                        s->position.y = map->nTiles.y * map->tileSize.y - s->position.y;
                    }
                    s->rotation = -s->rotation;
                }


                //----
                if(ss->kind==SpriteKind::COMPOSED){
                    assert(ss->sourceComposedSprite);
                    if(ss->sourceComposedSprite==nullptr){
                        error = "LoadLayer error! Missing SourceComposedSprite for source sprite: " + ss->name;
                        delete layer;
                        return nullptr;
                    }

                    s->handle = ss->sourceComposedSprite->handle;
                    //if(configuration.IsYCoordinateUp()){
                    //    s->handle.y = ss->SourceComposedSprite->size.y - s->handle.y;
                    //}
                    if(map){
                        if(ss->name=="rotatorA"){
                            DummyFunction();
                        }
                    }

                    JugiMapBinaryLoader* loader = (map!=nullptr)? this : nullptr;
                    //ComposedSprite::CopyLayers(ss->sourceComposedSprite, static_cast<ComposedSprite*>(s), loader);
                    int zOrder = layer->zOrder;
                    if(map){
                        DummyFunction();
                    }
                    ComposedSprite::CopyLayers(ss->sourceComposedSprite, static_cast<ComposedSprite*>(s), zOrder);

                    if(map){
                        if(settings.GetZOrderStep()>0){
                            zOrderCounter = std::max(zOrderCounter, zOrder);     // !
                        }else{
                            zOrderCounter = std::min(zOrderCounter, zOrder);
                        }
                    }
                }

                //---
                layer->sprites.push_back(s);
            }

        }else if(signature==SaveSignature::PARAMETERS){
            LoadParameters(stream, layer->parameters);

        }else if(signature==SaveSignature::PARALLAX_LAYER_PARAMETERS){

            LoadParallaxAndScreenLayerParameters(stream, layer);

            /*

            layer->attachToLayer = stream.ReadString();

            int layerType = stream.ReadInt();
            if(layerType==typePARALLAX_LAYER){
                layer->layerType = LayerType::PARALLAX;

            }else if(layerType==typePARALLAX_STRETCHING_SINGLE_SPRITE){
                layer->layerType = LayerType::PARALLAX_STRETCHING_SINGLE_SPRITE;

            }else if(layerType==typeSCREEN_LAYER){
                layer->layerType = LayerType::SCREEN;

            }else if(layerType==typeSCREEN_STRETCHING_SINGLE_SPRITE){
                layer->layerType = LayerType::SCREEN_STRETCHING_SINGLE_SPRITE;
            }


            layer->parallaxFactor.x = stream.ReadFloat();
            layer->parallaxFactor.y = stream.ReadFloat();

            layer->alignPosition.x = stream.ReadInt();
            layer->alignPosition.y = stream.ReadInt();

            layer->alignOffset.x = stream.ReadInt();
            layer->alignOffset.y = stream.ReadInt();

            layer->alignOffset_obtainFromMap.x = stream.ReadByte();
            layer->alignOffset_obtainFromMap.y = stream.ReadByte();

            layer->tilingCount.x = stream.ReadInt();
            layer->tilingCount.y = stream.ReadInt();

            layer->tilingCountAutoSpread.x = stream.ReadByte();
            layer->tilingCountAutoSpread.y = stream.ReadByte();

            layer->tilingSpacing.x = stream.ReadInt();
            layer->tilingSpacing.y = stream.ReadInt();

            layer->tilingSpacingDelta.x = stream.ReadInt();
            layer->tilingSpacingDelta.y = stream.ReadInt();


            int stretching = stream.ReadInt();
            if(stretching==stretchXY_WORLD_SIZE){
                layer->stretchingVariant = StretchingVariant::XY_TO_WORLD_SIZE;

            }else if(stretching==stretchXY_VIEWPORT_SIZE){
                layer->stretchingVariant = StretchingVariant::XY_TO_VIEWPORT_SIZE;
            }

            */


        }else{
            messages.push_back("LoadLayer - unknown signature:" + std::to_string(signature));
            stream.Seek(posChunk+sizeChunk);
        }

        //---
        int dSize = stream.Pos()-posChunk;
        if(dSize!=sizeChunk){
            error = "LoadLayer error! Wrong chunkSize for signature:" + std::to_string(signature);
            delete layer;
            return nullptr;
        }
        sizeCounter-=(dSize+8);   //8 for two integers - 'signature' and 'size'
    }

    if(stream.Pos()-posStart!=size){
        error = "LoadLayer error! Wrong size: " + to_string(stream.Pos()-posStart) + "  saved: " + to_string(size);
        delete layer;
        return nullptr;
    }

    if(map){
        DbgOutput("Map Layer:"+layer->GetName() + " zOrder:" + std::to_string(layer->zOrder));
    }

    return layer;

}


ColorOverlayBlend GetColorOverlayBlendFromInt(int _value)
{

    const int blendSIMPLE_MULTIPLY = 0;
    const int blendNORMAL = 1;
    const int blendMULTIPLY = 2;
    const int blendLINEAR_DODGE = 3;
    const int blendCOLOR = 4;

    if(_value==blendSIMPLE_MULTIPLY){
        return ColorOverlayBlend::SIMPLE_MULTIPLY;

    }else if(_value==blendNORMAL){
        return ColorOverlayBlend::NORMAL;

    }else if(_value==blendMULTIPLY){
        return ColorOverlayBlend::MULTIPLY;

    }else if(_value==blendLINEAR_DODGE){
        return ColorOverlayBlend::LINEAR_DODGE;

    }else if(_value==blendCOLOR){
        return ColorOverlayBlend::COLOR;
    }

    return ColorOverlayBlend::NOT_DEFINED;

}


bool JugiMapBinaryLoader::LoadSpriteData(BinaryStreamReader &stream, Sprite *s)
{

    s->position.x = stream.ReadInt();
    s->position.y = stream.ReadInt();

    /*
    if(usePixelCoordinatesForTileLayerSprites && layer->editorTileLayer){
        if(loadedComposedSprite){
            s->position.x = s->position.x*loadedComposedSprite->tileSize.x + loadedComposedSprite->tileSize.x/2.0;
            s->position.y = s->position.y*loadedComposedSprite->tileSize.y + loadedComposedSprite->tileSize.y/2.0;
        }else if(map){
            s->position.x = s->position.x*map->tileSize.x + map->tileSize.x/2.0;
            s->position.y = s->position.y*map->tileSize.y + map->tileSize.y/2.0;
        }
    }
    */



    //---- PROPERTIES
    /*
    const int flagXflip = 1;
    const int flagYflip = 2;
    const int flagUniformScale = 4;
    const int flagSavedXscale = 8;
    const int flagSavedYscale = 16;
    const int flagSavedRotation = 32;
    const int flagSavedDataFlag = 64;
    const int flagSavedXdelta = 128;
    const int flagSavedYdelta = 256;
    const int flagSavedAlpha = 512;
    const int flagSavedExtraProperty_ColorBlend = 1024;
    */

    const int saveFLIP_X =          1 << 0;
    const int saveFLIP_Y =          1 << 1;
    const int saveSCALE_UNIFORM =   1 << 2;
    const int saveSCALE_X =         1 << 3;
    const int saveSCALE_Y =         1 << 4;
    const int saveROTATION =        1 << 5;
    //const int flagSavedXPropAni = 1 << 7;
    //const int flagSavedYPropAni = 1 << 8;
    const int saveALPHA =           1 << 6;
    const int saveOVERLAY_COLOR =   1 << 7;

    const int saveID =              1 << 10;
    const int saveNAME_ID =         1 << 11;
    const int saveDATA_FLAG =       1 << 12;

    const int saved = stream.ReadInt();

    if(saved & saveFLIP_X) s->flip.x = true;
    if(saved & saveFLIP_Y) s->flip.y = true;
    if(saved & saveSCALE_X) s->scale.x = stream.ReadFloat();
    if(saved & saveSCALE_Y) s->scale.y = stream.ReadFloat();
    if(saved & saveSCALE_UNIFORM) s->scale.y = s->scale.x;
    if(saved & saveROTATION) s->rotation = stream.ReadFloat();
    //if(transformSaveFlags & flagSavedXdelta) s->animationFrameOffset.x = stream.ReadFloat();
    //if(transformSaveFlags & flagSavedYdelta) s->animationFrameOffset.y = stream.ReadFloat();
    if(saved & saveALPHA) s->alpha = stream.ReadFloat();

    if(saved & saveOVERLAY_COLOR){
        s->colorOverlayActive = true;
        s->colorOverlayRGBA = ColorRGBA(stream.ReadInt());
        int colorBlendMode = stream.ReadInt();

        /*
        const int blendSIMPLE_MULTIPLY = 0;
        const int blendNORMAL = 1;
        const int blendMULTIPLY = 2;
        const int blendLINEAR_DODGE = 3;
        const int blendCOLOR = 4;

        if(colorBlendMode==blendSIMPLE_MULTIPLY){
            s->colorOverlayBlend = ColorOverlayBlend::SIMPLE_MULTIPLY;

        }else if(colorBlendMode==blendNORMAL){
            s->colorOverlayBlend = ColorOverlayBlend::NORMAL;

        }else if(colorBlendMode==blendMULTIPLY){
            s->colorOverlayBlend = ColorOverlayBlend::MULTIPLY;

        }else if(colorBlendMode==blendLINEAR_DODGE){
            s->colorOverlayBlend = ColorOverlayBlend::LINEAR_DODGE;

        }else if(colorBlendMode==blendCOLOR){
            s->colorOverlayBlend = ColorOverlayBlend::COLOR;
        }
        */
        s->colorOverlayBlend = GetColorOverlayBlendFromInt(colorBlendMode);


    }

    if(saved & saveID) s->id = stream.ReadInt();
    if(saved & saveNAME_ID) s->name = stream.ReadString();
    if(saved & saveDATA_FLAG) s->dataFlags = stream.ReadInt();

    if(s->name!=""){
        DummyFunction();
    }

    /*
    const int saveFLIP_X = 1 << 0;
    const int saveFLIP_Y = 1 << 1;
    const int saveSCALE_UNIFORM = 1 << 2;
    const int saveSCALE_X = 1 << 3;
    const int saveSCALE_Y = 1 << 4;
    const int saveROTATION = 1 << 5;
    //const int flagSavedXPropAni = 1 << 7;
    //const int flagSavedYPropAni = 1 << 8;
    const int saveALPHA = 1 << 6;
    const int saveOVERLAY_COLOR = 1 << 7;

    const int saveID = 1 << 10;
    const int saveNAME_ID = 1 << 11;
    const int saveDATA_FLAG = 1 << 12;


    int save = 0;
    if(xFlip) save |= saveFLIP_X;
    if(yFlip) save |= saveFLIP_Y;
    if(uniformScale) save |= saveSCALE_UNIFORM;
    if(AreSame(xScale, 1.0)==false) save |= saveSCALE_X;
    if(AreSame(yScale, 1.0)==false && uniformScale==false) save |= saveSCALE_Y;
    if(AreSame(rotation, 0.0)==false) save |= saveROTATION;
    //if(AreSame(posPropAni.x, 0.0)==false) propertySaveFlags |= flagSavedXPropAni;
    //if(AreSame(posPropAni.y, 0.0)==false) propertySaveFlags |= flagSavedYPropAni;
    if(AreSame(alpha, 1.0)==false) save |= saveALPHA;
    if(extraProperties.GetKind()==TShaderBasedProperties::kindOVERLAY_COLOR) save |= saveOVERLAY_COLOR;

    if(id!=0) save |= saveID;
    if(nameID.isEmpty()==false) save |= saveNAME_ID;
    if(dataFlags!=0) save |= saveDATA_FLAG;


    Stream.WriteInt(save);
    if(save & saveSCALE_X) Stream.WriteFloat(xScale);
    if(save & saveSCALE_Y) Stream.WriteFloat(yScale);
    if(save & saveROTATION) Stream.WriteFloat(rotation);
    //if(propertySaveFlags & flagSavedXPropAni) Stream.WriteFloat(posPropAni.x);
    //if(propertySaveFlags & flagSavedYPropAni) Stream.WriteFloat(posPropAni.y);
    if(save & saveALPHA) Stream.WriteFloat(alpha);
    if(save & saveOVERLAY_COLOR){
        Stream.WriteInt(extraProperties.colorRGBA.GetRGBA());
        Stream.WriteInt(extraProperties.blend);
    }
    if(save & saveID) Stream.WriteInt(id);
    if(save & saveNAME_ID) Stream.WriteLatin1StringWithLength(nameID);
    if(save & saveDATA_FLAG) Stream.WriteInt(dataFlags);
    */


    /*
    if(settings.IsYCoordinateUp()){
        if(loadedComposedSprite){   // sprite is child of loadedComposedSprite
            //s->pos.y = loadedComposedSprite->nTiles.y * loadedComposedSprite->tileSize.y - s->pos.y;
            s->position.y = loadedComposedSprite->size.y - s->position.y;
        }else if(map){
            s->position.y = map->nTiles.y * map->tileSize.y - s->position.y;
        }
        s->rotation = -s->rotation;
    }
    */

    // sprite parameters
    int nParameters = stream.ReadInt();
    if(nParameters != s->sourceSprite->parametersTMP.size()){
        error = "LoadLayer nParameters:" + to_string(nParameters) + " different then for the source object: " + to_string(s->sourceSprite->parametersTMP.size());
        //assert(false);
        //delete s;
        //delete layer;
        //return nullptr;
        return false;
    }

    for(int i=0; i<nParameters; i++){

        Parameter &pvSource = s->sourceSprite->parametersTMP[i];
        Parameter pv;

        pv.name = pvSource.name;
        pv.kind = pvSource.kind;

        bool pvActive = (bool)stream.ReadByte();

        if(pv.kind==jmINTEGER){
            int valueInt = stream.ReadInt();
            pv.value = to_string(valueInt);

        }else if(pv.kind==jmFLOAT){
            float valueFloat = stream.ReadFloat();
            pv.value = to_string(valueFloat);

        }else if(pv.kind==jmBOOLEAN){
            //empty

        }else if(pv.kind==jmSTRING){
            pv.value = stream.ReadString();

        }else if(pv.kind==jmVALUES_SET){
            pv.value = stream.ReadString();
        }

        if(pvActive){
            s->parameters.push_back(pv);
        }
    }


    return true;

}


VectorLayer* JugiMapBinaryLoader::LoadVectorLayer(BinaryStreamReader &stream, int size)
{

    VectorLayer *vl = objectFactory->NewVectorLayer();
    vl->kind = LayerKind::VECTOR;
    if(map){
        vl->map = map;
        vl->zOrder = zOrderCounter;
        zOrderCounter += settings.GetZOrderStep();
        //zOrderCounter += zOrderCounterStep;
    }


    int posStart = stream.Pos();
    int sizeCounter = size;

    //---
    while(sizeCounter>0){
        int signature = stream.ReadInt();
        int sizeChunk = stream.ReadInt();

        if(sizeChunk>sizeCounter){
            error = "LoadVectorLayer error! ChunkSize error for signature: " + std::to_string(signature);
            delete vl;
            return nullptr;
        }
        int posChunk = stream.Pos();

        if(signature==SaveSignature::VARIABLES){

            vl->name = stream.ReadString();

        }else if(signature==SaveSignature::VECTOR_SHAPES_V2){

            int nVectorShapes = stream.ReadInt();
            for(int i=0; i<nVectorShapes; i++){

                int signature2 = stream.ReadInt();
                int sizeChunk2 = stream.ReadInt();
                int posChunk2 = stream.Pos();
                if(signature2==SaveSignature::VECTOR_SHAPE){
                    VectorShape *vs = LoadVectorShape(stream, sizeChunk2);
                    if(vs){
                        vs->RebuildPath();
                        vl->vectorShapes.push_back(vs);
                    }else{
                        delete vl;
                        return nullptr;
                    }

                }else{
                    messages.push_back("LoadVectorLayer - VECTOR_SHAPE - unknown signature: " + std::to_string(signature2));
                    stream.Seek(posChunk2+sizeChunk2);
                }
            }

        }else if(signature==SaveSignature::PARAMETERS){
            LoadParameters(stream, vl->parameters);

        }else if(signature==SaveSignature::PARALLAX_LAYER_PARAMETERS){
            LoadParallaxAndScreenLayerParameters(stream, vl);

        }else{
            messages.push_back("LoadVectorLayer - unknown signature: " + std::to_string(signature));
            stream.Seek(posChunk+sizeChunk);
        }

        //---
        int dSize = stream.Pos()-posChunk;
        if(dSize!=sizeChunk){
            error = "LoadVectorLayer error! Wrong chunkSize for signature:" +  std::to_string(signature);
            delete vl;
            return nullptr;
        }
        sizeCounter-=(dSize+8);   //8 for two integers - 'signature' and 'size'
    }

    if(stream.Pos()-posStart!=size){
        error = "LoadVectorLayer error! Wrong size: " + to_string(stream.Pos()-posStart) + "  saved: " + to_string(size);
        delete vl;
        return nullptr;
    }

    return vl;

}


VectorShape* JugiMapBinaryLoader::LoadVectorShape(BinaryStreamReader &stream, int size, bool vectorLayerShape)
{

    FVectorShape fvs;

    int posStart = stream.Pos();
    int sizeCounter = size;

    //---
    while(sizeCounter>0){
        int signature = stream.ReadInt();
        int sizeChunk = stream.ReadInt();

        if(sizeChunk>sizeCounter){
            error = "LoadVectorShape error! ChunkSize error for signature: " + std::to_string(signature);
            return nullptr;
        }
        int posChunk = stream.Pos();

        if(signature==SaveSignature::VARIABLES){

            fvs.kind = stream.ReadByte();
            fvs.closed = stream.ReadByte();
            fvs.name = stream.ReadString();
            fvs.id = stream.ReadInt();
            fvs.dataFlags = stream.ReadInt();
            fvs.probe = stream.ReadByte();

        }else if(signature==SaveSignature::VECTOR_SHAPE_CONTROL_POINTS){

            int nVertices = stream.ReadInt();
            for(int i=0; i<nVertices; i++){
                FVectorVertex V;
                V.x = stream.ReadInt();
                V.y = stream.ReadInt();
                if(vectorLayerShape && settings.IsYCoordinateUp()){
                    V.y = map->nTiles.y*map->tileSize.y - V.y;
                }
                if(fvs.kind==jmBEZIER_POLYCURVE){
                    V.xBPprevious = stream.ReadFloat();
                    V.yBPprevious = stream.ReadFloat();
                    V.xBPnext = stream.ReadFloat();
                    V.yBPnext = stream.ReadFloat();

                    if(vectorLayerShape && settings.IsYCoordinateUp()){
                        V.yBPprevious = map->nTiles.y*map->tileSize.y - V.yBPprevious;
                        V.yBPnext = map->nTiles.y*map->tileSize.y - V.yBPnext;
                    }
                }
                fvs.vertices.push_back(V);
            }

        }else if(signature==SaveSignature::PARAMETERS){

            LoadParameters(stream, fvs.parameters);

        }else{
            cout << "LoadVectorShape - unknown signature:"<< signature;
            stream.Seek(posChunk+sizeChunk);
        }

        //---
        int dSize = stream.Pos()-posChunk;
        if(dSize!=sizeChunk){
            error = "LoadVectorShape error! Wrong chunkSize for signature:" + std::to_string(signature);
            return nullptr;
        }
        sizeCounter-=(dSize+8);   //8 for two integers - 'signature' and 'size'
    }

    if(stream.Pos()-posStart!=size){
        error = "LoadVectorShape error! Wrong size: " + to_string(stream.Pos()-posStart) + "  saved: " + to_string(size);
        return nullptr;
    }


    //---
    VectorShape *vs = new VectorShape(fvs.probe, fvs.dataFlags, fvs.parameters);
    vs->name = fvs.name;
    vs->id = fvs.id;

    if(fvs.kind==jmRECTANGLE){

        PolyLineShape *polyline = new PolyLineShape();
        polyline->rectangle = true;
        polyline->closed = true;
        for(FVectorVertex &v : fvs.vertices){
            polyline->vertices.push_back(Vec2f(v.x, v.y));
        }
        //polyline->CalculatePathLength();
        polyline->UpdateBoundingBox();
        vs->shape = polyline;

    }else if(fvs.kind==jmPOLYLINE){
        PolyLineShape *polyline = new PolyLineShape();
        polyline->closed = fvs.closed;
        for(FVectorVertex &v : fvs.vertices){
            polyline->vertices.push_back(Vec2f(v.x, v.y));
        }
        //polyline->CalculatePathLength();
        polyline->UpdateBoundingBox();
        vs->shape = polyline;

    }else if(fvs.kind==jmELLIPSE){
        EllipseShape *ellipse = new EllipseShape();
        ellipse->center.Set(fvs.vertices[0].x, fvs.vertices[0].y);
        ellipse->a = fvs.vertices[2].x - fvs.vertices[0].x;   // 0 center, 1 top, 2 right, 3 bottom, 4 left
        ellipse->b = fvs.vertices[3].y - fvs.vertices[0].y;
        if(vectorLayerShape && settings.IsYCoordinateUp()) ellipse->b = -ellipse->b;
        //ellipse->CalculatePathLength();

        vs->shape = ellipse;

    }else if(fvs.kind==jmBEZIER_POLYCURVE){
        BezierShape *bezierCurve = new BezierShape();
        bezierCurve->closed = fvs.closed;
        for(FVectorVertex &v : fvs.vertices){
            bezierCurve->vertices.push_back(BezierVertex(Vec2f(v.x, v.y), Vec2f(v.xBPprevious, v.yBPprevious), Vec2f(v.xBPnext, v.yBPnext)));
        }
        //BezierPolycurveToPolyline(bezierCurve->vertices, bezierCurve->polylineVertices);
        //bezierCurve->CalculatePathLength();

        vs->shape = bezierCurve;

    }else if(fvs.kind==jmSINGLE_POINT){
        SinglePointShape *singlePoint = new SinglePointShape();
        singlePoint->point.Set(fvs.vertices[0].x, fvs.vertices[0].y);

        vs->shape = singlePoint;

    }

    if(vs->shape==nullptr){
        delete vs;
        vs = nullptr;
    }

    return vs;
}


FrameAnimation* JugiMapBinaryLoader::LoadFrameAnimation(BinaryStreamReader &stream, int size, SourceSprite *ss)
{

    FrameAnimation *fa = new FrameAnimation(ss);


    int posStart = stream.Pos();
    int sizeCounter = size;

    //---
    while(sizeCounter>0){
        int signature = stream.ReadInt();
        int sizeChunk = stream.ReadInt();

        if(sizeChunk>sizeCounter){
            error = "LoadFrameAnimation error! ChunkSize error for signature: " + std::to_string(signature);
            delete fa;
            return nullptr;
        }
        int posChunk = stream.Pos();

        if(signature==SaveSignature::VARIABLES){

            fa->name = stream.ReadString();
            int animationKind = stream.ReadInt();       //not needed
            fa->bp.startDelay = stream.ReadInt();
            fa->bp.loopCount = stream.ReadInt();
            fa->bp.loopForever = stream.ReadByte();
            fa->bp.repeat = stream.ReadByte();
            fa->bp.repeat_DelayTimeStart = stream.ReadInt();
            fa->bp.repeat_DelayTimeEnd = stream.ReadInt();
            fa->bp.startAtRepeatTime = stream.ReadByte();
            fa->duration = stream.ReadInt();
            fa->dataFlags = stream.ReadInt();


        }else if(signature==SaveSignature::ANIMATION_FRAMES){

            int nFrames = stream.ReadInt();
            for(int i=0; i<nFrames; i++){

                AnimationFrame *af = new AnimationFrame();
                int indexGraphicItem = stream.ReadInt();
                if(indexGraphicItem==saveID_DUMMY_EMPTY_FRAME){
                    af->texture = nullptr;

                }else{

                    if(indexGraphicItem<0 || indexGraphicItem >= ss->graphicItems.size()){
                        error = "_BuildFrameAnimations error! Animation: "+fa->name+"  IndexGraphicItem:"+std::to_string(indexGraphicItem);
                        delete af;
                        delete fa;
                        return nullptr;
                    }
                    af->texture = ss->graphicItems[indexGraphicItem];
                }

                af->msTime = stream.ReadInt();
                af->offset.x = stream.ReadInt();
                af->offset.y = stream.ReadInt();
                af->flip.x = stream.ReadInt();
                af->flip.y = stream.ReadInt();
                af->dataFlags = stream.ReadInt();
                for(int i=0; i<3; i++){
                    stream.ReadInt();
                }

                if(settings.IsYCoordinateUp()){
                    af->offset.y = - af->offset.y;
                }

                //---
                fa->frames.push_back(af);
            }

            // currently 'msTime' is actually 'frame duration'. We convert it to the actual time which is more suitable for animation player
            int t = 0;
            for(AnimationFrame* af : fa->frames){
                int duration = af->msTime;
                af->msTime = t;
                t += duration;
            }


        }else if(signature==SaveSignature::PARAMETERS){

            LoadParameters(stream, fa->parameters);

        }else{
            messages.push_back("LoadFrameAnimation - unknown signature: " + std::to_string(signature));
            stream.Seek(posChunk+sizeChunk);
        }

        //---
        int dSize = stream.Pos()-posChunk;
        if(dSize!=sizeChunk){
            error = "LoadFrameAnimation error! Wrong chunkSize for signature:" + std::to_string(signature);
            delete fa;
            return nullptr;
        }
        sizeCounter-=(dSize+8);   //8 for two integers - 'signature' and 'size'
    }

    if(stream.Pos()-posStart!=size){
        error = "LoadFrameAnimation error! Wrong size: " + std::to_string(stream.Pos()-posStart) + "  saved: " + std::to_string(size);
        delete fa;
        return nullptr;
    }

    return fa;

}


TimelineAnimation* JugiMapBinaryLoader::LoadTimelineAnimation(BinaryStreamReader &stream, int size, SourceSprite *ss)
{

    TimelineAnimation *ta = new TimelineAnimation(ss);


    int posStart = stream.Pos();
    int sizeCounter = size;

    //---
    while(sizeCounter>0){
        int signature = stream.ReadInt();
        int sizeChunk = stream.ReadInt();

        if(sizeChunk>sizeCounter){
            error = "LoadTimelineAnimation error! ChunkSize error for signature: " + std::to_string(signature);
            delete ta;
            return nullptr;
        }
        int posChunk = stream.Pos();

        if(signature==SaveSignature::VARIABLES){

            ta->name = stream.ReadString();
            int animationKind = stream.ReadInt();       //not needed
            ta->bp.startDelay = stream.ReadInt();
            ta->bp.loopCount = stream.ReadInt();
            ta->bp.loopForever = stream.ReadByte();
            ta->bp.repeat = stream.ReadByte();
            ta->bp.repeat_DelayTimeStart = stream.ReadInt();
            ta->bp.repeat_DelayTimeEnd = stream.ReadInt();
            ta->bp.startAtRepeatTime = stream.ReadByte();
            ta->duration = stream.ReadInt();
            ta->dataFlags = stream.ReadInt();


        }else if(signature==SaveSignature::ANIMATION_MEMBERS){

            int nMembers = stream.ReadInt();
            for(int i=0; i<nMembers; i++){

                AnimationMember *am = new AnimationMember();
                am->nameID = stream.ReadString();
                am->disabled = stream.ReadInt();

                int nTracks = stream.ReadInt();
                for(int t=0; t<nTracks; t++){

                    int signature2 = stream.ReadInt();
                    int sizeChunk2 = stream.ReadInt();
                    int posChunk2 = stream.Pos();

                    if(signature2==SaveSignature::ANIMATION_TRACK){

                        AnimationTrack *at = LoadAnimationTrack(stream, sizeChunk2);
                        if(at){
                            am->animationTracks.push_back(at);
                        }else{
                            error = "LoadTimelineAnimation error! AnimationTrack=nullptr";
                            delete am;
                            delete ta;
                            return nullptr;
                        }

                    }else{
                        messages.push_back("LoadTimelineAnimation - ANIMATION_TRACK - unknown signature: " + std::to_string(signature2));
                        stream.Seek(posChunk2+sizeChunk2);
                    }
                }

                ta->animationMembers.push_back(am);
            }

        }else if(signature==SaveSignature::AK_META_TRACK){

            int signature2 = stream.ReadInt();
            int sizeChunk2 = stream.ReadInt();
            int posChunk2 = stream.Pos();

            if(signature2==SaveSignature::ANIMATION_TRACK){

                ta->metaAnimationTrack = LoadAnimationTrack(stream, sizeChunk2);
                if(ta->metaAnimationTrack==nullptr){
                    error = "LoadTimelineAnimation error! AnimationTrack=nullptr";
                    delete ta;
                    return nullptr;
                }

            }else{
                messages.push_back("LoadTimelineAnimation - AK_META_TRACK - unknown signature: " + std::to_string(signature2));
                stream.Seek(posChunk2+sizeChunk2);
            }


        }else if(signature==SaveSignature::PARAMETERS){

            LoadParameters(stream, ta->parameters);

        }else{
            messages.push_back("LoadTimelineAnimation - unknown signature: " + std::to_string(signature));
            stream.Seek(posChunk+sizeChunk);
        }

        //---
        int dSize = stream.Pos()-posChunk;
        if(dSize!=sizeChunk){
            error = "LoadTimelineAnimation error! Wrong chunkSize for signature:" + std::to_string(signature);
            delete ta;
            return nullptr;
        }
        sizeCounter-=(dSize+8);   //8 for two integers - 'signature' and 'size'
    }

    if(stream.Pos()-posStart!=size){
        error = "LoadTimelineAnimation error! Wrong size: " + std::to_string(stream.Pos()-posStart) + "  saved: " + std::to_string(size);
        delete ta;
        return nullptr;
    }

    return ta;

}


Easing::Kind GetEasingKindFromInt(int _kind)
{

    if(_kind==0){
        return Easing::Kind::LINEAR;

    }else if(_kind==1){
        return Easing::Kind::EASE_START;

    }else if(_kind==2){
        return Easing::Kind::EASE_END;

    }else if(_kind==3){
        return Easing::Kind::EASE_START_END;

    }else if(_kind==4){
        return Easing::Kind::CONSTANT;

    }

    return Easing::Kind::LINEAR;

}


AnimationTrackKind GetAnimationTrackKindFromInt(int _value)
{

    if(_value == 0){
        return AnimationTrackKind::TRANSLATION;

    }else if(_value == 1){
        return AnimationTrackKind::SCALING;

    }else if(_value == 2){
        return AnimationTrackKind::ROTATION;

    }else if(_value == 3){
        return AnimationTrackKind::ALPHA_CHANGE;

    }else if(_value == 4){
        return AnimationTrackKind::OVERLAY_COLOR_CHANGE;

    }else if(_value == 5){
        return AnimationTrackKind::PATH_MOVEMENT;

    }else if(_value == 6){
        return AnimationTrackKind::FRAME_CHANGE;

    }else if(_value == 7){
        return AnimationTrackKind::FRAME_ANIMATION;

    }else if(_value == 8){
        return AnimationTrackKind::TIMELINE_ANIMATION;

    }else if(_value == 9){
        return AnimationTrackKind::FLIP_HIDE;

    }else if(_value == 20){
        return AnimationTrackKind::META;

    }else{
        return AnimationTrackKind::NOT_DEFINED;

    }

}


AnimationTrack * JugiMapBinaryLoader::LoadAnimationTrack(BinaryStreamReader &stream, int size)
{


    AnimationTrack *at = new AnimationTrack();


    int posStart = stream.Pos();
    int sizeCounter = size;

    //---
    while(sizeCounter>0){
        int signature = stream.ReadInt();
        int sizeChunk = stream.ReadInt();

        if(sizeChunk>sizeCounter){
            error = "LoadAnimationTrack error! ChunkSize error for signature: " + std::to_string(signature);
            delete at;
            return nullptr;
        }
        int posChunk = stream.Pos();

        if(signature==SaveSignature::VARIABLES){

            at->disabled = stream.ReadByte();
            int trackKind = stream.ReadInt();
            at->kind = GetAnimationTrackKindFromInt(trackKind);

            at->tp.pathNameID = stream.ReadString();
            int colorBlend = stream.ReadInt();
            at->tp.colorBlend = GetColorOverlayBlendFromInt(colorBlend);
            at->tp.reverseDirectionOnClosedShape = stream.ReadByte();
            at->tp.pathRelDistanceOffset = stream.ReadFloat();
            at->tp.pathDirectionOrientation = stream.ReadByte();


        }else if(signature==SaveSignature::ANIMATION_KEYS){

            int nKeys = stream.ReadInt();
            for(int i=0; i<nKeys; i++){

                AnimationKey * ak = CreateAnimationKey(at);

                ak->time = stream.ReadInt();
                int easing = stream.ReadInt();
                ak->easing.kind = GetEasingKindFromInt(easing);

                if(at->kind==AnimationTrackKind::TRANSLATION){
                   int sig = stream.ReadInt();
                   AKTranslation *k = static_cast<AKTranslation*>(ak);
                   k->position.x = stream.ReadFloat();
                   k->position.y = stream.ReadFloat();
                   if(settings.IsYCoordinateUp()){
                       k->position.y = - k->position.y;
                   }

                }else if(at->kind==AnimationTrackKind::ROTATION){
                    int sig = stream.ReadInt();
                   AKRotation *k = static_cast<AKRotation*>(ak);
                   k->rotation = stream.ReadFloat();
                   if(settings.IsYCoordinateUp()){
                       k->rotation = - k->rotation;
                   }

                }else if(at->kind==AnimationTrackKind::SCALING){
                    int sig = stream.ReadInt();
                    AKScaling *k = static_cast<AKScaling*>(ak);
                    k->scale.x = stream.ReadFloat();
                    k->scale.y = stream.ReadFloat();

                }else if(at->kind==AnimationTrackKind::FLIP_HIDE){
                    int sig = stream.ReadInt();
                   AKFlipHide *k = static_cast<AKFlipHide*>(ak);
                   k->flip.x = stream.ReadInt();
                   k->flip.y = stream.ReadInt();
                   k->hidden = (bool)stream.ReadByte();

                }else if(at->kind==AnimationTrackKind::ALPHA_CHANGE){
                    int sig = stream.ReadInt();
                   AKAlphaChange *k = static_cast<AKAlphaChange*>(ak);
                   k->alpha = stream.ReadFloat();

                }else if(at->kind==AnimationTrackKind::OVERLAY_COLOR_CHANGE){
                    int sig = stream.ReadInt();
                    AKOverlayColorChange *k = static_cast<AKOverlayColorChange*>(ak);
                    k->color.r = stream.ReadFloat();
                    k->color.g = stream.ReadFloat();
                    k->color.b = stream.ReadFloat();
                    k->color.a = stream.ReadFloat();

                }else if(at->kind==AnimationTrackKind::PATH_MOVEMENT){
                    int sig = stream.ReadInt();
                    AKPathMovement *k = static_cast<AKPathMovement*>(ak);
                    k->relDistance = stream.ReadFloat();;

                }else if(at->kind==AnimationTrackKind::FRAME_CHANGE){
                    int sig = stream.ReadInt();
                    AKFrameChange *k = static_cast<AKFrameChange*>(ak);
                    k->frameImageIndex = stream.ReadInt();
                    k->animationFrame.offset.x = stream.ReadInt();
                    k->animationFrame.offset.y = stream.ReadInt();
                    if(settings.IsYCoordinateUp()){
                        k->animationFrame.offset.y = - k->animationFrame.offset.y;
                    }
                    k->animationFrame.flip.x = stream.ReadInt();
                    k->animationFrame.flip.y = stream.ReadInt();

                }else if(at->kind==AnimationTrackKind::FRAME_ANIMATION){
                    int sig = stream.ReadInt();
                    AKFrameAnimation *k = static_cast<AKFrameAnimation*>(ak);
                    k->animationName = stream.ReadString();
                    k->completeLoops = stream.ReadByte();
                    k->discardAnimationsQueue = stream.ReadByte();
                    k->fSpeed = stream.ReadFloat();

                }else if(at->kind==AnimationTrackKind::TIMELINE_ANIMATION){
                    int sig = stream.ReadInt();
                    AKTimelineAnimation *k = static_cast<AKTimelineAnimation*>(ak);
                    k->animationName = stream.ReadString();
                    k->completeLoops = stream.ReadByte();
                    k->discardAnimationsQueue = stream.ReadByte();
                    k->fSpeed = stream.ReadFloat();

                }else if(at->kind==AnimationTrackKind::META){
                    int sig = stream.ReadInt();
                    AKMeta *k = static_cast<AKMeta*>(ak);
                    k->label = stream.ReadString();
                    k->label2 = stream.ReadString();
                    k->dataFlags = stream.ReadInt();

                    //-----
                    int signat = stream.ReadInt();
                    if(signat == SaveSignature::PARAMETERS){
                        int size = stream.ReadInt();
                        LoadParameters(stream, k->parameters);
                    }
                }

                at->animationKeys.push_back(ak);
            }


        }else{
            messages.push_back("LoadAnimationTrack - unknown signature: " + std::to_string(signature));
            stream.Seek(posChunk+sizeChunk);
        }

        //---
        int dSize = stream.Pos()-posChunk;
        if(dSize!=sizeChunk){
            error = "LoadAnimationTrack error! Wrong chunkSize for signature:" + std::to_string(signature);
            delete at;
            return nullptr;
        }
        sizeCounter-=(dSize+8);   //8 for two integers - 'signature' and 'size'
    }

    if(stream.Pos()-posStart!=size){
        error = "LoadAnimationTrack error! Wrong size: " + std::to_string(stream.Pos()-posStart) + "  saved: " + std::to_string(size);
        delete at;
        return nullptr;
    }

    return at;

}


bool JugiMapBinaryLoader::LoadParameters(BinaryStreamReader &stream, std::vector<Parameter> & parameters)
{

    int nParameters = stream.ReadInt();
    for(int i=0; i<nParameters; i++){

        Parameter pv;

        bool pvActive = (bool)stream.ReadByte();

        pv.kind = stream.ReadByte();
        pv.name = stream.ReadString();
        pv.value = stream.ReadString();

        if(pvActive){
            parameters.push_back(pv);
        }
    }

    return true;

}


void JugiMapBinaryLoader::LoadParallaxAndScreenLayerParameters(BinaryStreamReader &stream, Layer *layer)
{

    layer->attachToLayer = stream.ReadString();

    int layerType = stream.ReadInt();
    if(layerType==typePARALLAX_LAYER){
        layer->layerType = LayerType::PARALLAX;

    }else if(layerType==typePARALLAX_STRETCHING_SINGLE_SPRITE){
        layer->layerType = LayerType::PARALLAX_STRETCHING_SINGLE_SPRITE;

    }else if(layerType==typeSCREEN_LAYER){
        layer->layerType = LayerType::SCREEN;

    }else if(layerType==typeSCREEN_STRETCHING_SINGLE_SPRITE){
        layer->layerType = LayerType::SCREEN_STRETCHING_SINGLE_SPRITE;
    }


    layer->parallaxFactor.x = stream.ReadFloat();
    layer->parallaxFactor.y = stream.ReadFloat();

    layer->alignPosition.x = stream.ReadInt();
    layer->alignPosition.y = stream.ReadInt();

    layer->alignOffset.x = stream.ReadInt();
    layer->alignOffset.y = stream.ReadInt();
    if(settings.IsYCoordinateUp()){
        layer->alignOffset.y = - layer->alignOffset.y;      // offset is distance in pixels
    }

    layer->alignOffset_obtainFromMap.x = stream.ReadByte();
    layer->alignOffset_obtainFromMap.y = stream.ReadByte();

    layer->tilingCount.x = stream.ReadInt();
    layer->tilingCount.y = stream.ReadInt();

    layer->tilingCountAutoSpread.x = stream.ReadByte();
    layer->tilingCountAutoSpread.y = stream.ReadByte();

    layer->tilingSpacing.x = stream.ReadInt();
    layer->tilingSpacing.y = stream.ReadInt();

    layer->tilingSpacingDelta.x = stream.ReadInt();
    layer->tilingSpacingDelta.y = stream.ReadInt();


    int stretching = stream.ReadInt();
    if(stretching==stretchXY_WORLD_SIZE){
        layer->stretchingVariant = StretchingVariant::XY_TO_WORLD_SIZE;

    }else if(stretching==stretchXY_VIEWPORT_SIZE){
        layer->stretchingVariant = StretchingVariant::XY_TO_VIEWPORT_SIZE;
    }

}


void JugiMapBinaryLoader::InitCollisionShapes(GraphicItem *gi)
{

    // This function perform two operations:
    // 1) Store coolision shapes vertices relative to image's handle point (exported from the editor are stored relative to the image's top left corner)
    // 2) The first suitable shape will be stored as the default shape for collision detection
    /*
    for(VectorShape *vs : gi->collisionShapes){

        if(vs->GetKind()==ShapeKind::POLYLINE){
            PolyLineShape *poly = static_cast<PolyLineShape *>(vs->GetGeometricShape());

            for(Vec2f &v : poly->vertices){
                if(configuration.IsYCoordinateUp()){
                    v.y = gi->GetHeight()-v.y;
                }

                v = v - gi->handle;
            }

            //----
            if(poly->rectangle==false){
                bool CW = false;
                poly->convex = IsPolygonConvex(poly->vertices,CW);
                if(poly->convex && CW){
                    std::reverse(poly->vertices.begin(), poly->vertices.end());     // order must be CCW
                }
            }

            //----
            if(gi->collisionShape==nullptr){
                if(vs->probe==false && poly->convex && poly->vertices.size()>=3 && poly->vertices.size()<12){
                    gi->collisionShape = vs;
                }
            }


        }else if(vs->GetKind()==ShapeKind::ELLIPSE){
            EllipseShape *ellipse = static_cast<EllipseShape *>(vs->GetGeometricShape());
            if(configuration.IsYCoordinateUp()){
                ellipse->center.y = gi->GetHeight()-ellipse->center.y;
            }
            ellipse->center = ellipse->center - gi->handle;

            if(gi->collisionShape==nullptr){
                if(vs->probe==false && AreSame(ellipse->a, ellipse->b)){        // must be circle
                    gi->collisionShape = vs;
                }
            }

        }else if(vs->GetKind()==ShapeKind::SINGLE_POINT){
            SinglePointShape *sp = static_cast<SinglePointShape *>(vs->GetGeometricShape());
            if(configuration.IsYCoordinateUp()){
                sp->point.y = gi->GetHeight()-sp->point.y;
            }
            sp->point = sp->point - gi->handle;

        }
    }
    */

    std::vector<VectorShape*>allShapes;
    for(VectorShape *vs : gi->spriteShapes) allShapes.push_back(vs);
    for(VectorShape *vs : gi->probeShapes) allShapes.push_back(vs);

    //---
    for(VectorShape *vs : allShapes){

        if(vs->GetKind()==ShapeKind::POLYLINE){
            PolyLineShape *poly = static_cast<PolyLineShape *>(vs->GetGeometricShape());

            for(Vec2f &v : poly->vertices){
                if(settings.IsYCoordinateUp()){
                    v.y = gi->GetHeight()-v.y;
                }

                v = v - gi->handle;
            }

            //----
            //if(poly->rectangle==false){
            //    bool CW = false;
            //    poly->convex = IsPolygonConvex(poly->vertices,CW);
            //    if(poly->convex && CW){
            //        std::reverse(poly->vertices.begin(), poly->vertices.end());     // order must be CCW
            //    }
            //}

            //----
            //if(gi->collisionShape==nullptr){
            //    if(vs->probe==false && poly->convex && poly->vertices.size()>=3 && poly->vertices.size()<12){
            //        gi->collisionShape = vs;
            //    }
            //}


        }else if(vs->GetKind()==ShapeKind::ELLIPSE){
            EllipseShape *ellipse = static_cast<EllipseShape *>(vs->GetGeometricShape());
            if(settings.IsYCoordinateUp()){
                ellipse->center.y = gi->GetHeight()-ellipse->center.y;
            }
            ellipse->center = ellipse->center - gi->handle;

            //if(gi->collisionShape==nullptr){
            //    if(vs->probe==false && AreSame(ellipse->a, ellipse->b)){        // must be circle
            //        gi->collisionShape = vs;
            //    }
            //}

        }else if(vs->GetKind()==ShapeKind::SINGLE_POINT){
            SinglePointShape *sp = static_cast<SinglePointShape *>(vs->GetGeometricShape());
            if(settings.IsYCoordinateUp()){
                sp->point.y = gi->GetHeight()-sp->point.y;
            }
            sp->point = sp->point - gi->handle;

        }
    }
}




}

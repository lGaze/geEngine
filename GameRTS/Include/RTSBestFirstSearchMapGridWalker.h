#pragma once
#include "RTSMapGridWalker.h"



using namespace geEngineSDK;
using namespace WALKSTATE;

namespace RTSGame {

  class RTSTexture;
  class RTSMapTileNode;
  class RTSBestFirstSearchMapGridWalker : public RTSMapGridWalker
  {
  public:

    RTSBestFirstSearchMapGridWalker( void );
    RTSBestFirstSearchMapGridWalker( RTSTiledMap *pMap );
    ~RTSBestFirstSearchMapGridWalker( void );

    /************************************************************************/
    /* Funciones de ayuda de la clase                                       */
    /************************************************************************/

  public:

    virtual bool Init( sf::RenderTarget * target );
    virtual void Destroy();
    virtual WALKSTATE::E Update();
    virtual void Render();
    virtual void PathRender();
    virtual void Reset();
    virtual void traceBack();
    virtual bool weightedGraphSupported()
    {
      return false;
    };

  protected:

    virtual void visitGridNode( int32 x, int32 y );


  private:

    RTSTexture * m_bestPathTex;
    RTSTexture * m_patTex;
    List<RTSMapTileNode*> m_open;			//Nuestra lista abierta utilizando un queue ordinario
    Vector<RTSMapTileNode *> m_close;
    RTSMapTileNode *m_start, *m_n, *m_end;		//Punteros a los nodos de inicio, uso y final
    RTSMapTileNode **m_nodegrid;					//Matriz para almacenamiento de los nodos del mapa
    Vector<RTSMapTileNode *> m_bestPath;

    void
      PriorityQueue( int32 x, int32 y );

  };
}

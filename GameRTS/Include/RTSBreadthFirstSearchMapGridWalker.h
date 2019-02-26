#pragma once
#include "RTSMapGridWalker.h"



using namespace geEngineSDK;
using namespace WALKSTATE;

class RTSTexture;
class RTSMapTileNode;
class RTSBreadthFirstSearchMapGridWalker : public RTSMapGridWalker
{
public:

  RTSBreadthFirstSearchMapGridWalker(void);
  RTSBreadthFirstSearchMapGridWalker(RTSTiledMap *pMap);
  ~RTSBreadthFirstSearchMapGridWalker(void);

  /************************************************************************/
  /* Funciones de ayuda de la clase                                       */
  /************************************************************************/

public:

  virtual bool Init(sf::RenderTarget * target);
  virtual void Destroy();
  virtual WALKSTATE::E Update();
  virtual void Render();
  virtual void PathRender();
  virtual void Reset();
  virtual void traceBack();
  virtual bool weightedGraphSupported() { return false; };

protected:

  virtual void visitGridNode(int32 x, int32 y);


private:

  RTSTexture * m_bestPathTex;
  RTSTexture * m_patTex;
  std::queue<RTSMapTileNode*> m_open;			//Nuestra lista abierta utilizando un queue ordinario
  Vector<RTSMapTileNode *> m_close;
  RTSMapTileNode *m_start, *m_n, *m_end;		//Punteros a los nodos de inicio, uso y final
  RTSMapTileNode **m_nodegrid;					//Matriz para almacenamiento de los nodos del mapa
  Vector<RTSMapTileNode *> m_bestPath;

};
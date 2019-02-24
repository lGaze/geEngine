#pragma once
#pragma once
#include "RTSMapGridWalker.h"



using namespace geEngineSDK;
using namespace WALKSTATE;

class RTSTexture;
class RTSMapTileNode;
class RTSDepthFirstSearchMapGridWalker : public RTSMapGridWalker
{
public:

  RTSDepthFirstSearchMapGridWalker(void);
  RTSDepthFirstSearchMapGridWalker(RTSTiledMap *pMap);
  ~RTSDepthFirstSearchMapGridWalker(void);

  /************************************************************************/
  /* Funciones de ayuda de la clase                                       */
  /************************************************************************/

public:

  virtual bool Init(sf::RenderTarget * target);
  virtual void Destroy();
  virtual WALKSTATE::E Update();
  virtual void Render();
  virtual void Reset();

  virtual bool weightedGraphSupported() { return false; };

protected:

  virtual void visitGridNode(int32 x, int32 y);


private:
  RTSTexture * m_patTex;
  std::stack<RTSMapTileNode*> m_open;			//Nuestra lista abierta utilizando un queue ordinario
  Vector<RTSMapTileNode *> m_close;
  RTSMapTileNode *m_start, *m_n, *m_end;		//Punteros a los nodos de inicio, uso y final
  RTSMapTileNode **m_nodegrid;					//Matriz para almacenamiento de los nodos del mapa

};

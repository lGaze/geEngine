/************************************************************************************************************************/
/* Inclusión de los archivos de cabecera necesarios para la compilación 												*/
/************************************************************************************************************************/
#include "RTSDepthFirstSearchMapGridWalker.h"
#include "RTSTiledMap.h"
#include "RTSMapTileNode.h"
#include "RTSTexture.h"



/************************************************************************************************************************/
/* Implementación de funciones de la clase                              												*/
/************************************************************************************************************************/
RTSDepthFirstSearchMapGridWalker::RTSDepthFirstSearchMapGridWalker(void)
{//Constructor standard
  //Limpiamos variables miembro
  m_n = NULL;
  m_nodegrid = NULL;
  m_StartX = m_StartY = 0;
  m_EndX = m_EndY = 0;
}

RTSDepthFirstSearchMapGridWalker::RTSDepthFirstSearchMapGridWalker(RTSTiledMap *pMap) : RTSMapGridWalker(pMap)
{//Constructor con parámetros (overrided)
  //Limpiamos variables miembro
  m_n = NULL;
  m_nodegrid = NULL;
  m_StartX = m_StartY = 0;
  m_EndX = m_EndY = 0;
}

RTSDepthFirstSearchMapGridWalker::~RTSDepthFirstSearchMapGridWalker(void)
{//Destructor
  Destroy();
}

bool RTSDepthFirstSearchMapGridWalker::Init(sf::RenderTarget * target)
{//Inicializa los objetos para su uso
  //Revisamos que no estén alojados ya los nodos

  m_patTex = new RTSTexture();
  m_patTex->loadFromFile(target, "Textures/Marks/Mark_3.png");
  m_patTex->setScale(.075f, .075f);
  m_patTex->setOrigin(
    (m_patTex->getWidth() - 120) / 2.0f,
    m_patTex->getHeight() / 2.0f);

  m_bestPathTex = new RTSTexture();
  m_bestPathTex->loadFromFile(target, "Textures/Marks/Mark_4.png");
  m_bestPathTex->setScale(.075f, .075f);

  m_bestPathTex->setOrigin(
    (m_bestPathTex->getWidth() - 120) / 2.0f,
    m_bestPathTex->getHeight() / 2.0f);

  if (m_nodegrid != NULL)
  {
    Destroy();
  }

  /*
    NOTA: En esta sección estamos creando un nodo por cada tile en el mapa
    Esta sección puede ser optimizada para su uso de memoria de la siguientes maneras:
    1.- Limitando el número de pasos y creando solo el número de objetos que requeriremos para eso
    2.- Manteniendo una matriz estática reducida para la información utilizada
    3.- Reducir el tipo de variable a un uint16 para las coordenadas
    4.- Reducir la matriz de visitados utilizando máscaras de bits
  */

  //Alojamos nuevos nodos para el uso del algoritmo
  m_nodegrid = new RTSMapTileNode*[m_pTiledMap->getMapSize().x];
  for (int32 i = 0; i < m_pTiledMap->getMapSize().x; i++)
  {
    m_nodegrid[i] = new RTSMapTileNode[m_pTiledMap->getMapSize().y];
    for (int32 j = 0; j < m_pTiledMap->getMapSize().y; j++)
    {
      //Establecemos posiciones y estado de no visitado a todos los nodos
      m_nodegrid[i][j].setVisited(false);
      m_nodegrid[i][j].m_x = i;
      m_nodegrid[i][j].m_y = j;
    }
  }

  return true;	//Si llegamos a este punto, todo salió bien
}

void RTSDepthFirstSearchMapGridWalker::Destroy()
{
  //Destruimos los nodos de la matriz bidimensional
  if (m_nodegrid != nullptr)
  {
    for (int32 i = 0; i < m_pTiledMap->getMapSize().x; i++)
    {
      if (nullptr != m_nodegrid[i])
      {
        delete[](m_nodegrid[i]);
      }
    }
    delete(m_nodegrid);
  }

  //Limpiamos punteros a los nodos
  m_start = nullptr;
  m_end = nullptr;
  m_n = nullptr;
  m_nodegrid = nullptr;
  ge_delete(m_patTex);
  ge_delete(m_bestPathTex);
}

void RTSDepthFirstSearchMapGridWalker::Render()
{//Función utilizada para renderear información del nodo en pantalla

  int32 tmpx;
  int32 tmpy;
  for (int32 i = 1; i < m_close.size();  i++)
  {
    m_pTiledMap->getMapToScreenCoords(m_close[i]->m_x, m_close[i]->m_y, tmpx, tmpy);
    m_patTex->setPosition(tmpx + (TILESIZE_X >> 1), tmpy + (TILESIZE_Y >> 1));
    m_patTex->draw();
  }
}

void RTSDepthFirstSearchMapGridWalker::PathRender()
{
  int32 tmpx;
  int32 tmpy;
  for (int32 i = 0; i < m_bestPath.size(); ++i)
  {
    m_pTiledMap->getMapToScreenCoords(m_bestPath[i]->m_x, m_bestPath[i]->m_y, tmpx, tmpy);
    m_bestPathTex->setPosition(tmpx + (TILESIZE_X >> 1), tmpy + (TILESIZE_Y >> 1));
    m_bestPathTex->draw();
  }
}

WALKSTATE::E RTSDepthFirstSearchMapGridWalker::Update()
{//Función de actualización del algoritmo (calcula un paso a la vez del algoritmo)
  //Revisamos si hay objetos en la lista abierta
  if (m_open.size() > 0)
  {//Hay objetos, por lo que podemos seguir calculando una ruta
    m_n = (RTSMapTileNode*)m_open.top();	//Obtenemos el nodo actual para chequeos
    m_open.pop();							//Sacamos este objeto de la lista abierta
    m_n->setVisited(true);					//Marcamos este nodo como visitado
    m_close.push_back(m_n);

    //Revisamos si el nodo está en la posición del objetivo
    if (m_n->Equals(*m_end))
    {//Este es el objetivo
      State = WALKSTATE::KREACHEDGOAL;	//Indicamos que hemos llegado a la ruta pedida
      return State;
    }

    //Creamos variables temporales para almacenar la dirección de chequeo
    int32 x, y;

    //Agregamos todos los nodos adyacentes a este

    //Agregamos el nodo ESTE
    x = m_n->m_x + 1;
    y = m_n->m_y;
    if (m_n->m_x < (m_pTiledMap->getMapSize().x - 1))
    {//Si no nos hemos salido del rango del mapa
      visitGridNode(x, y);	//Visitamos el nodo
    }

    //Agregamos el nodo SUD-ESTE
    x = m_n->m_x + 1;
    y = m_n->m_y + 1;
    if (m_n->m_x < (m_pTiledMap->getMapSize().x - 1) && m_n->m_y < (m_pTiledMap->getMapSize().y - 1))
    {//Si no nos hemos salido del rango del mapa
      visitGridNode(x, y);	//Visitamos el nodo
    }

    //Agregamos el nodo SUR
    x = m_n->m_x;
    y = m_n->m_y + 1;
    if (m_n->m_y < (m_pTiledMap->getMapSize().y - 1))
    {//Si no nos hemos salido del rango del mapa
      visitGridNode(x, y);	//Visitamos el nodo
    }

    //Agregamos el nodo SUD-OESTE
    x = m_n->m_x - 1;
    y = m_n->m_y + 1;
    if (m_n->m_y < (m_pTiledMap->getMapSize().y - 1) && m_n->m_x > 0)
    {//Si no nos hemos salido del rango del mapa
      visitGridNode(x, y);	//Visitamos el nodo
    }

    //Agregamos el nodo OUSTE
    x = m_n->m_x - 1;
    y = m_n->m_y;
    if (m_n->m_x > 0)
    {//Si no nos hemos salido del rango del mapa
      visitGridNode(x, y);	//Visitamos el nodo
    }

    //Agregamos el nodo NOR-OESTE
    x = m_n->m_x - 1;
    y = m_n->m_y - 1;
    if (m_n->m_x > 0 && m_n->m_y > 0)
    {//Si no nos hemos salido del rango del mapa
      visitGridNode(x, y);	//Visitamos el nodo
    }

    //Agregamos el nodo NORTE
    x = m_n->m_x;
    y = m_n->m_y - 1;
    if (m_n->m_y > 0)
    {//Si no nos hemos salido del rango del mapa
      visitGridNode(x, y);	//Visitamos el nodo
    }

    //Agragamos el nodo NOR-ESTE
    x = m_n->m_x + 1;
    y = m_n->m_y - 1;
    if (m_n->m_y > 0 && m_n->m_x < (m_pTiledMap->getMapSize().x - 1))
    {//Si no nos hemos salido del rango del mapa
      visitGridNode(x, y);	//Visitamos el nodo
    }

    State = WALKSTATE::KSTILLLOOKING;	//Indicamos que aún estamos buscando el objetivo
    return State;
  }

  State = WALKSTATE::KUNABLETOREACHGOAL;	//Si llegamos a este punto indicamos que no es posible encontrar una ruta al objetivo
  return State;
}

void RTSDepthFirstSearchMapGridWalker::visitGridNode(int32 x, int32 y)
{//Esta función "visita" un nodo, esto es para saber si debe agregarse a la lista abierta para su chequeo en el futuro
  // if the node is blocked or has been visited, early out
  if (m_pTiledMap->getType(x, y) != TERRAIN_TYPE::kObstacle && !m_nodegrid[x][y].getVisited())
  {//Si este nodo está bloqueado o ya fue visitado
     //Marcamos este nodo como visitable agregándolo a la lista abierta
    m_open.push(&m_nodegrid[x][y]);

    //Tambien marcamos que el nodo en chequeo actual es el padre de este nodo
   if (!m_nodegrid[x][y].m_parent)
   {
     m_nodegrid[x][y].m_parent = m_n;
   }
  }
}

void RTSDepthFirstSearchMapGridWalker::Reset()
{//Reinicializa la clase para utilizarla otra vez
  //Vaciamos el Queue
  while (m_open.size() > 0)
  {
    m_open.pop();
  }

  if (m_close.size() > 0)
  {
    m_close.clear();
  }

  if (m_bestPath.size() > 0)
  {
    m_bestPath.clear();
  }

  //Establecemos que no hay un nodo actual en chequeo
  m_n = NULL;

  //Revisamos que los nodos ya hayan sido creado (Solo en modo Debug)
  GE_ASSERT(m_nodegrid);

  //Para este punto los nodos ya están creados, solo limpiamos la bandera de visitado a false en todos
  for (int32 i = 0; i < m_pTiledMap->getMapSize().x; i++)
  {
    for (int j = 0; j < m_pTiledMap->getMapSize().y; j++)
    {
      m_nodegrid[i][j].setVisited(false);
    }
  }

  //Obtenemos el punto de inicio, lo marcamos como visitado y lo establecemos como el nodo inicial
  int x, y;
  getStartPosition(x, y);
  m_start = &m_nodegrid[x][y];
  m_start->setVisited(true);

  //Obtenemos el punto final, obtenemos el nodo y lo marcamos como el nodo final
  getEndPosition(x, y);
  m_end = &m_nodegrid[x][y];

  //Agregamos el nodo inicial a la lista abierta
  m_open.push(m_start);

  State = KSTILLLOOKING;
}

void RTSDepthFirstSearchMapGridWalker::traceBack()
{
  if (m_close.size() > 0)
  {
    RTSMapTileNode * node = m_close.back();
    while (node->m_parent)
    {
      m_bestPath.push_back(node);
      node = node->m_parent;
    }
  }
}

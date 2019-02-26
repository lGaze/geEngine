#include "RTSWorld.h"
#include "RTSTiledMap.h"
#include "RTSBreadthFirstSearchMapGridWalker.h"
#include "RTSDepthFirstSearchMapGridWalker.h"
#include "RTSBestFirstSearchMapGridWalker.h"
#include "RTSDijkstraMapGridWalker.h"
#include "RTSAstarMapGridWalker.h"
#include "RTSUnitType.h"

RTSWorld::RTSWorld() {
  m_pTiledMap = nullptr;
  m_activeWalkerIndex = -1;	//-1 = Invalid index
}

RTSWorld::~RTSWorld() {
  destroy();
}

bool
RTSWorld::init(sf::RenderTarget* pTarget) {
  GE_ASSERT(nullptr == m_pTiledMap && "World was already initialized");
  destroy();

  m_pTarget = pTarget;

  //Initialize the map (right now it's an empty map)
  m_pTiledMap = ge_new<RTSTiledMap>();
  GE_ASSERT(m_pTiledMap);
  m_pTiledMap->init(m_pTarget, Vector2I(512, 512));

  //Create the path finding classes and push them to the walker list
  m_walkersList.push_back(ge_new<RTSDepthFirstSearchMapGridWalker>(m_pTiledMap));
  m_walkersList.push_back(ge_new<RTSBreadthFirstSearchMapGridWalker>(m_pTiledMap));
  m_walkersList.push_back(ge_new<RTSBestFirstSearchMapGridWalker>(m_pTiledMap));
  m_walkersList.push_back(ge_new<RTSDijkstraMapGridWalker>(m_pTiledMap));
  m_walkersList.push_back(ge_new<RTSAstarMapGridWalker>(m_pTiledMap));

  //Init the walker objects

  for (SIZE_T it = 0; it < m_walkersList.size(); ++it) {
    m_walkersList[it]->Init(m_pTarget);
  }

  //Set the first walker as the active walker
  setCurrentWalker(m_walkersList.size() > 0 ? 0 : -1);


  RTSGame::RTSUnitType unitTypes;
  unitTypes.loadAnimationData(m_pTarget, 1);

  return true;
}

void
RTSWorld::destroy() {
 //Destroy all the walkers
  while (m_walkersList.size() > 0) {
    ge_delete(m_walkersList.back());
    m_walkersList.pop_back();
  }

  //As the last step, destroy the full map
  if (nullptr != m_pTiledMap) {
    ge_delete(m_pTiledMap);
    m_pTiledMap = nullptr;
  }
}

void
RTSWorld::update(float deltaTime) {
  m_pTiledMap->update(deltaTime);
  if (m_activeWalker->getState() == KSTILLLOOKING)
  {
    m_activeWalker->Update();
  }
 
  if (m_activeWalker->getState() == KREACHEDGOAL)
  {
    m_activeWalker->traceBack();
  }
}

void
RTSWorld::render() {
  m_pTiledMap->render();
  m_activeWalker->Render();
  m_activeWalker->PathRender();
  m_pTiledMap->renderMarks();
}

void RTSWorld::resetPath()
{
  for(int8 it = 0; it < m_walkersList.size(); ++it)
  {
    m_walkersList[it]->Reset();
  }
}

void
RTSWorld::setStartPos(int32 x, int32 y)
{
  for (SIZE_T it = 0; it < m_walkersList.size(); ++it) {
    m_walkersList[it]->setStartPosition(x, y);
  }
}

void
RTSWorld::setEndPos(int32 x, int32 y)
{
  for (SIZE_T it = 0; it < m_walkersList.size(); ++it) {
    m_walkersList[it]->setEndPosition(x, y);
  }
}

void
RTSWorld::updateResolutionData() {
  if (nullptr != m_pTiledMap) {
    Vector2I appResolution = g_gameOptions().s_Resolution;
    
    m_pTiledMap->setStart(0, 0);
    m_pTiledMap->setEnd(appResolution.x, appResolution.y - 175);
    
    //This ensures a clamp if necessary
    m_pTiledMap->moveCamera(0, 0);
  }
}

void
RTSWorld::setCurrentWalker(const int8 index) {
  //Revisamos que el walker exista (en modo de debug)
  GE_ASSERT(m_walkersList.size() > static_cast<SIZE_T>(index));

  m_activeWalker = m_walkersList[index];
  m_activeWalkerIndex = index;
}

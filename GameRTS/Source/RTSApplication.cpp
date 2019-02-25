#include <gePrerequisitesUtil.h>

#if GE_PLATFORM == GE_PLATFORM_WIN32
# include <Win32/geWin32Windows.h>
#endif

#include <geRTTIPlainField.h>
#include <geException.h>
#include <geMath.h>

#include <geCrashHandler.h>
#include <geDynLibManager.h>
#include <geFileSystem.h>
#include <geTime.h>
#include <geUnicode.h>

#include <SFML/Graphics.hpp>

#include <commdlg.h>
#include <imgui.h>
#include <imgui-sfml.h>

#include "RTSConfig.h"
#include "RTSApplication.h"
#include "RTSTiledMap.h"

void
loadMapFromFile(RTSApplication* pApp);

void
mainMenu(RTSApplication* pApp);

RTSApplication::RTSApplication()
  : m_window(nullptr),
    m_fpsTimer(0.0f),
    m_fpsCounter(0.0f),
    m_framesPerSecond(0.0f)
{}

RTSApplication::~RTSApplication() {}

int32
RTSApplication::run() {
  CrashHandler::startUp();
  DynLibManager::startUp();
  Time::startUp();
  GameOptions::startUp();

  __try {
    initSystems();
    gameLoop();
    destroySystems();
  }
  __except (g_crashHandler().reportCrash(GetExceptionInformation())) {
    PlatformUtility::terminate(true);
  }

  GameOptions::shutDown();
  Time::shutDown();
  DynLibManager::shutDown();
  CrashHandler::shutDown();

  return 0;
}

void
RTSApplication::initSystems() {
  if (nullptr != m_window) {  //Window already initialized
    return; //Shouldn't do anything
  }

  //Create the application window
  m_window = ge_new<sf::RenderWindow>(sf::VideoMode(GameOptions::s_Resolution.x,
                                                    GameOptions::s_Resolution.y),
                                      "RTS Game",
                                      sf::Style::Fullscreen);
  if (nullptr == m_window) {
    GE_EXCEPT(InvalidStateException, "Couldn't create Application Window");
  }

  m_arialFont = ge_new<sf::Font>();
  if (nullptr == m_arialFont) {
    GE_EXCEPT(InvalidStateException, "Couldn't create a Font");
  }
  
  
  if (!m_arialFont->loadFromFile("Fonts/arial.ttf")) {
    GE_EXCEPT(FileNotFoundException, "Arial font not found");
  }

  //m_window->setVerticalSyncEnabled(true);

  initGUI();
}

void
RTSApplication::initGUI() {
  ImGui::SFML::Init(*m_window);
}

void
RTSApplication::destroySystems() {
  ImGui::SFML::Shutdown();

  if (nullptr != m_window) {
    m_window->close();
    ge_delete(m_window);
  }

  if (nullptr != m_arialFont) {
    ge_delete(m_arialFont);
  }
}

void
RTSApplication::gameLoop() {
  if (nullptr == m_window) {  //Windows not yet initialized
    return; //Shouldn't do anything
  }

  postInit();

  while (m_window->isOpen()) {
    sf::Event event;
    while (m_window->pollEvent(event)) {
      ImGui::SFML::ProcessEvent(event);
      
      if (ImGui::IsMouseHoveringAnyWindow() || ImGui::IsAnyItemHovered())
      {
        continue;
      }
      else
      {
        //Water
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && GameOptions::s_Terrain == 0)
        {
          int32 Xcoord, Ycoord;
          auto map = m_gameWorld.getTiledMap();
          sf::Vector2i mousePos = sf::Mouse::getPosition();
          map->getScreenToMapCoords(mousePos.x, mousePos.y, Xcoord, Ycoord);
          map->setType(Xcoord, Ycoord, TERRAIN_TYPE::kWater);
        }

        //Obstacle
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && GameOptions::s_Terrain == 1)
        {
          int32 Xcoord, Ycoord;
          auto map = m_gameWorld.getTiledMap();
          sf::Vector2i mousePos = sf::Mouse::getPosition();
          map->getScreenToMapCoords(mousePos.x, mousePos.y, Xcoord, Ycoord);
          map->setType(Xcoord, Ycoord, TERRAIN_TYPE::kObstacle);
          map->setCost(Xcoord, Ycoord, 3000);
        }

        //Grass
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && GameOptions::s_Terrain == 2)
        {
          int32 Xcoord, Ycoord;
          auto map = m_gameWorld.getTiledMap();
          sf::Vector2i mousePos = sf::Mouse::getPosition();
          map->getScreenToMapCoords(mousePos.x, mousePos.y, Xcoord, Ycoord);
          map->setType(Xcoord, Ycoord, TERRAIN_TYPE::kGrass);
        }

        //Marsh
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && GameOptions::s_Terrain == 3)
        {
          int32 Xcoord, Ycoord;
          auto map = m_gameWorld.getTiledMap();
          sf::Vector2i mousePos = sf::Mouse::getPosition();
          map->getScreenToMapCoords(mousePos.x, mousePos.y, Xcoord, Ycoord);
          map->setType(Xcoord, Ycoord, TERRAIN_TYPE::kMarsh);
        }

        //StartFlag
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && GameOptions::s_PfPositions == 0)
        {
          int32 Xcoord, Ycoord;
          auto map = m_gameWorld.getTiledMap();
          sf::Vector2i mousePos = sf::Mouse::getPosition();
          map->getScreenToMapCoords(mousePos.x, mousePos.y, Xcoord, Ycoord);
          m_gameWorld.setStartPos(Xcoord, Ycoord);
          m_gameWorld.setStartPos(Xcoord, Ycoord);
          map->setMark(Xcoord, Ycoord, MARK_TYPE::kStartFlag);
        }       
        
        //EndFlag
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && GameOptions::s_PfPositions == 1)
        {
          int32 Xcoord, Ycoord;
          auto map = m_gameWorld.getTiledMap();
          sf::Vector2i mousePos = sf::Mouse::getPosition();
          map->getScreenToMapCoords(mousePos.x, mousePos.y, Xcoord, Ycoord);
          m_gameWorld.setEndPos(Xcoord, Ycoord);
          map->setMark(Xcoord, Ycoord, MARK_TYPE::kEndFlag);
        }

      }

      if (event.type == sf::Event::Closed) {
        m_window->close();
      }
    }

    g_time()._update();
    ge_frame_mark();
    updateFrame();
    renderFrame();
    ge_frame_clear();
  }

  postDestroy();
}

void
RTSApplication::updateFrame() {
  float deltaTime = g_time().getFrameDelta();
  
  m_fpsTimer += deltaTime;
  if (1.0f < m_fpsTimer) {
    m_framesPerSecond = m_fpsCounter;
    m_fpsCounter = 0.0f;
    m_fpsTimer = 0.0f;
  }
  m_fpsCounter += 1.0f;

  //Update the interface
  ImGui::SFML::Update(*m_window, sf::seconds(deltaTime));

  //Begin the menu 
  mainMenu(this);

  //Check for camera movement
  Vector2 axisMovement(FORCE_INIT::kForceInitToZero);
  Vector2I mousePosition;
  mousePosition.x = sf::Mouse::getPosition(*m_window).x;
  mousePosition.y = sf::Mouse::getPosition(*m_window).y;

  if (0 == mousePosition.x ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::A) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
#ifdef MAP_IS_ISOMETRIC
    axisMovement += Vector2(-1.f, 1.f);
#else
    axisMovement += Vector2(-1.f, 0.f);
#endif
  }
  if (GameOptions::s_Resolution.x -1 == mousePosition.x ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::D) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
#ifdef MAP_IS_ISOMETRIC
    axisMovement += Vector2(1.f, -1.f);
#else
    axisMovement += Vector2(1.f, 0.f);
#endif
  }
  if (0 == mousePosition.y ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::W) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
#ifdef MAP_IS_ISOMETRIC
    axisMovement += Vector2(-1.f, -1.f);
#else
    axisMovement += Vector2(0.f, -1.f);
#endif
  }
  if (GameOptions::s_Resolution.y - 1 == mousePosition.y ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::S) ||
      sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
#ifdef MAP_IS_ISOMETRIC
    axisMovement += Vector2(1.f, 1.f);
#else
    axisMovement += Vector2(0.f, 1.f);
#endif
  }

  axisMovement *= GameOptions::s_MapMovementSpeed * deltaTime;

  m_gameWorld.getTiledMap()->moveCamera(axisMovement.x, axisMovement.y); 

  //Update the world
  m_gameWorld.update(deltaTime);
}

void
RTSApplication::renderFrame() {
  m_window->clear(sf::Color::Blue);

  m_gameWorld.render();

  ImGui::SFML::Render(*m_window);

/*
  sf::Text text;
  text.setPosition(0.f, 30.f);
  text.setFont(*m_arialFont);
  text.setCharacterSize(24);
  text.setFillColor(sf::Color::Red);
  text.setString( toString(1.0f/g_time().getFrameDelta()).c_str() );
  m_window->draw(text);
  */

  m_window->display();
}

void
RTSApplication::postInit() {
  m_gameWorld.init(m_window);
  m_gameWorld.updateResolutionData();
}

void
RTSApplication::postDestroy() {
  m_gameWorld.destroy();
}

void
loadMapFromFile(RTSApplication* pApp) {
  OPENFILENAMEW ofn = { 0 };

  WString fileName;
  fileName.resize(MAX_PATH);
  bool bMustLoad = false;

  Path currentDirectory = FileSystem::getWorkingDirectoryPath();

  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = nullptr;
  ofn.lpstrDefExt = L".bmp";
  ofn.lpstrFilter = L"Bitmap File\0*.BMP\0All\0*.*\0";
  ofn.lpstrInitialDir = L"Maps\\";
  ofn.lpstrFile = &fileName[0];
  ofn.lpstrFile[0] = '\0';
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if (GetOpenFileNameW(&ofn)) {
    if (fileName.size() > 0) {
      bMustLoad = true;
    }
  }

  SetCurrentDirectoryW(UTF8::toWide(currentDirectory.toString()).c_str());

  if (bMustLoad) {
    pApp->getWorld()->getTiledMap()->loadFromImageFile(pApp->getRenderWindow(),
                                                       toString(fileName));
  }
}

void
mainMenu(RTSApplication* pApp) {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Map")) {
      if (ImGui::MenuItem("Load...", "CTRL+O")) {
        loadMapFromFile(pApp);
      }
      if (ImGui::MenuItem("Save...", "CTRL+S")) {

      }
      ImGui::Separator();

      if (ImGui::MenuItem("Quit", "CTRL+Q")) {
        pApp->getRenderWindow()->close();
      }

      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")){
      ImGui::MenuItem("Game Options","", &GameOptions::s_GameOptions);
      if (ImGui::MenuItem("Editor", "", &GameOptions::s_Editor))
      {
        GameOptions::s_PathFinders = false;
      }
      if (ImGui::MenuItem("PathFinders", " ", &GameOptions::s_PathFinders))
      {
        GameOptions::s_Editor = false;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }


  if (GameOptions::s_GameOptions)
  {

    ImGui::Begin("Game Options", &GameOptions::s_GameOptions, ImGuiWindowFlags_AlwaysAutoResize);
    {
      ImGui::Text("Framerate: %f", pApp->getFPS());

      ImGui::SliderFloat("Map movement speed X",
        &GameOptions::s_MapMovementSpeed.x,
        0.0f,
        10240.0f);
      ImGui::SliderFloat("Map movement speed Y",
        &GameOptions::s_MapMovementSpeed.y,
        0.0f,
        10240.0f);

      ImGui::Checkbox("Show grid", &GameOptions::s_MapShowGrid);

    }
    ImGui::End();
  }

  if (GameOptions::s_Editor)
  {
    ImGui::Begin("Editor", &GameOptions::s_Editor, ImGuiWindowFlags_AlwaysAutoResize);
    {
      ImGui::Text("Terrain");
      ImGui::Separator();
      ImGui::RadioButton("Enable Water", &GameOptions::s_Terrain, 0);
      ImGui::Spacing();
      ImGui::RadioButton("Enable Obstacle", &GameOptions::s_Terrain, 1);
      ImGui::Spacing();
      ImGui::RadioButton("Enable Grass", &GameOptions::s_Terrain, 2);
      ImGui::Spacing();
      ImGui::RadioButton("Enable Marsh", &GameOptions::s_Terrain, 3);
      ImGui::Spacing();
      //ImGui::SliderInt("Brush Size")
    }
    ImGui::End();
  }
  else
  {
    GameOptions::s_Terrain = -1;
  }

  if (GameOptions::s_PathFinders)
  {
    ImGui::Begin("PathFinders", &GameOptions::s_PathFinders ,ImGuiWindowFlags_AlwaysAutoResize);
    {
      ImGui::Text("Positions");
      ImGui::Separator();
      ImGui::RadioButton("Start Position", &GameOptions::s_PfPositions, 0);
      ImGui::Spacing();
      ImGui::RadioButton("End Position", &GameOptions::s_PfPositions, 1);
      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::Text("Patfinding Types");
      ImGui::Separator();
      ImGui::RadioButton("Depth First Search", &GameOptions::s_PathFindingTypes, 0);
      ImGui::Spacing();
      ImGui::RadioButton("Breath First Search", &GameOptions::s_PathFindingTypes, 1);
      ImGui::Spacing();
      ImGui::RadioButton("Best First Search", &GameOptions::s_PathFindingTypes, 2);
      ImGui::Spacing();
      ImGui::RadioButton("Dijkstra", &GameOptions::s_PathFindingTypes, 3);
      ImGui::Spacing();
      ImGui::RadioButton("A*", &GameOptions::s_PathFindingTypes, 4);
      ImGui::Spacing();   
      ImGui::Spacing();
      if (ImGui::Button("Start"))
      {
        pApp->getWorld()->resetPath();
        pApp->getWorld()->setCurrentWalker(GameOptions::s_PathFindingTypes);
      }
    }
    ImGui::End();
  }
  else
  {
    GameOptions::s_PfPositions = -1;
    GameOptions::s_PathFindingTypes = -1;
  }

}

#include <gePrerequisitesUtil.h>
#include <geFileSystem.h>
#include <geDataStream.h>
#include <Externals/json.hpp>
#include "RTSUnitType.h"

using namespace geEngineSDK;
using nlohmann::json;

namespace RTSGame {
  RTSUnitType::RTSUnitType() : m_texLoaded( false ), m_texture(nullptr)
  {
  }

  RTSUnitType::~RTSUnitType()
  {
    ge_delete( m_texture );
  }

  void
    RTSUnitType::loadAnimationData( sf::RenderTarget* pTarget, uint32 idUnitType )
  {
    struct tmpStruct
    {
      uint32 id;
      Map<String, uint32> animation;
    };

    Path filePath = "RTS/assets/game_objects/units/";
    Path jsonPath = filePath;
    jsonPath += toString( idUnitType ) + ".json";
    DataStreamPtr fData = FileSystem::openFile( jsonPath );
    auto myJSON = json::parse( fData->getAsString() );

    auto& frames = myJSON["frames"];
    Map<String, tmpStruct> unitsMap;
    Map<String, uint32> animsMap;

    for ( auto iter = frames.begin(); iter != frames.end(); ++iter )
    {
      Vector<String> parsedKey = StringUtil::split( iter.key().c_str(), "/" );
      Vector<String> parsedAnim = StringUtil::split( parsedKey[1], "_" );

      //Categorize all the animations in the file
      tmpStruct& tmpObj = unitsMap[parsedKey[0]];
      tmpObj.id = static_cast< uint32 >( unitsMap.size() );
      if ( tmpObj.animation.end() == tmpObj.animation.find( parsedAnim[0] ) )
      {
        tmpObj.animation[parsedAnim[0]] = 0;
      }

      //Add a frame only on one direction to avoid repetition
      if ( 0 == StringUtil::compare( parsedAnim[1], String( "N" ) ) )
      {
        tmpObj.animation[parsedAnim[0]]++;
      }
    }

    String dirSubIndex[DIRECTIONS::kNUM_DIRECTIONS] = {
      "N", "NW", "W", "SW", "S", "SW", "W", "NW"
    };

    bool invertedSubIndex[DIRECTIONS::kNUM_DIRECTIONS] = {
      false, false, false, false, false, true, true, true
    };

    //Read the information pertinent from the main json and copy to the class
    m_id = unitsMap.size();

    for ( auto& unit : unitsMap )
    {
      if ( m_id == unit.second.id )
      {
        m_name = unit.first;
        m_animationFrames.resize( unit.second.animation.size() );

        uint32 count = 0;
        for ( auto& animation : unit.second.animation )
        {
          Animation& locAnim = m_animationFrames[count];
          locAnim.name = animation.first;
          locAnim.numFrames = animation.second;
          locAnim.duration = 1.0f; //Seconds

          for ( uint32 i = 0; i < DIRECTIONS::kNUM_DIRECTIONS; ++i )
          {
            locAnim.frames[i].resize( locAnim.numFrames );

            for ( uint32 j = 0; j < locAnim.numFrames; ++j )
            {
              StringStream frameName;
              String fullKey;
              frameName << m_name << "/" << locAnim.name << "_";
              frameName << dirSubIndex[i] << "/";
              frameName << locAnim.name << dirSubIndex[i];
              frameName << std::setfill( '0' ) << std::setw( 4 ) << ( j + 1 );
              frameName << ".png";
              frameName >> fullKey;

              auto thisFrame = frames[fullKey.c_str()]["frame"];
              locAnim.frames[i][j].x = thisFrame["x"].get<int32>();
              locAnim.frames[i][j].y = thisFrame["y"].get<int32>();
              locAnim.frames[i][j].w = thisFrame["w"].get<int32>();
              locAnim.frames[i][j].h = thisFrame["h"].get<int32>();
              locAnim.frames[i][j].bSwap = invertedSubIndex[i];
            }
          }
          ++count;
        }
        break;
      }
    }

    //Load the texture for this unit type
    m_pTarget = pTarget;
    if ( !m_texLoaded )
    {
      m_texture = ge_new<RTSTexture>();
      m_texture->loadFromFile( pTarget, filePath.toString() + "units.png" );
      m_texLoaded = true;
    }
  }
}

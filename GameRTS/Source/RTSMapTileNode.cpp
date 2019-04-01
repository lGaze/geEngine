#include "..\Include\RTSMapTileNode.h"

namespace RTSGame {
  RTSMapTileNode::RTSMapTileNode( const RTSMapTileNode & copy )
  {
    m_x = copy.m_x;
    m_y = copy.m_y;
    m_cost = copy.m_cost;
    m_parent = copy.m_parent;
    m_visited = copy.m_visited;
  }

  RTSMapTileNode & RTSMapTileNode::operator=( const RTSMapTileNode & rhs )
  {
    m_x = rhs.m_x;
    m_y = rhs.m_y;
    m_parent = rhs.m_parent;
    m_visited = rhs.m_visited;
    m_cost = rhs.m_cost;

    return *this;
  }

  bool RTSMapTileNode::operator==( const RTSMapTileNode & rhs )
  {
    return this->Equals( rhs );
  }

  bool RTSMapTileNode::operator<( const RTSMapTileNode & rhs )
  {
    return m_cost < rhs.m_cost;
  }

  bool RTSMapTileNode::operator>( const RTSMapTileNode & rhs )
  {
    return m_cost > rhs.m_cost;
  }

  void RTSMapTileNode::setCost( const int32 cost )
  {
    m_cost = cost;
  }

  int32 RTSMapTileNode::getCost() const
  {
    return m_cost;
  }
}
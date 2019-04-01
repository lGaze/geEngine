#pragma once
#include <gePlatformUtility.h>
#include "RTSConfig.h"

#define TILENODE_BLOCKED 3000	//Define un valor muy alto que indica que un nodo está bloqueado y no debe pasarse

namespace RTSGame {
/************************************************************************************************************************/
/* Definición de la clase RTSMapTilNode, esta es la clase base de todos los nodos de traverse para pathfinfing			*/
/************************************************************************************************************************/
  class RTSMapTileNode
  {
    /************************************************************************************************************************/
    /* Constructores y destructor virtual (virtual porque derivaremos clases de esta)                                      	*/
    /************************************************************************************************************************/
  public:
    RTSMapTileNode( void )
    {//Constructor standard
      //Inicializamos las variables miembro de la clase
      m_cost = m_x = m_y = 0;
      m_parent = nullptr;
      m_visited = false;
    }
    RTSMapTileNode( const int32 x, const int32 y, RTSMapTileNode *parent, const bool visited, const int32 cost )
    {//Constructor con parámetros establecidos
      //Inicializa los datos de la clase con los datos de los parámetros
      m_x = x; m_y = y; m_parent = parent;
      m_visited = visited;
      m_cost = cost;
    }
    RTSMapTileNode( const RTSMapTileNode &copy );	//Contructor de copia, genera un nuevo objeto utilizando los valores del objeto de referencia

    virtual ~RTSMapTileNode( void )
    {//Destructor virtual
      m_parent = nullptr;	//Limpiamos la referencia al parent al destruirlo
    }

    /************************************************************************************************************************/
    /* Declaración de operadores virtuales para la clase                           											*/
    /************************************************************************************************************************/
  public:
    RTSMapTileNode &operator=( const RTSMapTileNode &rhs );		//Operador de referencia
    bool operator==( const RTSMapTileNode &rhs );				//Operador de comparación igual		
    bool operator<( const RTSMapTileNode &rhs );				//Operador de comparación menor a
    bool operator>( const RTSMapTileNode &rhs );				//Operador de comparación mayor a

   /************************************************************************************************************************/
   /* Declaración de funciones de acceso                                   												*/
   /************************************************************************************************************************/
    void setParent( RTSMapTileNode* parent )
    {//Establece el parent de este nodo
      m_parent = parent;
    }
    void setVisited( const bool visited )
    {//Establece el estado de visitado de este nodo
      m_visited = visited;
    }
    bool getVisited() const
    {//Regresa el valor de visitado de este nodo
      return m_visited;
    }

    void setCost( const int32 cost );	//Función virtual para establecer el costo de este nodo
    int32 getCost() const;			//Función virtual de acceso al costo de este nodo

    /************************************************************************************************************************/
    /* Funciones de ayuda                                                   												*/
    /************************************************************************************************************************/
    bool Equals( const RTSMapTileNode &rhs ) const
    {//Compara las posiciones entre este y otro nodo e indica si son iguales
      return ( ( m_x == rhs.m_x ) && ( m_y == rhs.m_y ) );
    }

    /************************************************************************************************************************/
    /* Declaración de variables miembro de la clase                         												*/
    /************************************************************************************************************************/
  public:
    int32 m_x;						//Indica la posición en el eje X del nodo
    int32 m_y;						//Indica la posición en el eje Y del nodo
    int32 m_cost;					//Indica el costo para moverse a este nodo
    bool m_visited;					//Bandera que indica si este nodo fue o no visitado
    RTSMapTileNode *m_parent;
  };
}
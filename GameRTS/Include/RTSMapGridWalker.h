/********************************************************************
  Created:	2014/02/13
  Filename:	geMapGridWalker.h
  Author:		Samuel Prince
  Purpose:	Declaraci�n de la clase base geMapGridWalker
        Esta clase es utilizada como una clase base para la
        implementaci�n de diferentes algoritmos de
        pathfinding calculados utilizando el un mapa
*********************************************************************/
#pragma once
#include "gePrerequisitesUtil.h"
#include "SFML/Graphics.hpp"

using namespace geEngineSDK;

namespace WALKSTATE {

  enum E
  {
    KSTILLLOOKING = 0,
    KREACHEDGOAL,
    KUNABLETOREACHGOAL
  };
}

namespace RTSGame {




  class RTSTiledMap;

  /************************************************************************************************************************/
  /* Declaraci�n de la clase geMapGridWalker                              												*/
  /************************************************************************************************************************/
  class RTSMapGridWalker
  {
    /************************************************************************************************************************/
    /* Tipo definido enumerador para reconocer los estados del iterador de pathfinding										*/
    /************************************************************************************************************************/
  public:

    /************************************************************************************************************************/
    /* Constructores y destructor virtual (es virtual para poder derivar clases a partir de esta)							*/
    /************************************************************************************************************************/
  public:
    RTSMapGridWalker( void );
    RTSMapGridWalker( RTSTiledMap *pMap )
    {
      m_pTiledMap = pMap;
    }
    virtual ~RTSMapGridWalker( void );

    /************************************************************************************************************************/
    /* Funciones de ayuda de la clase                                      													*/
    /************************************************************************************************************************/
  public:

    virtual bool Init( )
    {
      return false;
    }									//Funci�n de inicializaci�n de la clase (por default en este aspecto regresamos false porque no estamos haciendo nada)
    virtual void Destroy() = 0;											                                //Destruye los objetos de la clase
    virtual WALKSTATE::E Update() = 0;									                            //Actualiza el estado del pathfinding (puede hacerse por pasos o recursivamente)
    virtual void Render() = 0;											                                //Renderea la informaci�n necesaria para su uso en pathfinding
    virtual void PathRender() = 0;                                                  //Renderea el camino del algotimo 
    virtual void Reset() = 0;												                                //Reinicializa las variables de esta clase para su uso en un nuevo c�lculo

    virtual bool weightedGraphSupported()
    {
      return false;
    }				                  //Indica si este Walker soporta la asignaci�n y el uso de pesos a los nodos del graph
    virtual bool heuristicsSupported()
    {
      return false;
    }					                  //Indica si este Walker soporta heuristicas

    void setTiledMap( RTSTiledMap *pMap )
    {
      m_pTiledMap = pMap;
    }			                //Establecemos un puntero al mapa que ser� utilizado para todos los c�lculos
    RTSTiledMap *getMapGrid()
    {
      return m_pTiledMap;
    }					                      //Regresa el puntero al mapa que se est� utilizando para todos los c�lculos

    void setStartPosition( const int32 x, const int32 y )
    {
      m_StartX = x; m_StartY = y;
    }	    //Esta funci�n se utiliza para establecer el punto inicial de b�squeda
    void setEndPosition( const int32 x, const int32 y )
    {
      m_EndX = x; m_EndY = y;
    }			      //Esta funci�n se utiliza para establecer el punto final de la b�squeda

    void getStartPosition( int32 & x, int32 & y )
    {
      x = m_StartX; y = m_StartY;
    }				      //Funci�n de ayuda para obtener el punto inicial de b�squeda
    void getEndPosition( int32 & x, int32 & y )
    {
      x = m_EndX; y = m_EndY;
    }			        			//Funci�n de ayuda para obtener el punto final de b�squeda

    WALKSTATE::E getState()
    {
      return State;
    }
    virtual void traceBack() = 0;                                                  //Funcion de ayua ara realizar el BackTracing de los algoritmos

  protected:
    virtual void visitGridNode( int32 x, int32 y ) = 0;					                     //Marca un nodo de mapa como visitado (esto lo procesa seg�n el algoritmo utilizado)

    /************************************************************************************************************************/
    /* Declaraci�n de variables miembro de la clase                                 										*/
    /************************************************************************************************************************/
  protected:
    RTSTiledMap *m_pTiledMap;									//Puntero al mapa que estamos usando para calcular
    int32 m_StartX, m_StartY;									//Variables que contendr�n la posici�n de inicio de b�squeda
    int32 m_EndX, m_EndY;										//Variables que contendr�n la posici�n de destino a buscar
    WALKSTATE::E State;

  };
}
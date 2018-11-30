/*****************************************************************************/
/**
 * @file    geGameObject.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/12/06
 * @brief   Type of object that can be referenced by a GameObject handle.
 *
 * Type of object that can be referenced by a GameObject handle. Each object
 * has an unique ID and is registered with the GameObjectManager.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/
#pragma once

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "gePrerequisitesCore.h"
#include <geIReflectable.h>

namespace geEngineSDK {
  /**
   * @brief Flags used for notifying child scene object and components when a
   *        transform has been changed.
   */
  namespace TRANSFORM_CHANGED_FLAGS {
    enum E {
      /**
       * Component will not be notified about any events relating to the
       * transform.
       */
      kNone = 0x00,

      /**
       * Component will be notified when its position, rotation or scale has
       * changed.
       */
      kTransform = 0x01,

      /**
       * Component will be notified when its parent changes.
       */
      kParent = 0x02,

      /**
       * Component will be notified when mobility state changes.
       */
      kMobility = 0x04
    };
  }

  using TransformChangedFlags = TRANSFORM_CHANGED_FLAGS::E;

  /**
   * @brief Type of object that can be referenced by a GameObject handle. Each
   *        object has an unique ID and is registered with the
   *        GameObjectManager.
   */
  class GE_CORE_EXPORT GameObject : public IReflectable
  {
   public:
    GameObject() = default;
    virtual ~GameObject() = default;

    /**
     * @brief Returns the unique instance ID of the GameObject.
     */
    uint64
    getInstanceId() const {
      return m_instanceData->instanceId;
    }

    /**
     * @brief Returns an ID that identifies a link between this object and its
     *        equivalent in the linked prefab. This will be -1 if the object
     *        has no prefab link, or if the object is specific to the instance
     *        and has no prefab equivalent.
     */
    uint32
    getLinkId() const {
      return m_linkId;
    }

    /**
     * @brief Gets the name of the object.
     */
    const String&
    getName() const {
      return m_name;
    }

    /**
     * @brief Sets the name of the object.
     */
    void
    setName(const String& name) {
      m_name = name;
    }

   public:
    /**
     * @brief Marks the object as destroyed. Generally this means the object
     *        has been queued for destruction but it hasn't occurred yet.
     */
    void
    _setIsDestroyed() {
      m_isDestroyed = true;
    }

    /**
     * @brief Checks if the object has been destroyed.
     */
    bool
    _getIsDestroyed() const {
      return m_isDestroyed;
    }

    /**
     * @brief Changes the prefab link ID for this object. See getLinkId().
     */
    void
    _setLinkId(uint32 id) {
      m_linkId = id;
    }

    /**
     * @brief Replaces the instance data with another objects instance data.
     *        This object will basically become the original owner of the
     *        provided instance data as far as all game object handles
     *        referencing it are concerned.
     * @note  No alive objects should ever be sharing the same instance data.
     *        This can be used for restoring dead handles.
     */
    virtual void
    _setInstanceData(GameObjectInstanceDataPtr& other);

    /**
     * @brief Returns instance data that identifies this GameObject and is used
     *        for referencing by game object handles.
     */
    virtual GameObjectInstanceDataPtr
    _getInstanceData() const {
      return m_instanceData;
    }

   protected:
    friend class GameObjectHandleBase;
    friend class GameObjectManager;
    friend class PrefabDiff;
    friend class PrefabUtility;

    /**
     * @brief Initializes the GameObject after construction.
     */
    void
    initialize(const SPtr<GameObject>& object, uint64 instanceId);

    /**
     * @brief Destroys this object.
     * @param[in] handle    Game object handle to this object.
     * @param[in] immediate If true, the object will be deallocated and become
     *            unusable right away. Otherwise the deallocation will be
     *            delayed to the end of frame (preferred method).
     */
    virtual void
    destroyInternal(GameObjectHandleBase& handle, bool immediate = false) = 0;

   protected:
    String m_name;
    uint32 m_linkId = NumLimit::MAX_UINT32;

    Any m_rttiData; //RTTI only

   private:
    friend class Prefab;

    GameObjectInstanceDataPtr m_instanceData;
    bool m_isDestroyed = false;

    /*************************************************************************/
    /**
     * RTTI
     */
    /*************************************************************************/

    friend class ComponentRTTI;
    friend class SceneObjectRTTI;

   public:
     friend class GameObjectRTTI;

    static RTTITypeBase*
    getRTTIStatic();

    virtual RTTITypeBase*
    getRTTI() const override;
  };
}

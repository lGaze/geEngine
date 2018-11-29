/*****************************************************************************/
/**
 * @file    gePrefabDiff.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2018/05/04
 * @brief   Contains differences between two components of the same type.
 *
 * Contains differences between two components of the same type.
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
#include "geGameObject.h"

#include <geIReflectable.h>
#include <geVector3.h>
#include <geQuaternion.h>

namespace geEngineSDK {
  struct SerializationContext;

  struct GE_CORE_EXPORT PrefabComponentDiff : public IReflectable
  {
    int32 id;
    SPtr<SerializedObject> data;

    /*************************************************************************/
    /**
     * RTTI
     */
    /*************************************************************************/
   public:
    friend class PrefabComponentDiffRTTI;

    static RTTITypeBase*
    getRTTIStatic();

    RTTITypeBase*
    getRTTI() const override;
  };

  /**
   * @brief Flags that mark which portion of a scene-object is modified.
   */
  namespace SCENE_OBJECT_DIFF_FLAGS {
    enum E {
      kName = 0x01,
      kTranslation = 0x02,
      kRotation = 0x04,
      kScale = 0x08,
      kActive = 0x10
    };
  }

  /**
   * @brief Contains a set of prefab differences for a single scene object.
   * @see PrefabDiff
   */
  struct GE_CORE_EXPORT PrefabObjectDiff : public IReflectable
  {
    PrefabObjectDiff() = default;

    uint32 id = 0;

    String name;
    Vector3 translation = Vector3::ZERO;
    Quaternion rotation = Quaternion::IDENTITY;
    Vector3 scale = Vector3::ZERO;
    bool isActive = false;
    uint32 soFlags = 0;

    Vector<SPtr<PrefabComponentDiff>> componentDiffs;
    Vector<uint32> removedComponents;
    Vector<SPtr<SerializedObject>> addedComponents;

    Vector<SPtr<PrefabObjectDiff>> childDiffs;
    Vector<uint32> removedChildren;
    Vector<SPtr<SerializedObject>> addedChildren;

    /*************************************************************************/
    /**
     * RTTI
     */
    /*************************************************************************/

  public:
    friend class PrefabObjectDiffRTTI;

    static RTTITypeBase*
    getRTTIStatic();

    RTTITypeBase*
    getRTTI() const override;
  };

  /**
   * @brief Contains modifications between a prefab and its instance.
   *        The modifications are a set of added / removed children or
   *        components and per-field "diffs" of their components.
   */
  class GE_CORE_EXPORT PrefabDiff : public IReflectable
  {
   public:
    /**
     * @brief Creates a new prefab diff by comparing the provided instanced
     *        scene object hierarchy with the prefab scene object hierarchy.
     */
    static SPtr<PrefabDiff>
    create(const HSceneObject& prefab, const HSceneObject& instance);

    /**
     * @brief Applies the internal prefab diff to the provided object.
     *        The object should have similar hierarchy as the prefab the diff
     *        was created for, otherwise the results are undefined.
     * @note  Be aware that this method will not instantiate newly added
     *        components or scene objects. It's expected that this method will
     *        be called on a fresh copy of a scene object hierarchy, and
     *        everything to be instantiated at once after diff is applied.
     */
    void
    apply(const HSceneObject& object);

   private:
    /**
     * @brief A reference to a renamed game object instance data, and its
     *        original ID so it may be restored later.
     */
    struct RenamedGameObject
    {
      GameObjectInstanceDataPtr instanceData;
      uint64 originalId;
    };

    /**
     * @brief Recurses over every scene object in the prefab a generates
     *        differences between itself and the instanced version.
     * @see   create
     */
    static SPtr<PrefabObjectDiff>
    generateDiff(const HSceneObject& prefab, const HSceneObject& instance);

    /**
     * @brief Recursively applies a per-object set of prefab differences to a
     *        specific object.
     * @see   apply
     */
    static void
    applyDiff(const SPtr<PrefabObjectDiff>& diff,
              const HSceneObject& object,
              SerializationContext* context);

    /**
     * @brief Renames all game objects in the provided instance so that IDs of
     *        the objects will match the IDs of their counterparts in the
     *        prefab.
     * @note  This is a temporary action and should be undone by calling
     *        restoreInstanceIds() and providing  it with the output of this
     *        method.
     * @note  By doing this before calling generateDiff() we ensure that any
     *        game object handles pointing to objects within the prefab
     *        instance hierarchy aren't recorded by the diff system, since we
     *        want those to remain as they are after applying the diff.
     */
    static void
    renameInstanceIds(const HSceneObject& prefab,
                      const HSceneObject& instance,
                      Vector<RenamedGameObject>& output);

    /**
     * @brief Restores any instance IDs that were modified by the
     *        renameInstanceIds() method.
     * @see   renameInstanceIds
     */
    static void
    restoreInstanceIds(const Vector<RenamedGameObject>& renamedObjects);

    SPtr<PrefabObjectDiff> m_root;

    /*************************************************************************/
    /**
     * RTTI
     */
    /*************************************************************************/

    Any m_rttiData;

  public:
    friend class PrefabDiffRTTI;

    static RTTITypeBase*
    getRTTIStatic();

    RTTITypeBase*
    getRTTI() const override;
  };
}

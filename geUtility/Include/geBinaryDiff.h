/*****************************************************************************/
/**
 * @file    geBinaryDiff.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/03
 * @brief   Represents an interface RTTI objects need to implement if they want
 *          to provide custom "diff" generation and applying.
 *
 * Represents an interface RTTI objects need to implement if they want to
 * provide custom "diff" generation and applying.
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
#include "gePrerequisitesUtil.h"

namespace geEngineSDK {
  struct SerializationContext;

  /**
   * @brief Types of commands that are used when applying difference field
   *        values.
   */
  namespace DIFF_COMMAND_TYPE {
    enum E {
      kPlain          = 0x01,
      kReflectable    = 0x02,
      kReflectablePtr = 0x03,
      kDataBlock      = 0x04,
      kArraySize      = 0x05,
      kObjectStart    = 0x06,
      kObjectEnd      = 0x07,
      kSubObjectStart = 0x08,
      kArrayFlag      = 0x10
    };
  }

  class GE_UTILITY_EXPORT IDiff
  {
   public:
    virtual ~IDiff() = default;

    /**
     * @brief Generates per-field differences between the provided original and
     *        new object. Any field or array entry that is different in the new
     *        object compared to the original will be output in the resulting
     *        object, with a full hierarchy of that field.
     * Will return null if there is no difference.
     */
    SPtr<SerializedObject>
    generateDiff(const SPtr<SerializedObject>& orgObj,
                 const SPtr<SerializedObject>& newObj);

    /**
     * @brief Applies a previously generated per-field differences to the
     *        provided object. This will essentially transform the original
     *        object the differences were generated for into the modified
     *        version.
     */
    void
    applyDiff(const SPtr<IReflectable>& object,
              const SPtr<SerializedObject>& diff,
              SerializationContext* context);

   protected:
    using ObjectMap = UnorderedMap<SPtr<SerializedObject>, SPtr<SerializedObject>>;
    using DiffObjectMap = UnorderedMap<SPtr<SerializedObject>, SPtr<IReflectable>>;

    /**
     * @brief A command that is used for delaying writing to an object, it
     *        contains all necessary information for setting RTTI field values
     *        on an object.
     */
    struct DiffCommand
    {
      RTTIField* field;
      uint32 type;
      SPtr<IReflectable> object;
      uint8* value;
      SPtr<DataStream> streamValue;
      uint32 size;

      union {
        uint32 arrayIdx;
        uint32 arraySize;
        RTTITypeBase* rttiType;
      };
    };

    /**
     * @brief Recursive version of
     *        generateDiff(const SPtr<SerializedObject>&, const SPtr<SerializedObject>&)
     * @see   generateDiff(const SPtr<SerializedObject>&, const SPtr<SerializedObject>&)
     */
    virtual SPtr<SerializedObject>
    generateDiff(const SPtr<SerializedObject>& orgObj,
                 const SPtr<SerializedObject>& newObj,
                 ObjectMap& objectMap) = 0;

    /**
     * @brief Generates a difference between data of a specific field type
     *        indiscriminately of the specific field type.
     * @see   generateDiff(const SPtr<SerializedObject>&, const SPtr<SerializedObject>&)
     */
    SPtr<SerializedInstance>
    generateDiff(RTTITypeBase* rtti,
                 uint32 fieldType,
                 const SPtr<SerializedInstance>& orgData,
                 const SPtr<SerializedInstance>& newData,
                 ObjectMap& objectMap);

    /**
     * @brief Recursive version of
              applyDiff(const SPtr<IReflectable>&, const SPtr<SerializedObject>&).
              Outputs a set of commands that then must be executed in order to
              actually apply the difference to the provided object.
     * @see   applyDiff(const SPtr<IReflectable>&, const SPtr<SerializedObject>&)
     */
    virtual void
    applyDiff(const SPtr<IReflectable>& object,
              const SPtr<SerializedObject>& diff,
              FrameAlloc& alloc,
              DiffObjectMap& objectMap,
              FrameVector<DiffCommand>& diffCommands,
              SerializationContext* context) = 0;

    /**
     * @brief Applies diff according to the diff handler retrieved from the
     *        provided RTTI object.
     * @see   applyDiff(const SPtr<IReflectable>&, const SPtr<SerializedObject>&)
     */
    void
    applyDiff(RTTITypeBase* rtti,
              const SPtr<IReflectable>& object,
              const SPtr<SerializedObject>& diff,
              FrameAlloc& alloc,
              DiffObjectMap& objectMap,
              FrameVector<DiffCommand>& diffCommands,
              SerializationContext* context);
  };

  /**
   * @brief Generates and applies "diffs". Diffs contain per-field differences
   *        between an original and new object. These differences can be saved
   *        and then applied to an original object to transform it to the new
   *        version.
   * @note	Objects must be in intermediate serialized format generated by
   *        BinarySerializer.
   */
  class GE_UTILITY_EXPORT BinaryDiff : public IDiff
  {
   private:
    /**
     * @copydoc IDiff::generateDiff(const SPtr<SerializedObject>&,
                                    const SPtr<SerializedObject>&, ObjectMap&)
     */
    SPtr<SerializedObject>
    generateDiff(const SPtr<SerializedObject>& orgObj,
                 const SPtr<SerializedObject>& newObj,
                 ObjectMap& objectMap) override;

    /**
     * @copydoc IDiff::applyDiff(const SPtr<IReflectable>&,
     *                           const SPtr<SerializedObject>&,
     *                           DiffObjectMap&,
     *                           Vector<DiffCommand>&)
     */
    void
    applyDiff(const SPtr<IReflectable>& object,
              const SPtr<SerializedObject>& diff,
              FrameAlloc& alloc,
              DiffObjectMap& objectMap,
              FrameVector<DiffCommand>& diffCommands,
              SerializationContext* context) override;
  };
}

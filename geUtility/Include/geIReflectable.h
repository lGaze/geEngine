/*****************************************************************************/
/**
 * @file    geIReflectable.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/03
 * @brief   Interface implemented by classes that provide RTTI.
 *
 * Provides an interface for accessing fields of a certain class.
 * Data can be easily accessed by getter and setter methods.
 *
 * Any class implementing this interface must implement the getRTTI() method,
 * as well as a static getRTTIStatic() method, returning the same value as
 * getRTTI(). Object returned by those methods is used for retrieving actual
 * RTTI data about the class.
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
#include "geAny.h"

namespace geEngineSDK {
  class GE_UTILITY_EXPORT IReflectable
  {
   public:
    virtual ~IReflectable() = default;

    /**
     * @brief Returns an interface you can use to access class's RTTI.
     * @note  You must derive your own version of RTTITypeBase, in which you
     *        may encapsulate all reflection specific operations.
     */
    virtual RTTITypeBase*
    getRTTI() const = 0;

    /**
     * @brief Returns all available RTTI types.
     */
    static UnorderedMap<uint32, RTTITypeBase*>&
    getAllRTTITypes() {
      static UnorderedMap<uint32, RTTITypeBase*> s_AllRTTITypes;
      return s_AllRTTITypes;
    }

    /**
     * @brief Returns true if current RTTI class is derived from @p base
     *        (Or if it is the same type as base).
     */
    bool
    isDerivedFrom(RTTITypeBase* base);

    /**
     * @brief Returns an unique type identifier of the class.
     */
    uint32
    getTypeId() const;

    /**
     * @brief Returns the type name of the class.
     * @note  Name is not necessarily unique.
     */
    const String&
    getTypeName() const;

    /**
     * @brief Creates an empty instance of a class from a type identifier.
     */
    static SPtr<IReflectable>
    createInstanceFromTypeId(uint32 rttiTypeId);

    /**
     * @brief Called by each type implementing RTTITypeBase, on program load.
     */
    static void
    _registerRTTIType(RTTITypeBase* rttiType);

    /**
     * @brief Returns class's RTTI type from type id.
     */
    static RTTITypeBase*
    _getRTTIfromTypeId(uint32 rttiTypeId);

    /**
     * @brief Checks if the provided type id is unique.
     */
    static bool
    _isTypeIdDuplicate(uint32 typeId);

    /**
     * @brief Iterates over all RTTI types and reports any circular references
     *        (for example one type having a field referencing another type,
     *        and that type having a field referencing the first type).
     *        Circular references are problematic because when serializing the
     *        system cannot determine in which order they should be resolved.
     *        In that case user should use RTTI_Flag_WeakRef to mark one of the
     *        references as weak. This flags tells the system that the
     *        reference may be resolved in an undefined order, but also no
     *        longer guarantees that object assigned to that field during
     *        de-serialization will be fully de-serialized itself, as that
     *        might be delayed to a later time.
     */
    static void
    _checkForCircularReferences();

    /**
     * @brief Returns an interface you can use to access class's RTTI.
     */
    static RTTITypeBase*
    getRTTIStatic();
  };
}

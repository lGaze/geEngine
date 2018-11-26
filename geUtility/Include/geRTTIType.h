/*****************************************************************************/
/**
 * @file    geRTTIType.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/03
 * @brief   Provides an interface for accessing fields of a certain class.
 *
 * Provides an interface for accessing fields of a certain class.
 * Data can be easily accessed by getter and setter methods.
 *
 * Supported data types:
 * - Plain types - All types defined in geRTTIField.h, mostly native types and
 *   POD (plain old data) structures. Data is parsed byte by byte.
 *   No pointers to plain types are supported. Data is passed around by value.
 *
 * - Reflectable types - Any class deriving from IReflectable. Data is parsed
 *   based on fields in its RTTI class. Can be pointer or value type.
 *
 * - Arrays of both plain and reflectable types are supported
 *
 * - Data blocks - A managed or unmanaged block of data. See ManagedDataBlock.
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
#include <string>
#include <algorithm>
#include <unordered_map>

#include "gePrerequisitesUtil.h"
#include "geRTTIField.h"
#include "geRTTIPlainField.h"
#include "geRTTIReflectableField.h"
#include "geRTTIReflectablePtrField.h"
#include "geRTTIManagedDataBlockField.h"
#include "geIReflectable.h"
#include "geBinaryDiff.h"

namespace geEngineSDK {
  using std::is_base_of;
  using std::function;
  using std::bind;

  /***************************************************************************/
  /**
   * @brief Starts definitions for member fields within a RTTI type. Follow
   *        this with calls to GE_RTTI_MEMBER* calls, and finish by calling
   *        GE_END_RTTI_MEMBERS. You must also initialize m_initMembers field
   *        in the parent class's constructor.
   */
#define GE_BEGIN_RTTI_MEMBERS                                                 \
  struct META_FirstEntry {};                                                  \
                                                                              \
  void                                                                        \
  META_InitPrevEntry(META_FirstEntry typeId) {                                \
    GE_UNREFERENCED_PARAMETER(typeId);                                        \
  }                                                                           \
                                                                              \
  typedef META_FirstEntry
   /***************************************************************************/

   /***************************************************************************/
   /**
    * @brief Registers a new member field in the RTTI type. The field references
    *        the @p name member in the owner class.
    * The type of the member must be a valid plain type. Each field must specify
    * a unique ID for @p id.
    */
#define GE_RTTI_MEMBER_PLAIN(name, id)                                        \
  META_Entry_##name;                                                          \
                                                                              \
  decltype(OwnerType::name)&                                                  \
  get##name(OwnerType* obj) {                                                 \
    return obj->name;                                                         \
  }                                                                           \
                                                                              \
  void                                                                        \
  set##name(OwnerType* obj, decltype(OwnerType::name)& val) {                 \
    obj->name = val;                                                          \
  }                                                                           \
                                                                              \
  struct META_NextEntry_##name{};                                             \
                                                                              \
  void                                                                        \
  META_InitPrevEntry(META_NextEntry_##name typeId) {                          \
    GE_UNREFERENCED_PARAMETER(typeId);                                        \
    addPlainField(#name, id, &MyType::get##name, &MyType::set##name);         \
    META_InitPrevEntry(META_Entry_##name());                                  \
  }                                                                           \
                                                                              \
  typedef META_NextEntry_##name
  /***************************************************************************/

  /***************************************************************************/
  /**
   * @brief Same as GE_RTTI_MEMBER_PLAIN, but allows you to specify separate
   *        names for the field name and the member variable.
   */
#define GE_RTTI_MEMBER_PLAIN_NAMED(name, field, id)                           \
  META_Entry_##name;                                                          \
                                                                              \
  decltype(OwnerType::field)&                                                 \
  get##name(OwnerType* obj) {                                                 \
    return obj->field;                                                        \
  }                                                                           \
                                                                              \
  void                                                                        \
  set##name(OwnerType* obj, decltype(OwnerType::field)& val) {                \
    obj->field = val;                                                         \
  }                                                                           \
                                                                              \
  struct META_NextEntry_##name{};                                             \
                                                                              \
  void                                                                        \
  META_InitPrevEntry(META_NextEntry_##name typeId) {                          \
    GE_UNREFERENCED_PARAMETER(typeId);                                        \
    addPlainField(#name, id, &MyType::get##name, &MyType::set##name);         \
    META_InitPrevEntry(META_Entry_##name());                                  \
  }                                                                           \
                                                                              \
  typedef META_NextEntry_##name
  /***************************************************************************/

  /***************************************************************************/
  /**
   * @brief Registers a new member field in the RTTI type. The field references
   *        the @p name member in the owner class.
   * The type of the member must be an array of valid plain types. Each field
   * must specify a unique ID for @p id.
   */
#define GE_RTTI_MEMBER_PLAIN_ARRAY(name, id)                                  \
  META_Entry_##name;                                                          \
                                                                              \
  std::common_type<decltype(OwnerType::name)>::type::value_type&              \
  get##name(OwnerType* obj, uint32 idx) {                                     \
    return obj->name[idx];                                                    \
  }                                                                           \
                                                                              \
  void                                                                        \
  set##name(OwnerType* obj,                                                   \
            uint32 idx,                                                       \
            std::common_type<decltype(OwnerType::name)>                       \
            ::type::value_type& val) {                                        \
    obj->name[idx] = val;                                                     \
  }                                                                           \
                                                                              \
  uint32                                                                      \
  getSize##name(OwnerType* obj) {                                             \
    return static_cast<uint32>(obj->name.size());                             \
  }                                                                           \
                                                                              \
  void                                                                        \
  setSize##name(OwnerType* obj, uint32 val) {                                 \
    obj->name.resize(val);                                                    \
  }                                                                           \
                                                                              \
  struct META_NextEntry_##name {};                                            \
                                                                              \
  void                                                                        \
  META_InitPrevEntry(META_NextEntry_##name typeId) {                          \
    GE_UNREFERENCED_PARAMETER(typeId);                                        \
    addPlainArrayField(#name,                                                 \
                       id,                                                    \
                       &MyType::get##name,                                    \
                       &MyType::getSize##name,                                \
                       &MyType::set##name,                                    \
                       &MyType::setSize##name);                               \
    META_InitPrevEntry(META_Entry_##name());                                  \
  }                                                                           \
                                                                              \
  typedef META_NextEntry_##name
  /***************************************************************************/

  /***************************************************************************/
  /**
   * @brief Same as GE_RTTI_MEMBER_PLAIN_ARRAY, but allows you to specify
   *        separate names for the field name and the member variable.
   */
#define GE_RTTI_MEMBER_PLAIN_ARRAY_NAMED(name, field, id)                     \
  META_Entry_##name;                                                          \
                                                                              \
  std::common_type<decltype(OwnerType::field)>::type::value_type&             \
  get##name(OwnerType* obj, uint32 idx) {                                     \
    return obj->field[idx];                                                   \
  }                                                                           \
                                                                              \
  void                                                                        \
  set##name(OwnerType* obj,                                                   \
            uint32 idx,                                                       \
            std::common_type<decltype(OwnerType::field)>                      \
            ::type::value_type& val) {                                        \
    obj->field[idx] = val;                                                    \
  }                                                                           \
                                                                              \
  uint32                                                                      \
  getSize##name(OwnerType* obj) {                                             \
    return static_cast<uint32>(obj->field.size());                            \
  }                                                                           \
                                                                              \
  void                                                                        \
  setSize##name(OwnerType* obj, uint32 val) {                                 \
    obj->field.resize(val);                                                   \
  }                                                                           \
                                                                              \
  struct META_NextEntry_##name {};                                            \
                                                                              \
  void                                                                        \
  META_InitPrevEntry(META_NextEntry_##name typeId) {                          \
    GE_UNREFERENCED_PARAMETER(typeId);                                        \
    addPlainArrayField(#name,                                                 \
                       id,                                                    \
                       &MyType::get##name,                                    \
                       &MyType::getSize##name,                                \
                       &MyType::set##name,                                    \
                       &MyType::setSize##name);                               \
    META_InitPrevEntry(META_Entry_##name());                                  \
  }                                                                           \
                                                                              \
  typedef META_NextEntry_##name
  /***************************************************************************/

  /***************************************************************************/
  /**
   * @brief Registers a new member field in the RTTI type. The field references
   *        the @p name member in the owner class.
   * The type of the member must be a valid reflectable (non-pointer) type.
   * Each field must specify a unique ID for @p id.
   */
#define GE_RTTI_MEMBER_REFL(name, id)                                         \
  META_Entry_##name;                                                          \
                                                                              \
  decltype(OwnerType::name)&                                                  \
  get##name(OwnerType* obj) {                                                 \
    return obj->name;                                                         \
  }                                                                           \
                                                                              \
  void                                                                        \
  set##name(OwnerType* obj, decltype(OwnerType::name)& val) {                 \
    obj->name = val;                                                          \
  }                                                                           \
                                                                              \
  struct META_NextEntry_##name {};                                            \
                                                                              \
  void                                                                        \
  META_InitPrevEntry(META_NextEntry_##name typeId) {                          \
    GE_UNREFERENCED_PARAMETER(typeId);                                        \
    addReflectableField(#name, id, &MyType::get##name, &MyType::set##name);   \
    META_InitPrevEntry(META_Entry_##name());                                  \
  }                                                                           \
                                                                              \
  typedef META_NextEntry_##name
  /***************************************************************************/

  /***************************************************************************/
  /**
   * @brief Same as GE_RTTI_MEMBER_REFL, but allows you to specify separate
   *        names for the field name and the member variable.
   */
#define GE_RTTI_MEMBER_REFL_NAMED(name, field, id)                            \
  META_Entry_##name;                                                          \
                                                                              \
  decltype(OwnerType::field)&                                                 \
  get##name(OwnerType* obj) {                                                 \
    return obj->field;                                                        \
  }                                                                           \
                                                                              \
  void                                                                        \
  set##name(OwnerType* obj, decltype(OwnerType::field)& val) {                \
    obj->field = val;                                                         \
  }                                                                           \
                                                                              \
  struct META_NextEntry_##name {};                                            \
                                                                              \
  void                                                                        \
  META_InitPrevEntry(META_NextEntry_##name typeId) {                          \
    GE_UNREFERENCED_PARAMETER(typeId);                                        \
    addReflectableField(#name, id, &MyType::get##name, &MyType::set##name);   \
    META_InitPrevEntry(META_Entry_##name());                                  \
  }                                                                           \
                                                                              \
  typedef META_NextEntry_##name
  /***************************************************************************/

  /***************************************************************************/
  /**
   * @brief Registers a new member field in the RTTI type. The field references
   *        the @p name member in the owner class.
   * The type of the member must be an array of valid reflectable (non-pointer)
   * types. Each field must specify a unique ID for @p id.
   */
#define GE_RTTI_MEMBER_REFL_ARRAY(name, id)                                   \
  META_Entry_##name;                                                          \
                                                                              \
  std::common_type<decltype(OwnerType::name)>::type::value_type&              \
  get##name(OwnerType* obj, uint32 idx) {                                     \
    return obj->name[idx];                                                    \
  }                                                                           \
                                                                              \
  void                                                                        \
  set##name(OwnerType* obj,                                                   \
            uint32 idx,                                                       \
            std::common_type<decltype(OwnerType::name)>                       \
            ::type::value_type& val) {                                        \
    obj->name[idx] = val;                                                     \
  }                                                                           \
                                                                              \
  uint32                                                                      \
  getSize##name(OwnerType* obj) {                                             \
    return static_cast<uint32>(obj->name.size());                             \
  }                                                                           \
                                                                              \
  void                                                                        \
  setSize##name(OwnerType* obj, uint32 val) {                                 \
    obj->name.resize(val);                                                    \
  }                                                                           \
                                                                              \
  struct META_NextEntry_##name {};                                            \
                                                                              \
  void                                                                        \
  META_InitPrevEntry(META_NextEntry_##name typeId) {                          \
    GE_UNREFERENCED_PARAMETER(typeId);                                        \
    addReflectableArrayField(#name,                                           \
                             id,                                              \
                             &MyType::get##name,                              \
                             &MyType::getSize##name,                          \
                             &MyType::set##name,                              \
                             &MyType::setSize##name);                         \
    META_InitPrevEntry(META_Entry_##name());                                  \
  }                                                                           \
                                                                              \
  typedef META_NextEntry_##name
  /***************************************************************************/

  /***************************************************************************/
  /**
   * @brief Same as GE_RTTI_MEMBER_REFL_ARRAY, but allows you to specify
   *        separate names for the field name and the member variable.
   */
#define GE_RTTI_MEMBER_REFL_ARRAY_NAMED(name, field, id)                      \
  META_Entry_##name;                                                          \
                                                                              \
  std::common_type<decltype(OwnerType::field)>::type::value_type&             \
  get##name(OwnerType* obj, uint32 idx) {                                     \
    return obj->field[idx];                                                   \
  }                                                                           \
                                                                              \
  void                                                                        \
  set##name(OwnerType* obj,                                                   \
      uint32 idx,                                                             \
      std::common_type<decltype(OwnerType::field)>                            \
      ::type::value_type& val) {                                              \
    obj->field[idx] = val;                                                    \
  }                                                                           \
                                                                              \
  uint32                                                                      \
  getSize##name(OwnerType* obj) {                                             \
    return static_cast<uint32>(obj->field.size());                            \
  }                                                                           \
                                                                              \
  void                                                                        \
  setSize##name(OwnerType* obj, uint32 val) {                                 \
    obj->field.resize(val);                                                   \
  }                                                                           \
                                                                              \
  struct META_NextEntry_##name{};                                             \
                                                                              \
  void                                                                        \
  META_InitPrevEntry(META_NextEntry_##name typeId) {                          \
    GE_UNREFERENCED_PARAMETER(typeId);                                        \
    addReflectableArrayField(#name,                                           \
                             id,                                              \
                             &MyType::get##name,                              \
                             &MyType::getSize##name,                          \
                             &MyType::set##name,                              \
                             &MyType::setSize##name);                         \
    META_InitPrevEntry(META_Entry_##name());                                  \
  }                                                                           \
                                                                              \
  typedef META_NextEntry_##name
  /***************************************************************************/

  /***************************************************************************/
  /**
   * @brief Registers a new member field in the RTTI type. The field references
   *        the @p name member in the owner class.
   * The type of the member must be a valid reflectable pointer type. Each
   * field must specify a unique ID for @p id.
   */
#define GE_RTTI_MEMBER_REFLPTR(name, id)                                      \
  META_Entry_##name;                                                          \
                                                                              \
  decltype(OwnerType::name)                                                   \
  get##name(OwnerType* obj) {                                                 \
    return obj->name;                                                         \
  }                                                                           \
                                                                              \
  void                                                                        \
  set##name(OwnerType* obj, decltype(OwnerType::name) val) {                  \
    obj->name = val;                                                          \
  }                                                                           \
                                                                              \
  struct META_NextEntry_##name {};                                            \
                                                                              \
  void                                                                        \
  META_InitPrevEntry(META_NextEntry_##name typeId) {                          \
    GE_UNREFERENCED_PARAMETER(typeId);                                        \
    addReflectablePtrField(#name,                                             \
                           id,                                                \
                           &MyType::get##name,                                \
                           &MyType::set##name);                               \
    META_InitPrevEntry(META_Entry_##name());                                  \
  }                                                                           \
                                                                              \
  typedef META_NextEntry_##name
  /***************************************************************************/

  /***************************************************************************/
  /**
   * @brief Same as GE_RTTI_MEMBER_REFLPTR, but allows you to specify separate
   *        names for the field name and the member variable.
   */
#define GE_RTTI_MEMBER_REFLPTR_NAMED(name, field, id)                         \
  META_Entry_##name;                                                          \
                                                                              \
  decltype(OwnerType::field)                                                  \
  get##name(OwnerType* obj) {                                                 \
    return obj->field;                                                        \
  }                                                                           \
                                                                              \
  void                                                                        \
  set##name(OwnerType* obj, decltype(OwnerType::field) val) {                 \
    obj->field = val;                                                         \
  }                                                                           \
                                                                              \
  struct META_NextEntry_##name {};                                            \
                                                                              \
  void                                                                        \
  META_InitPrevEntry(META_NextEntry_##name typeId) {                          \
    GE_UNREFERENCED_PARAMETER(typeId);                                        \
    addReflectablePtrField(#name,                                             \
                           id,                                                \
                           &MyType::get##name,                                \
                           &MyType::set##name);                               \
    META_InitPrevEntry(META_Entry_##name());                                  \
  }                                                                           \
                                                                              \
  typedef META_NextEntry_##name
  /***************************************************************************/

  /***************************************************************************/
  /**
   * @brief Registers a new member field in the RTTI type. The field references
   *        the @p name member in the owner class.
   * The type of the member must be a valid reflectable pointer type. Each
   * field must specify a unique ID for @p id.
   */
#define GE_RTTI_MEMBER_REFLPTR_ARRAY(name, id)                                \
  META_Entry_##name;                                                          \
                                                                              \
  std::common_type<decltype(OwnerType::name)>::type::value_type               \
  get##name(OwnerType* obj, uint32 idx) {                                     \
    return obj->name[idx];                                                    \
  }                                                                           \
                                                                              \
  void                                                                        \
  set##name(OwnerType* obj,                                                   \
            uint32 idx,                                                       \
            std::common_type<decltype(OwnerType::name)>                       \
            ::type::value_type val) {                                         \
    obj->name[idx] = val;                                                     \
  }                                                                           \
                                                                              \
  uint32                                                                      \
  getSize##name(OwnerType* obj) {                                             \
    return static_cast<uint32>(obj->name.size());                             \
  }                                                                           \
                                                                              \
  void                                                                        \
  setSize##name(OwnerType* obj, uint32 val) {                                 \
    obj->name.resize(val);                                                    \
  }                                                                           \
                                                                              \
  struct META_NextEntry_##name {};                                            \
                                                                              \
  void                                                                        \
  META_InitPrevEntry(META_NextEntry_##name typeId) {                          \
    GE_UNREFERENCED_PARAMETER(typeId);                                        \
    addReflectablePtrArrayField(#name,                                        \
                                id,                                           \
                                &MyType::get##name,                           \
                                &MyType::getSize##name,                       \
                                &MyType::set##name,                           \
                                &MyType::setSize##name);                      \
    META_InitPrevEntry(META_Entry_##name());                                  \
  }                                                                           \
                                                                              \
  typedef META_NextEntry_##name
  /***************************************************************************/

  /***************************************************************************/
  /**
   * @brief Same as GE_RTTI_MEMBER_REFLPTR_ARRAY, but allows you to specify
   *        separate names for the field name and the member variable.
   */
#define GE_RTTI_MEMBER_REFLPTR_ARRAY_NAMED(name, field, id)                   \
  META_Entry_##name;                                                          \
                                                                              \
  std::common_type<decltype(OwnerType::field)>::type::value_type              \
  get##name(OwnerType* obj, uint32 idx) {                                     \
    return obj->field[idx];                                                   \
  }                                                                           \
                                                                              \
  void                                                                        \
  set##name(OwnerType* obj,                                                   \
            uint32 idx,                                                       \
            std::common_type<decltype(OwnerType::field)>                      \
            ::type::value_type val) {                                         \
    obj->field[idx] = val;                                                    \
  }                                                                           \
                                                                              \
  uint32                                                                      \
  getSize##name(OwnerType* obj) {                                             \
    return static_cast<uint32>(obj->field.size());                            \
  }                                                                           \
                                                                              \
  void                                                                        \
  setSize##name(OwnerType* obj, uint32 val) {                                 \
    obj->field.resize(val);                                                   \
  }                                                                           \
                                                                              \
  struct META_NextEntry_##name {};                                            \
                                                                              \
  void                                                                        \
  META_InitPrevEntry(META_NextEntry_##name typeId) {                          \
    GE_UNREFERENCED_PARAMETER(typeId);                                        \
    addReflectablePtrArrayField(#name,                                        \
                                id,                                           \
                                &MyType::get##name,                           \
                                &MyType::getSize##name,                       \
                                &MyType::set##name,                           \
                                &MyType::setSize##name);                      \
    META_InitPrevEntry(META_Entry_##name());                                  \
  }                                                                           \
                                                                              \
  typedef META_NextEntry_##name
  /***************************************************************************/

  /***************************************************************************/
  /**
   * @brief Ends definitions for member fields with a RTTI type.
   *        Must follow GE_BEGIN_RTTI_MEMBERS.
   */
#define GE_END_RTTI_MEMBERS                                                   \
  META_LastEntry;                                                             \
                                                                              \
  struct META_InitAllMembers                                                  \
  {                                                                           \
    META_InitAllMembers(MyType* owner) {                                      \
      owner->META_InitPrevEntry(META_LastEntry());                            \
    }                                                                         \
  };                                                                          \
                                                                              \
  META_InitAllMembers m_initMembers{this};

  //RTTI-Internal
  struct SerializationContext;
  
  /***************************************************************************/

  /**
   * @brief Provides an interface for accessing fields of a certain class.
   * Data can be easily accessed by getter and setter methods.
   *
   * Supported data types:
   * - Plain types - All types defined in geRTTIField.h, mostly native types
   *   and POD (plain old data) structures. Data is parsed byte by byte.
   *   No pointers to plain types are supported. Data is passed around by value
   *
   * - Reflectable types - Any class deriving from IReflectable. Data is parsed
   *   based on fields in its RTTI class. Can be pointer or value type.
   *
   * - Arrays of both plain and reflectable types are supported
   *
   * - Data blocks - A managed or unmanaged block of data. See ManagedDataBlock
   */
  class GE_UTILITY_EXPORT RTTITypeBase
  {
   public:
    RTTITypeBase() = default;
    virtual ~RTTITypeBase();

    /**
     * @brief Returns RTTI type information for all classes that derive from
     *        the class that owns this RTTI type.
     */
    virtual Vector<RTTITypeBase*>&
    getDerivedClasses() = 0;

    /**
     * @brief Returns RTTI type information for the class that owns this RTTI
     *        type. If the class has not base type, null is returned instead.
     */
    virtual RTTITypeBase*
    getBaseClass() = 0;

    /**
     * @brief Returns true if current RTTI class is derived from @p base.
     *        (Or if it is the same type as base)
     */
    virtual bool
    isDerivedFrom(RTTITypeBase* base) = 0;

    /**
     * @brief Creates a new instance of the class owning this RTTI type.
     */
    virtual SPtr<IReflectable>
    newRTTIObject() = 0;

    /**
     * @brief Returns the name of the class owning this RTTI type.
     */
    virtual const String&
    getRTTIName() = 0;

    /**
     * @brief Returns an RTTI id that uniquely represents each class in the
     *        RTTI system.
     */
    virtual uint32
    getRTTIId() = 0;

    /**
     * @brief Called by the serializers when serialization for this object has
     *        started. Use this to do any preprocessing on data you might need
     *        during serialization itself.
     */
    virtual void
    onSerializationStarted(IReflectable* /*obj*/,
                           SerializationContext* /*context*/)
    {}

    /**
     * @brief Called by the serializers when serialization for this object has
     *        ended. After serialization has ended you can be sure that the
     *        type has been fully serialized, and you may clean up any
     *        temporary data.
     */
    virtual void
    onSerializationEnded(IReflectable* /*obj*/,
                         SerializationContext* /*context*/)
    {}

    /**
     * @brief Called by the serializers when deserialization for this object
     *        has started. Use this to do any preprocessing on data you might
     *        need during deserialization itself.
     */
    virtual void
    onDeserializationStarted(IReflectable* /*obj*/,
                             SerializationContext* /*context*/)
    {}

    /**
     * @brief Called by the serializers when deserialization for this object
     *        has ended. At this point you can be sure the instance has been
     *        fully deserialized and you may safely use it.
     * One exception being are fields you marked with RTTI_Flag_WeakRef, as
     * they might be resolved only after deserialization has fully completed
     * for all objects.
     */
    virtual void
    onDeserializationEnded(IReflectable* /*obj*/,
                           SerializationContext* /*context*/)
    {}

    /**
     * @brief Returns a handler that determines how are "diffs" generated and
     *        applied when it comes to objects of this RTTI type. A "diff" is a
     *        list of differences between two objects that may be saved, viewed
     *        or applied to another object to transform it.
     */
    virtual IDiff&
    getDiffHandler() const {
      static BinaryDiff diffHandler;
      return diffHandler;
    }

    /**
     * @brief Returns the total number of fields in this RTTI type.
     */
    uint32
    getNumFields() const {
      return static_cast<uint32>(m_fields.size());
    }

    /**
     * @brief Returns a field based on the field index.
     *        Use getNumFields() to get total number of fields available.
     */
    RTTIField*
    getField(uint32 idx) {
      return m_fields.at(idx);
    }

    /**
     * @brief Tries to find a field with the specified name.
     *        Throws an exception if it can't.
     * @param name  The name of the field.
     */
    RTTIField*
    findField(const String& name);

    /**
     * @brief Tries to find a field with the specified unique ID.
     *        Doesn't throw an exception if it can't find the field
     *        (Unlike findField(const String&)).
     * @param uniqueFieldId Unique identifier for the field.
     * @return  nullptr if it can't find the field.
     */
    RTTIField*
    findField(int32 uniqueFieldId);

    /**
     * @brief Called by the RTTI system when a class is first found in order to
     *        form child/parent class hierarchy.
     */
    virtual void
    _registerDerivedClass(RTTITypeBase* derivedClass) = 0;

    /**
     * @brief Constructs a cloned version of the underlying class.
     *        The cloned version will not have any field information and should
     *        instead be used for passing to various RTTIField methods during
     *        serialization/deserialization. This allows each object instance
     *        to have a unique places to store temporary instance-specific
     *        data.
     */
    virtual RTTITypeBase*
    _clone(FrameAlloc& alloc) = 0;

   protected:
    /**
     * @brief Tries to add a new field to the fields array, and throws an
              exception if a field with the same name or id already exists.
     * @param[in] field Field, must be non-null.
     */
    void
    addNewField(RTTIField* field);

   private:
    Vector<RTTIField*> m_fields;
  };

  /**
   * @brief Used for initializing a certain type as soon as the program is
   *        loaded.
   */
  template<typename Type, typename BaseType>
  struct InitRTTIOnStart
  {
  public:
    InitRTTIOnStart() {
      IReflectable::_registerRTTIType(Type::getRTTIStatic());
      BaseType::getRTTIStatic()->_registerDerivedClass(Type::getRTTIStatic());
    }

    void makeSureIAmInstantiated() {}
  };

  /**
   * @brief Specialization for root class of RTTI hierarchy - IReflectable
   */
  template<typename Type>
  struct InitRTTIOnStart<Type, IReflectable>
  {
   public:
    InitRTTIOnStart() {
      IReflectable::_registerRTTIType(Type::getRTTIStatic());
    }

    void makeSureIAmInstantiated() {}
  };

  /**
   * @brief Template that returns RTTI type of the specified type, unless the
   *        specified type is IReflectable in which case it returns a null.
   */
  template<typename Type>
  struct GetRTTIType
  {
    RTTITypeBase*
    operator()() {
      return Type::getRTTIStatic();
    }
  };

  /**
   * @brief Specialization for root class of RTTI hierarchy - IReflectable.
   */
  template<>
  struct GetRTTIType<IReflectable>
  {
    RTTITypeBase*
    operator()() {
      return nullptr;
    }
  };

  /**
   * @brief Allows you to provide a run-time type information for a specific
   *        class, along with support for serialization/deserialization.
   * Derive from this class and return the class from IReflectable::getRTTI.
   * This way you can separate serialization logic from the actual class you're
   * serializing. This class will provide a way to register individual fields
   * in the class, together with ways to read and write them, as well as
   * providing information about class hierarchy, and run-time type checking.
   */
  template <typename Type, typename BaseType, typename MyRTTIType>
  class RTTIType : public RTTITypeBase
  {
   public:
    RTTIType() {
      //Compiler will only generate code for stuff that is directly used,
      //including static data members, so we fool it here like we're using the
      //class directly. Otherwise compiler won't generate the code for the
      //member and our type won't get initialized on start
      //(Actual behavior is a bit more random)
      s_initOnStart.makeSureIAmInstantiated();
    }

    virtual ~RTTIType() = default;

    /**
     * @brief Returns a singleton of this RTTI type.
     */
    static MyRTTIType*
    instance() {
      static MyRTTIType inst;
      return &inst;
    }

    /**
     * @copydoc RTTITypeBase::getDerivedClasses
     */
    Vector<RTTITypeBase*>&
    getDerivedClasses() override {
      static Vector<RTTITypeBase*> mRTTIDerivedClasses;
      return mRTTIDerivedClasses;
    }

    /**
     * @copydoc RTTITypeBase::getBaseClass
     */
    RTTITypeBase*
    getBaseClass() override {
      return GetRTTIType<BaseType>()();
    }

    /**
     * @copydoc RTTITypeBase::isDerivedFrom
     */
    bool
    isDerivedFrom(RTTITypeBase* base) override {
      GE_ASSERT(nullptr != base);

      Stack<RTTITypeBase*> todo;
      todo.push(base);

      while (!todo.empty()) {
        RTTITypeBase* currentType = todo.top();
        todo.pop();

        if (currentType->getRTTIId() == getRTTIId()) {
          return true;
        }

        for (const auto& item : currentType->getDerivedClasses()) {
          todo.push(item);
        }
      }

      return false;
    }

    /**
     * @copydoc RTTITypeBase::_registerDerivedClass
     */
    void
    _registerDerivedClass(RTTITypeBase* derivedClass) override {
      getDerivedClasses().push_back(derivedClass);
    }

    /**
     * @copydoc RTTITypeBase::_clone
     */
    RTTITypeBase*
    _clone(FrameAlloc& alloc) override {
      return alloc.construct<MyRTTIType>();
    }

    /*************************************************************************/
    /**
     * Fields operating directly on serializable object
     */
    /*************************************************************************/

   protected:
    using OwnerType = Type;
    using MyType = MyRTTIType;

    /**
     * @brief Registers a new plain field. This field can then be accessed
     *        dynamically from the RTTI system and used for automatic
     *        serialization. See RTTIField for more info about field types.
     * @param[in] name  Name of the field.
     * @param[in] uniqueId  Unique identifier for this field. Although name is
     *            also a unique identifier we want a small data type that can
     *            be used for efficiently serializing data to disk and similar.
     *            It is primarily used for compatibility between different
     *            versions of serialized data.
     * @param[in] getter    Method used for retrieving the value of this field.
     * @param[in] setter    Method used for setting the value of this field.
     * @param[in] flags     Various flags you can use to specialize how systems
     *            handle this field. See RTTIFieldFlag.
     */
    template<class InterfaceType, class ObjectType, class DataType>
    void
    addPlainField(const String& name,
                  uint32 uniqueId,
                  DataType& (InterfaceType::*getter)(ObjectType*),
                  void(InterfaceType::*setter)(ObjectType*, DataType&),
                  uint64 flags = 0) {
      static_assert((is_base_of<geEngineSDK::RTTIType<Type,
                                                      BaseType,
                                                      MyRTTIType>,
                                InterfaceType>::value),
                    "Class with the get/set methods must derive from "
                    "geEngineSDK::RTTIType.");

      static_assert(!(is_base_of<geEngineSDK::IReflectable, DataType>::value),
                    "Data type derives from IReflectable but it is being "
                    "added as a plain field.");

      auto newField = ge_new<RTTIPlainField<InterfaceType, DataType, ObjectType>>();
      newField->initSingle(name,
                           static_cast<uint16>(uniqueId),
                           getter,
                           setter,
                           flags);
      addNewField(newField);
    }

    /**
     * @brief Registers a new reflectable object field. This field can then be
     *        accessed dynamically from the RTTI system and used for automatic
     *        serialization. See RTTIField for more info about field types.
     * @param[in] name  Name of the field.
     * @param[in] uniqueId  Unique identifier for this field. Although name is
     *            also a unique identifier we want a small data type that can
     *            be used for efficiently serializing data to disk and similar.
     *            It is primarily used for compatibility between different
     *            versions of serialized data.
     * @param[in] getter    Method used for retrieving the value of this field.
     * @param[in] setter    Method used for setting the value of this field.
     * @param[in] flags     Various flags you can use to specialize how systems
     *            handle this field. See RTTIFieldFlag.
     */
    template<class InterfaceType, class ObjectType, class DataType>
    void
    addReflectableField(const String& name,
                        uint32 uniqueId,
                        DataType& (InterfaceType::*getter)(ObjectType*),
                        void (InterfaceType::*setter)(ObjectType*, DataType&),
                        uint64 flags = 0) {
      static_assert((is_base_of<geEngineSDK::IReflectable, DataType>::value),
                    "Invalid data type for complex field. "
                    "It needs to derive from bs::IReflectable.");

      auto newField = ge_new<RTTIReflectableField<InterfaceType, DataType, ObjectType>>();
      newField->initSingle(name,
                           static_cast<uint16>(uniqueId),
                           getter,
                           setter,
                           flags);
      addNewField(newField);
    }

    /**
     * @brief Registers a new reflectable object pointer field. This field can
     *        then be accessed dynamically from the RTTI system and used for
     *        automatic serialization. See RTTIField for more info about field
     *        types.
     * @param[in] name  Name of the field.
     * @param[in] uniqueId  Unique identifier for this field. Although name is
     *            also a unique identifier we want a small data type that can
     *            be used for efficiently serializing data to disk and similar.
     *            It is primarily used for compatibility between different
     *            versions of serialized data.
     * @param[in] getter    Method used for retrieving the value of this field.
     * @param[in] setter    Method used for setting the value of this field.
     * @param[in] flags     Various flags you can use to specialize how systems
     *            handle this field. See RTTIFieldFlag.
     */
    template<class InterfaceType, class ObjectType, class DataType>
    void
    addReflectablePtrField(const String& name,
                           uint32 uniqueId,
                           SPtr<DataType>(InterfaceType::*getter)(ObjectType*),
                           void (InterfaceType::*setter)(ObjectType*, SPtr<DataType>),
                           uint64 flags = 0) {
      static_assert((is_base_of<geEngineSDK::IReflectable, DataType>::value),
                    "Invalid data type for complex field. "
                    "It needs to derive from bs::IReflectable.");

      auto newField = ge_new<RTTIReflectablePtrField<InterfaceType, DataType, ObjectType>>();
      newField->initSingle(name,
                           static_cast<uint16>(uniqueId),
                           getter,
                           setter,
                           flags);
      addNewField(newField);
    }

    /**
     * @brief Registers a new field containing an array of plain values. This
     *        field can then be accessed dynamically from the RTTI system and
     *        used for automatic serialization. See RTTIField for more info
     *        about field types.
     * @param[in] name  Name of the field.
     * @param[in] uniqueId  Unique identifier for this field. Although name is
     *            also a unique identifier we want a small data type that can
     *            be used for efficiently serializing data to disk and similar.
     *            It is primarily used for compatibility between different
     *            versions of serialized data.
     * @param[in] getter  Method used for retrieving an element of the array.
     * @param[in] getSize Getter method that returns the size of the array.
     * @param[in] setter  Method for setting a single element of the field.
     * @param[in] setSize Setter method that allows you to resize the array.
     * @param[in] flags   Various flags you can use to specialize how systems
     *            handle this field. See RTTIFieldFlag.
     */
    template<class InterfaceType, class ObjectType, class DataType>
    void
    addPlainArrayField(const String& name,
                       uint32 uniqueId,
                       DataType& (InterfaceType::*getter)(ObjectType*, uint32),
                       uint32(InterfaceType::*getSize)(ObjectType*),
                       void (InterfaceType::*setter)(ObjectType*, uint32, DataType&),
                       void(InterfaceType::*setSize)(ObjectType*, uint32),
                       uint64 flags = 0) {
      static_assert((is_base_of<geEngineSDK::RTTIType<Type, BaseType, MyRTTIType>,
                                InterfaceType>::value),
                    "Class with the get/set methods must derive from "
                    "geEngineSDK::RTTIType.");

      static_assert(!(is_base_of<geEngineSDK::IReflectable, DataType>::value),
                    "Data type derives from IReflectable but it is being "
                    "added as a plain field.");

      auto newField = ge_new<RTTIPlainField<InterfaceType, DataType, ObjectType>>();
      newField->initArray(name,
                          static_cast<uint16>(uniqueId),
                          getter,
                          getSize,
                          setter,
                          setSize,
                          flags);
      addNewField(newField);
    }

    /**
     * @brief Registers a new field containing an array of reflectable object
     *        values. This field can then be accessed dynamically from the RTTI
     *        system and used for automatic serialization. See RTTIField for
     *        more info about field types.
     * @param[in] name  Name of the field.
     * @param[in] uniqueId  Unique identifier for this field. Although name is
     *            also a unique identifier we want a small data type that can
     *            be used for efficiently serializing data to disk and similar.
     *            It is primarily used for compatibility between different
     *            versions of serialized data.
     * @param[in] getter  Method used for retrieving an element of the array.
     * @param[in] getSize Getter method that returns the size of the array.
     * @param[in] setter  Method for setting a single element of the field.
     * @param[in] setSize Setter method that allows you to resize the array.
     * @param[in] flags   Various flags you can use to specialize how systems
     *            handle this field. See RTTIFieldFlag.
     */
    template<class InterfaceType, class ObjectType, class DataType>
    void
    addReflectableArrayField(const String& name,
                             uint32 uniqueId,
                             DataType& (InterfaceType::*getter)(ObjectType*, uint32),
                             uint32(InterfaceType::*getSize)(ObjectType*),
                             void (InterfaceType::*setter)(ObjectType*, uint32, DataType&),
                             void(InterfaceType::*setSize)(ObjectType*, uint32),
                             uint64 flags = 0) {
      static_assert((is_base_of<geEngineSDK::IReflectable, DataType>::value),
                    "Invalid data type for complex field. "
                    "It needs to derive from bs::IReflectable.");

      auto newField = ge_new<RTTIReflectableField<InterfaceType, DataType, ObjectType>>();
      newField->initArray(name,
                          static_cast<uint16>(uniqueId),
                          getter,
                          getSize,
                          setter,
                          setSize,
                          flags);
      addNewField(newField);
    }

    /**
     * @brief Registers a new field containing an array of reflectable object
     *        pointers. This field can then be accessed dynamically from the
     *        RTTI system and used for automatic serialization. See RTTIField
     *        for more info about field types.
     * @param[in] name  Name of the field.
     * @param[in] uniqueId  Unique identifier for this field. Although name is
     *            also a unique identifier we want a small data type that can
     *            be used for efficiently serializing data to disk and similar.
     *            It is primarily used for compatibility between different
     *            versions of serialized data.
     * @param[in] getter  Method used for retrieving an element of the array.
     * @param[in] getSize Getter method that returns the size of the array.
     * @param[in] setter  Method for setting a single element of the field.
     * @param[in] setSize Setter method that allows you to resize the array.
     * @param[in] flags   Various flags you can use to specialize how systems
     *            handle this field. See RTTIFieldFlag.
     */
    template<class InterfaceType, class ObjectType, class DataType>
    void
    addReflectablePtrArrayField(const String& name,
                                uint32 uniqueId,
                                SPtr<DataType>(InterfaceType::*getter)(ObjectType*, uint32),
                                uint32(InterfaceType::*getSize)(ObjectType*),
                                void(InterfaceType::*setter)(ObjectType*,
                                                             uint32,
                                                             SPtr<DataType>),
                                void(InterfaceType::*setSize)(ObjectType*, uint32),
                                uint64 flags = 0) {
      static_assert((is_base_of<geEngineSDK::IReflectable, DataType>::value),
                    "Invalid data type for complex field. "
                    "It needs to derive from bs::IReflectable.");

      auto newField = ge_new<RTTIReflectablePtrField<InterfaceType, DataType, ObjectType>>();
      newField->initArray(name,
                          static_cast<uint16>(uniqueId),
                          getter,
                          getSize,
                          setter,
                          setSize,
                          flags);
      addNewField(newField);
    }

    /**
     * @brief Registers a new managed data block field. This field can then be
     *        accessed dynamically from the RTTI system and used for automatic
     *        serialization. See RTTIField for more info about field types.
     * @param[in] name  Name of the field.
     * @param[in] uniqueId  Unique identifier for this field. Although name is
     *            also a unique identifier we want a small data type that can
     *            be used for efficiently serializing data to disk and similar.
     *            It is primarily used for compatibility between different
     *            versions of serialized data.
     * @param[in] getter  Method used for retrieving an element of the array.
     * @param[in] getSize Getter method that returns the size of the array.
     * @param[in] setter  Method for setting a single element of the field.
     * @param[in] setSize Setter method that allows you to resize the array.
     * @param[in] flags   Various flags you can use to specialize how systems
     *            handle this field. See RTTIFieldFlag.
     */
    template<class InterfaceType, class ObjectType>
    void
    addDataBlockField(const String& name,
                      uint32 uniqueId,
                      SPtr<DataStream>(InterfaceType::*getter)(ObjectType*, uint32&),
                      void(InterfaceType::*setter)(ObjectType*,
                                                   const SPtr<DataStream>&,
                                                   uint32),
                      uint64 flags = 0) {
      auto newField = ge_new<RTTIManagedDataBlockField<InterfaceType,
                                                       uint8*,
                                                       ObjectType>>();
      newField->initSingle(name,
                           static_cast<uint16>(uniqueId),
                           getter,
                           setter,
                           flags);
      addNewField(newField);
    }
   
   protected:
    static InitRTTIOnStart<Type, BaseType> s_initOnStart;
  };

  template<typename Type, typename BaseType, typename MyRTTIType>
  InitRTTIOnStart<Type, BaseType> RTTIType<Type, BaseType, MyRTTIType>::s_initOnStart;

  /**
   * @brief Extendable class to be used by the user to provide extra
   *        information to RTTIType objects during serialization.
   */
  struct GE_UTILITY_EXPORT SerializationContext : IReflectable
  {
    uint32 flags = 0;

    static RTTITypeBase*
    getRTTIStatic();

    RTTITypeBase*
    getRTTI() const override;
  };

  /**
   * @brief Returns true if the provided object can be safely cast into type T.
   */
  template<class T>
  bool
  rtti_is_of_type(IReflectable* object) {
    static_assert( (is_base_of<geEngineSDK::IReflectable, T>::value),
                   "Invalid data type for type checking. It needs to derive "
                   "from geEngineSDK::IReflectable.");

    return object->getTypeId() == T::getRTTIStatic()->getRTTIId();
  }

  /**
   * @brief Returns true if the provided object can be safely cast into type T.
   */
  template<class T>
  bool
  rtti_is_of_type(SPtr<IReflectable> object) {
    static_assert( (is_base_of<geEngineSDK::IReflectable, T>::value),
                   "Invalid data type for type checking. It needs to derive "
                   "from geEngineSDK::IReflectable.");

    return object->getTypeId() == T::getRTTIStatic()->getRTTIId();
  }

  /**
   * @brief Creates a new object just from its type ID.
   */
  GE_UTILITY_EXPORT SPtr<IReflectable>
  rtti_create(uint32 rttiId);

  /**
   * @brief Checks is the current object a subclass of some type.
   */
  template<class T>
  bool
  rtti_is_subclass(IReflectable* object) {
    static_assert( (is_base_of<geEngineSDK::IReflectable, T>::value),
                   "Invalid data type for type checking. It needs to derive "
                   "from geEngineSDK::IReflectable.");

    return object->isDerivedFrom(T::getRTTIStatic());
  }

  /**
   * @brief Attempts to cast the object to the provided type,
   *        or returns null if cast is not valid.
   */
  template<class T>
  T*
  rtti_cast(IReflectable* object) {
    if (rtti_is_subclass<T>(object)) {
      return reinterpret_cast<T*>(object);
    }

    return nullptr;
  }
}

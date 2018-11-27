/*****************************************************************************/
/**
 * @file    gePrerequisitesCore.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/01
 * @brief   Provides core engine functionality.
 *
 * Second lowest layer that provides core engine functionality and abstract
 * interfaces for various systems.
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
#include <gePrerequisitesUtil.h>

/**
 * Maximum number of surfaces that can be attached to a multi render target.
 */
#define GE_MAX_MULTIPLE_RENDER_TARGETS 8
#define GE_FORCE_SINGLETHREADED_RENDERING 0

/**
 * Maximum number of individual GPU queues, per type.
 */
#define GE_MAX_QUEUES_PER_TYPE 8

/**
 * Maximum number of hardware devices usable at once.
 */
#define GE_MAX_DEVICES 5U

/**
 * Maximum number of devices one resource can exist at the same time.
 */
#define GE_MAX_LINKED_DEVICES 4U

//DLL export
#if GE_PLATFORM == GE_PLATFORM_WIN32
# if GE_COMPILER == GE_COMPILER_MSVC
#   if defined( GE_STATIC_LIB )
#     define GE_CORE_EXPORT
#   else
#     if defined( GE_CORE_EXPORTS )
#       define GE_CORE_EXPORT __declspec( dllexport )
#     else
#       define GE_CORE_EXPORT __declspec( dllimport )
#     endif
#   endif
# else  //Any other Compiler
#   if defined( GE_STATIC_LIB )
#     define GE_CORE_EXPORT
#   else
#     if defined( GE_CORE_EXPORTS )
#       define GE_CORE_EXPORT __attribute__ ((dllexport))
#     else
#       define GE_CORE_EXPORT __attribute__ ((dllimport))
#     endif
#   endif
# endif
# define GE_CORE_HIDDEN
#else //Linux/Mac settings
# define GE_CORE_EXPORT __attribute__ ((visibility ("default")))
# define GE_CORE_HIDDEN __attribute__ ((visibility ("hidden")))
#endif

#include <geStringID.h>
#include "geHString.h"

namespace geEngineSDK {
  /***************************************************************************/
  /*
   * Core objects
   */
  /***************************************************************************/
  template<class T>
  struct CoreThreadType {};

#define CORE_OBJECT_FORWARD_DECLARE(TYPE)                                     \
	class TYPE;                                                                 \
	namespace geCoreThread { class TYPE; }                                      \
	template<>                                                                  \
  struct CoreThreadType<TYPE> { typedef geCoreThread::TYPE Type; };

  CORE_OBJECT_FORWARD_DECLARE(IndexBuffer)
  CORE_OBJECT_FORWARD_DECLARE(VertexBuffer)
  CORE_OBJECT_FORWARD_DECLARE(GPUBuffer)
  CORE_OBJECT_FORWARD_DECLARE(GPUProgram)
  CORE_OBJECT_FORWARD_DECLARE(Pass)
  CORE_OBJECT_FORWARD_DECLARE(Technique)
  CORE_OBJECT_FORWARD_DECLARE(Shader)
  CORE_OBJECT_FORWARD_DECLARE(Material)
  CORE_OBJECT_FORWARD_DECLARE(RenderTarget)
  CORE_OBJECT_FORWARD_DECLARE(RenderTexture)
  CORE_OBJECT_FORWARD_DECLARE(RenderWindow)
  CORE_OBJECT_FORWARD_DECLARE(SamplerState)
  CORE_OBJECT_FORWARD_DECLARE(Viewport)
  CORE_OBJECT_FORWARD_DECLARE(VertexDeclaration)
  CORE_OBJECT_FORWARD_DECLARE(DepthStencilState)
  CORE_OBJECT_FORWARD_DECLARE(RasterizerState)
  CORE_OBJECT_FORWARD_DECLARE(BlendState)
  CORE_OBJECT_FORWARD_DECLARE(GPUParamBlockBuffer)
  CORE_OBJECT_FORWARD_DECLARE(GPUParams)
  CORE_OBJECT_FORWARD_DECLARE(GPUParamsSet)
  CORE_OBJECT_FORWARD_DECLARE(MaterialParams)
  CORE_OBJECT_FORWARD_DECLARE(Light)
  CORE_OBJECT_FORWARD_DECLARE(Camera)
  CORE_OBJECT_FORWARD_DECLARE(Renderable)
  CORE_OBJECT_FORWARD_DECLARE(GraphicsPipelineState)
  CORE_OBJECT_FORWARD_DECLARE(ComputePipelineState)
  CORE_OBJECT_FORWARD_DECLARE(ReflectionProbe)
  CORE_OBJECT_FORWARD_DECLARE(ParticleSystem)
  CORE_OBJECT_FORWARD_DECLARE(Texture)
  CORE_OBJECT_FORWARD_DECLARE(SpriteTexture)
  CORE_OBJECT_FORWARD_DECLARE(Mesh)
  CORE_OBJECT_FORWARD_DECLARE(VectorField)
  CORE_OBJECT_FORWARD_DECLARE(Skybox)
  CORE_OBJECT_FORWARD_DECLARE(Decal)

  class Collider;
  class Rigidbody;
  class BoxCollider;
  class SphereCollider;
  class PlaneCollider;
  class CapsuleCollider;
  class MeshCollider;
  class Joint;
  class FixedJoint;
  class DistanceJoint;
  class HingeJoint;
  class SphericalJoint;
  class SliderJoint;
  class D6Joint;
  class CharacterController;
  class AudioListener;
  class AudioSource;
  class Animation;
  class Bone;
  class LightProbeVolume;

  /***************************************************************************/
  /*
   * Components
   */
  /***************************************************************************/
  template<class T>
  struct ComponentType {};

#define COMPONENT_FORWARD_DECLARE(TYPE)                                       \
	class C##TYPE;                                                              \
	template<>                                                                  \
  struct ComponentType<TYPE> { typedef C##TYPE Type; };

  COMPONENT_FORWARD_DECLARE(Collider)
  COMPONENT_FORWARD_DECLARE(Rigidbody)
  COMPONENT_FORWARD_DECLARE(BoxCollider)
  COMPONENT_FORWARD_DECLARE(SphereCollider)
  COMPONENT_FORWARD_DECLARE(PlaneCollider)
  COMPONENT_FORWARD_DECLARE(CapsuleCollider)
  COMPONENT_FORWARD_DECLARE(MeshCollider)
  COMPONENT_FORWARD_DECLARE(Joint)
  COMPONENT_FORWARD_DECLARE(HingeJoint)
  COMPONENT_FORWARD_DECLARE(DistanceJoint)
  COMPONENT_FORWARD_DECLARE(FixedJoint)
  COMPONENT_FORWARD_DECLARE(SphericalJoint)
  COMPONENT_FORWARD_DECLARE(SliderJoint)
  COMPONENT_FORWARD_DECLARE(D6Joint)
  COMPONENT_FORWARD_DECLARE(CharacterController)
  COMPONENT_FORWARD_DECLARE(Camera)
  COMPONENT_FORWARD_DECLARE(Renderable)
  COMPONENT_FORWARD_DECLARE(Light)
  COMPONENT_FORWARD_DECLARE(Animation)
  COMPONENT_FORWARD_DECLARE(Bone)
  COMPONENT_FORWARD_DECLARE(AudioSource)
  COMPONENT_FORWARD_DECLARE(AudioListener)
  COMPONENT_FORWARD_DECLARE(ReflectionProbe)
  COMPONENT_FORWARD_DECLARE(Skybox)
  COMPONENT_FORWARD_DECLARE(LightProbeVolume)
  COMPONENT_FORWARD_DECLARE(ParticleSystem)
  COMPONENT_FORWARD_DECLARE(Decal)

  class Color;
  class GPUProgramManager;
  class GPUProgramManager;
  class GPUProgramFactory;
  class IndexData;
  class RenderAPICapabilities;
  class RenderTargetProperties;
  class TextureManager;
  class Input;
  struct PointerEvent;
  class RendererFactory;
  class AsyncOp;
  class HardwareBufferManager;
  class FontManager;
  class RenderStateManager;
  class GPUParamBlock;
  struct GPUParamDesc;
  struct GPUParamDataDesc;
  struct GPUParamObjectDesc;
  struct GPUParamBlockDesc;
  class ShaderInclude;
  class CoreObject;
  class ImportOptions;
  class TextureImportOptions;
  class FontImportOptions;
  class GPUProgramImportOptions;
  class MeshImportOptions;
  struct FontBitmap;
  class GameObject;
  class GPUResourceData;
  struct RenderOperation;
  class RenderQueue;
  struct ProfilerReport;
  class VertexDataDesc;
  class FrameAlloc;
  class FolderMonitor;
  class VideoMode;
  class VideoOutputInfo;
  class VideoModeInfo;
  struct SubMesh;
  class IResourceListener;
  class TextureProperties;
  class IShaderIncludeHandler;
  class Prefab;
  class PrefabDiff;
  class RendererMeshData;
  class Win32Window;
  class RenderAPIFactory;
  class PhysicsManager;
  class Physics;
  class FCollider;
  class PhysicsMaterial;
  class ShaderDefines;
  class ShaderImportOptions;
  class AudioClipImportOptions;
  class AnimationClip;
  class GPUPipelineParamInfo;
  template<class T> class TAnimationCurve;
  struct AnimationCurves;
  class Skeleton;
  class MorphShapes;
  class MorphShape;
  class MorphChannel;
  class Transform;
  class SceneActor;
  class CoreObjectManager;
  struct CollisionData;
  
  //Asset import
  class SpecificImporter;
  class Importer;
  
  //Resources
  class Resource;
  class Resources;
  class ResourceManifest;
  class MeshBase;
  class TransientMesh;
  class MeshHeap;
  class Font;
  class ResourceMetaData;
  class DropTarget;
  class StringTable;
  class PhysicsMaterial;
  class PhysicsMesh;
  class AudioClip;
  
  //Scene
  class SceneObject;
  class Component;
  class SceneManager;
  
  //RTTI
  class MeshRTTI;
  
  //Desc structs
  struct SAMPLER_STATE_DESC;
  struct DEPTH_STENCIL_STATE_DESC;
  struct RASTERIZER_STATE_DESC;
  struct BLEND_STATE_DESC;
  struct RENDER_TARGET_BLEND_STATE_DESC;
  struct RENDER_TEXTURE_DESC;
  struct RENDER_WINDOW_DESC;
  struct FONT_DESC;
  struct CHAR_CONTROLLER_DESC;
  struct JOINT_DESC;
  struct FIXED_JOINT_DESC;
  struct DISTANCE_JOINT_DESC;
  struct HINGE_JOINT_DESC;
  struct SLIDER_JOINT_DESC;
  struct SPHERICAL_JOINT_DESC;
  struct D6_JOINT_DESC;
  struct AUDIO_CLIP_DESC;

  template<class T>
  class TCoreThreadQueue;
  class CommandQueueNoSync;
  class CommandQueueSync;

  namespace geCoreThread {
    class Renderer;
    class VertexData;
    class RenderAPI;
    class CoreObject;
    class MeshBase;
    class TransientMesh;
    class MeshHeap;
    class GPUPipelineParamInfo;
    class CommandBuffer;
    class EventQuery;
    class TimerQuery;
    class OcclusionQuery;
    class TextureView;
    class RenderElement;
    class RenderWindowManager;
    class RenderStateManager;
    class HardwareBufferManager;
  }
}

namespace geEngineSDK {
  using CoreThreadQueue = TCoreThreadQueue<CommandQueueNoSync>;
}

namespace geEngineSDK {
  namespace TYPEID_CORE {
    enum E {
      kID_Texture = 1001,
      kID_Mesh = 1002,
      kID_MeshData = 1003,
      kID_VertexDeclaration = 1004,
      kID_VertexElementData = 1005,
      kID_Component = 1006,
      kID_ResourceHandle = 1009,
      kID_GPUProgram = 1010,
      kID_ResourceHandleData = 1011,
      kID_CgProgram = 1012,
      kID_Pass = 1014,
      kID_Technique = 1015,
      kID_Shader = 1016,
      kID_Material = 1017,
      kID_SamplerState = 1021,
      kID_BlendState = 1023,
      kID_RasterizerState = 1024,
      kID_DepthStencilState = 1025,
      kID_BLEND_STATE_DESC = 1034,
      kID_SHADER_DATA_PARAM_DESC = 1035,
      kID_SHADER_OBJECT_PARAM_DESC = 1036,
      kID_SHADER_PARAM_BLOCK_DESC = 1047,
      kID_ImportOptions = 1048,
      kID_Font = 1051,
      kID_FONT_DESC = 1052,
      kID_CHAR_DESC = 1053,
      kID_FontImportOptions = 1056,
      kID_FontBitmap = 1057,
      kID_SceneObject = 1059,
      kID_GameObject = 1060,
      kID_PixelData = 1062,
      kID_GPUResourceData = 1063,
      kID_VertexDataDesc = 1064,
      kID_MeshBase = 1065,
      kID_GameObjectHandleBase = 1066,
      kID_ResourceManifest = 1067,
      kID_ResourceManifestEntry = 1068,
      kID_EmulatedParamBlock = 1069,
      kID_TextureImportOptions = 1070,
      kID_ResourceMetaData = 1071,
      kID_ShaderInclude = 1072,
      kID_Viewport = 1073,
      kID_ResourceDependencies = 1074,
      kID_ShaderMetaData = 1075,
      kID_MeshImportOptions = 1076,
      kID_Prefab = 1077,
      kID_PrefabDiff = 1078,
      kID_PrefabObjectDiff = 1079,
      kID_PrefabComponentDiff = 1080,
      kID_CGUIWidget = 1081,
      ///kID_ProfilerOverlay = 1082,
      kID_StringTable = 1083,
      kID_LanguageData = 1084,
      kID_LocalizedStringData = 1085,
      kID_MaterialParamColor = 1086,
      kID_WeakResourceHandle = 1087,
      kID_TextureParamData = 1088,
      kID_StructParamData = 1089,
      kID_MaterialParams = 1090,
      kID_MaterialRTTIParam = 1091,
      kID_PhysicsMaterial = 1092,
      kID_CCollider = 1093,
      kID_CBoxCollider = 1094,
      kID_CSphereCollider = 1095,
      kID_CCapsuleCollider = 1096,
      kID_CPlaneCollider = 1097,
      kID_CRigidbody = 1098,
      kID_PhysicsMesh = 1099,
      kID_CMeshCollider = 1100,
      kID_CJoint = 1101,
      kID_CFixedJoint = 1102,
      kID_CDistanceJoint = 1103,
      kID_CHingeJoint = 1104,
      kID_CSphericalJoint = 1105,
      kID_CSliderJoint = 1106,
      kID_CD6Joint = 1107,
      kID_CCharacterController = 1108,
      kID_FPhysicsMesh = 1109,
      kID_ShaderImportOptions = 1110,
      kID_AudioClip = 1111,
      kID_AudioClipImportOptions = 1112,
      kID_CAudioListener = 1113,
      kID_CAudioSource = 1114,
      kID_AnimationClip = 1115,
      kID_AnimationCurve = 1116,
      kID_KeyFrame = 1117,
      kID_NamedAnimationCurve = 1118,
      kID_Skeleton = 1119,
      kID_SkeletonBoneInfo = 1120,
      kID_AnimationSplitInfo = 1121,
      kID_CAnimation = 1122,
      kID_AnimationEvent = 1123,
      kID_ImportedAnimationEvents = 1124,
      kID_CBone = 1125,
      kID_MaterialParamData = 1126,
      kID_RenderSettings = 1127,
      kID_MorphShape = 1128,
      kID_MorphShapes = 1129,
      kID_MorphChannel = 1130,
      kID_ReflectionProbe = 1131,
      kID_CReflectionProbe = 1132,
      kID_CachedTextureData = 1133,
      kID_Skybox = 1134,
      kID_CSkybox = 1135,
      kID_LightProbeVolume = 1136,
      kID_SavedLightProbeInfo = 1137,
      kID_CLightProbeVolume = 1138,

      kID_SceneActor = 1140,
      kID_AudioListener = 1141,
      kID_AudioSource = 1142,
      kID_ShaderVariationParam = 1143,
      kID_ShaderVariation = 1144,
      kID_GPUProgramBytecode = 1145,
      kID_GPUParamBlockDesc = 1146,
      kID_GPUParamDataDesc = 1147,
      kID_GPUParamObjectDesc = 1148,
      kID_GPUParamDesc = 1149,
      kID_RasterizerStateDesc = 1151,
      kID_DepthStencilStateDesc = 1152,
      kID_SerializedGPUProgramData = 1153,
      kID_SubShader = 1154,

      kID_ParticleSystem = 1155,
      kID_ColorDistribution = 1156,
      kID_TDistribution = 1157,
      kID_SHADER_PARAM_ATTRIBUTE = 1158,
      kID_DataParamInfo = 1159,
      kID_SpriteSheetGridAnimation = 1160,
      kID_ParticleEmitter = 1161,
      kID_ParticleEmitterConeShape = 1162,
      kID_ParticleEmitterSphereShape = 1163,
      kID_ParticleEmitterHemisphereShape = 1164,
      kID_ParticleEmitterBoxShape = 1165,
      kID_ParticleEmitterCircleShape = 1166,
      kID_ParticleEmitterRectShape = 1167,
      kID_ParticleEmitterLineShape = 1168,
      kID_ParticleEmitterStaticMeshShape = 1169,
      kID_ParticleEmitterSkinnedMeshShape = 1170,
      kID_ParticleTextureAnimation = 1171,
      kID_ParticleCollisions = 1172,
      kID_ParticleOrbit = 1173,
      kID_ParticleVelocity = 1174,
      kID_ParticleSystemSettings = 1175,
      kID_ParticleSystemEmitters = 1176,
      kID_ParticleSystemEvolvers = 1177,
      kID_CParticleSystem = 1178,
      kID_ParticleGravity = 1179,
      kID_VectorField = 1180,
      kID_ParticleVectorFieldSettings = 1181,
      kID_ParticleGpuSimulationSettings = 1182,
      kID_ParticleDepthCollisionSettings = 1183,
      kID_BloomSettings = 1184,
      kID_ParticleBurst = 1185,
      kID_CoreSerializationContext = 1186,
      kID_ParticleForce = 1187,
      kID_ParticleSize = 1188,
      kID_ParticleColor = 1189,
      kID_ParticleRotation = 1190,
      kID_Decal = 1191,
      kID_CDecal = 1192,

      //Moved from Engine layer
      kID_CCamera = 30000,
      kID_Camera = 30003,
      kID_CRenderable = 30001,
      kID_Renderable = 30004,
      kID_Light = 30011,
      kID_CLight = 30012,
      kID_AutoExposureSettings = 30016,
      kID_TonemappingSettings = 30017,
      kID_WhiteBalanceSettings = 30018,
      kID_ColorGradingSettings = 30019,
      kID_DepthOfFieldSettings = 30020,
      kID_AmbientOcclusionSettings = 30021,
      kID_ScreenSpaceReflectionsSettings = 30022,
      kID_ShadowSettings = 30023
    };
  }
}

/*****************************************************************************/
/**
 * Resource references
 */
/*****************************************************************************/

#include "geResourceHandle.h"

namespace geEngineSDK {
  using HResource = ResourceHandle<Resource>;
  using HStringTable = ResourceHandle<StringTable>;
  using HFont = ResourceHandle<Font>;
  using HTexture = ResourceHandle<Texture>;
  using HMesh = ResourceHandle<Mesh>;
  using HMaterial = ResourceHandle<Material>;
  using HShaderInclude = ResourceHandle<ShaderInclude>;
  using HShader = ResourceHandle<Shader>;
  using HPrefab = ResourceHandle<Prefab>;
  using HPhysicsMaterial = ResourceHandle<PhysicsMaterial>;
  using HPhysicsMesh = ResourceHandle<PhysicsMesh>;
  using HAnimationClip = ResourceHandle<AnimationClip>;
  using HAudioClip = ResourceHandle<AudioClip>;
  using HSpriteTexture = ResourceHandle<SpriteTexture>;
  using HVectorField = ResourceHandle<VectorField>;
}

#include "geGameObjectHandle.h"

namespace geEngineSDK {
  //Game object handles
  using HGameObject = GameObjectHandle<GameObject>;
  using HSceneObject = GameObjectHandle<SceneObject>;
  using HComponent = GameObjectHandle<Component>;
  using HCamera = GameObjectHandle<CCamera>;
  using HRenderable = GameObjectHandle<CRenderable>;
  using HLight = GameObjectHandle<CLight>;
  using HAnimation = GameObjectHandle<CAnimation>;
  using HBone = GameObjectHandle<CBone>;
  using HRigidbody = GameObjectHandle<CRigidbody>;
  using HCollider = GameObjectHandle<CCollider>;
  using HBoxCollider = GameObjectHandle<CBoxCollider>;
  using HSphereCollider = GameObjectHandle<CSphereCollider>;
  using HCapsuleCollider = GameObjectHandle<CCapsuleCollider>;
  using HPlaneCollider = GameObjectHandle<CPlaneCollider>;
  using HJoint = GameObjectHandle<CJoint>;
  using HHingeJoint = GameObjectHandle<CHingeJoint>;
  using HSliderJoint = GameObjectHandle<CSliderJoint>;
  using HDistanceJoint = GameObjectHandle<CDistanceJoint>;
  using HSphericalJoint = GameObjectHandle<CSphericalJoint>;
  using HFixedJoint = GameObjectHandle<CFixedJoint>;
  using HD6Joint = GameObjectHandle<CD6Joint>;
  using HCharacterController = GameObjectHandle<CCharacterController>;
  using HReflectionProbe = GameObjectHandle<CReflectionProbe>;
  using HSkybox = GameObjectHandle<CSkybox>;
  using HLightProbeVolume = GameObjectHandle<CLightProbeVolume>;
  using HAudioSource = GameObjectHandle<CAudioSource>;
  using HAudioListener = GameObjectHandle<CAudioListener>;
  using HParticleSystem = GameObjectHandle<CParticleSystem>;
  using HDecal = GameObjectHandle<CDecal>;
}

namespace geEngineSDK {
  using std::basic_string;
  using std::char_traits;

  /**
   * @brief Defers function execution until the next frame. If this function is
   *        called within another deferred call, then it will be executed the
   *        same frame, but only after all existing deferred calls are done.
   * @note  This method can be used for breaking dependencies among other
   *        things. If a class A depends on class B having something done, but
   *        class B also depends in some way on class A, you can break up the
   *        initialization into two separate steps, queuing the second step
   *        using this method.
   * @note  Similar situation can happen if you have multiple classes being
   *        initialized in an undefined order but some of them depend on
   *        others. Using this method you can defer the dependent step until
   *        next frame, which will ensure everything was initialized.
   * @param[in]  callback  The callback.
   */
  GE_CORE_EXPORT void
  deferredCall(std::function<void()> callback);

  //Special types for use by profilers
  using ProfilerString = basic_string<char, char_traits<char>, StdAlloc<char, ProfilerAlloc>>;

  template<typename T, typename A = StdAlloc<T, ProfilerAlloc>>
  using ProfilerVector = std::vector<T, A>;

  template <typename T, typename A = StdAlloc<T, ProfilerAlloc>>
  using ProfilerStack = std::stack<T, std::deque<T, A>>;

  /**
   * @brief geEngine thread policy that performs special startup / shutdown on
   *        threads managed by thread pool.
   */
  class GE_CORE_EXPORT geEngineThreadPolicy
  {
   public:
    static void
    onThreadStarted(const String& /*name*/) {
      MemStack::beginThread();
    }

    static void
    onThreadEnded(const String& /*name*/) {
      MemStack::endThread();
    }
  };

# define GE_ALL_LAYERS 0xFFFFFFFFFFFFFFFF

  /**
   * @brief Used for marking a CoreObject dependency as dirty.
   */
  static constexpr int32 DIRTY_DEPENDENCY_MASK = 1 << 31;

  template<class T, bool Core>
  struct CoreVariant {};

  template<class T>
  struct CoreVariant<T, false>
  {
    using Type = T;
  };

  template<class T>
  struct CoreVariant<T, true>
  {
    using Type = typename CoreThreadType<T>::Type;
  };

  /**
   * @brief Allows a simple way to define a member that can be both CoreObject
   *        variants depending on the Core template parameter.
   */
  template<class T, bool Core>
  using CoreVariantType = typename CoreVariant<T, Core>::Type;

  template<class T, bool Core>
  struct CoreVariantHandle {};

  template<class T>
  struct CoreVariantHandle<T, false>
  {
    using Type = ResourceHandle<T>;
  };

  template<class T>
  struct CoreVariantHandle<T, true>
  {
    using Type = SPtr<typename CoreThreadType<T>::Type>;
  };

  /**
   * @brief Allows a simple way to define a member that can be both CoreObject
   *        variants depending on the Core template parameter. Sim thread type
   *        is wrapped in as a resource handle while the core thread variant is
   *        wrapped in a shared pointer.
   */
  template<class T, bool Core>
  using CoreVariantHandleType = typename CoreVariantHandle<T, Core>::Type;

  /**
   * @brief Flags that are provided to the serialization system to control
   *        serialization/deserialization.
   */
  namespace SERIALIZATION_FLAGS {
    enum E {
      /**
       * Used when deserializing resources. Lets the system know not to discard
       * any intermediate resource data that might be required if the resource
       * needs to be serialized.
       */
      kKeepResourceSourceData
    };
  }

  /**
   * @brief Helper type that can contain either a component or scene actor
   *        version of an object.
   */
  template<class T>
  struct ComponentOrActor
  {
    using ComponentType = typename ComponentType<T>::Type;
    using HandleType = GameObjectHandle<ComponentType>;

    ComponentOrActor() = default;

    ComponentOrActor(const GameObjectHandle<ComponentType>& component)
      : m_component(component)
    {}

    ComponentOrActor(const SPtr<T>& actor)
      : m_actor(actor)
    {}

    /**
     * @brief Returns true if both the component and the actor fields are not
     *        assigned.
     */
    bool
    empty() const {
      return !m_actor && !m_component;
    }

    /**
     * Returns the assigned value as a scene actor.
     */
    SPtr<T>
    getActor() const {
      if (m_actor) {
        return m_actor;
      }
      return m_component->_getInternal();
    }

    /**
     * @brief Returns the assigned value as a component.
     */
    HandleType
    getComponent() const {
      return m_component;
    }

   private:
    GameObjectHandle<ComponentType> m_component;
    SPtr<T> m_actor;
  };
}

#include "geCommonTypes.h"

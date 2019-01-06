/*****************************************************************************/
/**
 * @file    geCommonTypes.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/25
 * @brief   Common definitions for core objects.
 *
 * Common definitions for core objects.
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

namespace geEngineSDK {
  /**
   * @brief Factors used when blending new pixels with existing pixels.
   */
  namespace BLEND_FACTOR {
    enum E {
      kONE,               //Use a value of one for all pixel components.
      kZERO,              //Use a value of zero for all pixel components.
      kDEST_COLOR,        //Use the existing pixel value.
      kSOURCE_COLOR,      //Use the newly generated pixel value.
      kINV_DEST_COLOR,    //Use the inverse of the existing value.
      kINV_SOURCE_COLOR,  //Use the inverse of the newly generated pixel value.
      kDEST_ALPHA,        //Use the existing alpha value.
      kSOURCE_ALPHA,      //Use the newly generated alpha value.
      kINV_DEST_ALPHA,    //Use the inverse of the existing alpha value.
      kINV_SOURCE_ALPHA   //Use the inverse of the newly generated alpha value.
    };
  }

  /**
   * @brief Operations that determines how are blending factors combined.
   */
  namespace BLEND_OPERATION {
    enum E {
      //Blend factors are added together.
      kADD,

      //Blend factors are subtracted in "srcFactor - dstFactor" order.
      kSUBTRACT,

      //Blend factors are subtracted in "dstFactor - srcFactor" order.
      kREVERSE_SUBTRACT,

      //Minimum of the two factors is chosen.
      kMIN,

      //Maximum of the two factors is chosen.0
      kMAX
    };
  }

  /**
   * @brief Comparison functions used for the depth/stencil buffer.
   */
  namespace COMPARE_FUNCTION {
    enum E {
      /**
       * Operation will always fail.
       */
      kALWAYS_FAIL,

      /**
       * Operation will always pass.
       */
      kALWAYS_PASS,

      /**
       * Operation will pass if the new value is less than existing value.
       */
      kLESS,

      /**
       * Operation will pass if the new value is less or equal than existing
       * value
       */
      kLESS_EQUAL,

      /**
       * Operation will pass if the new value is equal to the existing value.
       */
      kEQUAL,

      /**
       * Operation will pass if the new value is not equal to the existing
       * value.
       */
      kNOT_EQUAL,

      /**
       * Operation will pass if the new value greater or equal than the existing value.
       */
      kGREATER_EQUAL,

      /**
       * Operation will pass if the new value greater than the existing value.
       */
      kGREATER
    };
  }

  /**
   * @brief Types of texture addressing modes that determine what happens when
   *        texture coordinates are outside of the valid range.
   */
  namespace TEXTURE_ADDRESSING_MODE {
    enum E {
      /**
       * Coordinates wrap back to the valid range.
       */
      kWRAP,

      /**
       * Coordinates flip every time the size of the valid range is passed.
       */
      kMIRROR,

      /**
       * Coordinates are clamped within the valid range.
       */
      kCLAMP,

      /**
       * Coordinates outside of the valid range will return a separately set
       * border color.
       */
      kBORDER
    };
  }

  /**
   * @brief Types of available filtering situations.
   */
  namespace FILTER_TYPE {
    enum E {
      /**
       * The filter used when shrinking a texture.
       */
      kMIN,

      /**
       * The filter used when magnifying a texture.
       */
      kMAG,

      /**
       * The filter used when filtering between mipmaps.
       */
      kMIP
    };
  }

  /**
   * @brief Filtering options for textures.
   */
  namespace FILTER_OPTIONS {
    enum E {
      /**
       * Use no filtering. Only relevant for mipmap filtering.
       */
      kNONE = 0,

      /**
       * Filter using the nearest found pixel. Most basic filtering.
       */
      kPOINT = 1,

      /**
       * Average a 2x2 pixel area, bilinear filtering for texture, trilinear
       * for mipmaps.
       */
      kLINEAR = 2,

      /**
       * More advanced filtering that improves quality when viewing textures at
       * a steep angle
       */
      kANISOTROPIC = 3,
    };
  }

  /**
   * @brief Types of frame buffers.
   */
  namespace FRAME_BUFFER_TYPE {
    enum E {
      kCOLOR = 0x1,    //Clear the color surface.
      kDEPTH = 0x2,    //Clear the depth surface.
      kSTENCIL = 0x4   //Clear the stencil surface.
    };
  }

  /**
   * @brief Types of culling that determine how (and if) hardware discards
   *        faces with certain winding order. Winding order can be used for
   *        determining front or back facing polygons by checking the order of
   *        its vertices from the render perspective.
   */
  namespace CULLING_MODE {
    enum E {
      /**
       * Hardware performs no culling and renders both sides.
       */
      kNONE = 0,

      /**
       * Hardware culls faces that have a clockwise vertex ordering.
       */
      kCLOCKWISE = 1,
      
      /**
       * Hardware culls faces that have a counter-clockwise vertex ordering.
       */
      kCOUNTERCLOCKWISE = 2
    };
  }

  /**
   * @brief Polygon mode to use when rasterizing.
   */
  namespace POLYGON_MODE {
    enum E {
      /**
       * Render as wireframe showing only polygon outlines.
       */
      kWIREFRAME = 1,

      /**
       * Render as solid showing whole polygons.
       */
      kSOLID = 2
    };
  }

  /**
   * @brief Types of action that can happen on the stencil buffer.
   */
  namespace STENCIL_OPERATION {
    enum E {
      /**
       * Leave the stencil buffer unchanged.
       */
      kKEEP,

      /**
       * Set the stencil value to zero.
       */
      kZERO,

      /**
       * Replace the stencil value with the reference value.
       */
      kREPLACE,

      /**
       * Increase the stencil value by 1, clamping at the maximum value.
       */
      kINCREMENT,

      /**
       * Decrease the stencil value by 1, clamping at 0.
       */
      kDECREMENT,

      /**
       * Increase the stencil value by 1, wrapping back to 0 when incrementing
       * past the maximum value.
       */
      kINCREMENT_WRAP,

      /**
       * Decrease the stencil value by 1, wrapping when decrementing 0.
       */
      kDECREMENT_WRAP,

      /**
       * Invert the bits of the stencil buffer.
       */
      kINVERT
    };
  }

  /**
   * @brief Describes operation that will be used for rendering a certain set
   *        of vertices.
   */
  enum GE_SCRIPT_EXPORT(n:MeshTopology, m:Rendering) DrawOperationType {
    //Each vertex represents a point.
    DOT_POINT_LIST      GE_SCRIPT_EXPORT(n:PointList) = 1,
    //Each sequential pair of vertices represent a line.
    DOT_LINE_LIST       GE_SCRIPT_EXPORT(n:LineList) = 2,
    //Each vertex (except the first) forms a line with the previous vertex.
    DOT_LINE_STRIP      GE_SCRIPT_EXPORT(n:LineStrip) = 3,
    //Each sequential 3-tuple of vertices represent a triangle.
    DOT_TRIANGLE_LIST	  GE_SCRIPT_EXPORT(n:TriangleList) = 4,
    //Each vertex (except the first two) form a triangle with the previous two
    //vertices.
    DOT_TRIANGLE_STRIP  GE_SCRIPT_EXPORT(n:TriangleStrip) = 5,
    //Each vertex (except the first two) form a triangle with the first vertex
    //and previous vertex.
    DOT_TRIANGLE_FAN    GE_SCRIPT_EXPORT(n:TriangleFan) = 6
  };

  /**
   * @brief Type of mesh indices used, used for determining maximum number of
   *        vertices in a mesh.
   */
  enum GE_SCRIPT_EXPORT(m:Rendering) IndexType {
    IT_16BIT  GE_SCRIPT_EXPORT(n:Index16),  //16-bit indices.
    IT_32BIT  GE_SCRIPT_EXPORT(n:Index32)   //32-bit indices.
  };

  /**
   * @brief These values represent a hint to the driver when locking a hardware
   *        buffer.
   */
  namespace GPU_LOCK_OPTIONS {
    enum E {
      /**
       * Allows you to write to the buffer. Can cause a CPU-GPU sync point so
       * avoid using it often (every frame) as that might limit your
       * performance significantly.
       */
      kREAD_WRITE,

      /**
       * Allows you to write to the buffer. Tells the driver to completely
       * discard the contents of the buffer you are writing to. The driver will
       * (most likely) internally allocate another buffer with same
       * specifications (which is fairly fast) and you will avoid CPU-GPU
       * stalls.
       */
      kWRITE_ONLY_DISCARD,

      /**
       * Allows you to write to the buffer. Tells the driver to discard the
       * contents of the mapped buffer range (but not the entire buffer like
       * with GBL_WRITE_ONLY_DISCARD). Use this if you plan on overwriting all
       * of the range. This can help avoid CPU-GPU stalls.
       */
      kWRITE_ONLY_DISCARD_RANGE,

      /**
       * Allows you to read from a buffer. Be aware that reading is usually a
       * very slow operation.
       */
      kREAD_ONLY,

      /**
       * Allows you to write to the buffer. Guarantees the driver that you will
       * not be updating any part of the buffer that is currently used. This
       * will also avoid CPU-GPU stalls, without requiring you to discard the
       * entire buffer. However it is hard to guarantee when GPU has finished
       * using a buffer.
       */
      kWRITE_ONLY_NO_OVERWRITE,

      /**
       * Allows you to write to a buffer.
       */
      kWRITE_ONLY
    };
  }

  /**
   * @brief Types of programs that may run on GPU.
   */
  namespace GPU_PROGRAM_TYPE {
    enum E {
      kVERTEX_PROGRAM,   //Vertex program.
      kFRAGMENT_PROGRAM, //Fragment (pixel) program.
      kGEOMETRY_PROGRAM, //Geometry program.
      kDOMAIN_PROGRAM,   //Domain (tessellation evaluation) program.
      kHULL_PROGRAM,     //Hull (tessellation control) program.
      kCOMPUTE_PROGRAM,  //Compute program.
      kCOUNT             //Keep at end
    };
  }

  /**
   * @brief Values that represent hardware buffer usage. These usually
   *        determine in what type of memory is buffer placed in, however that
   *        depends on rendering API.
   */
  namespace GPU_BUFFER_USAGE {
    enum E {
      /**
       * Signifies that you don't plan on modifying the buffer often (or at all)
       * after creation. Modifying such buffer will involve a larger performance
       * hit. Mutually exclusive with GBU_DYNAMIC.
       */
      kSTATIC = 0x01,

      /**
       * Signifies that you will modify this buffer fairly often
       * (e.g. every frame). Mutually exclusive with GBU_STATIC.
       */
      kDYNAMIC = 0x02,

      /**
       * Signifies that the buffer can be used for arbitrary load/store
       * operations on the GPU. Implies GBU_STATIC.
       */
      kLOADSTORE = kSTATIC | 0x04
    };
  }

  /**
   * @brief Types of generic GPU buffers that may be attached to GPU programs.
   */
  namespace GPU_BUFFER_TYPE {
    enum E {
      /**
       * Buffer containing an array of primitives (e.g. float4's).
       */
      kSTANDARD,

      /**
       * Buffer containing an array of structures. Structure parameters can
       * usually be easily accessed from within the GPU program.
       */
      kSTRUCTURED,

      /**
       * Special type of buffer allowing you to specify arguments for draw
       * operations inside the buffer instead of providing them directly.
       * Useful when you want to control drawing directly from GPU.
       */
      kINDIRECTARGUMENT
    };
  }

  /**
   * @brief Types of valid formats used for standard GPU buffers.
   */
  namespace GPU_BUFFER_FORMAT {
    enum E {
      k16X1F,           //1D 16-bit floating-point format.
      k16X2F,           //2D 16-bit floating-point format.
      k16X4F,           //4D 16-bit floating-point format.
      k32X1F,           //1D 32-bit floating-point format.
      k32X2F,           //2D 32-bit floating-point format.
      k32X3F,           //3D 32-bit floating-point format.
      k32X4F,           //4D 32-bit floating-point format.
      k8X1,             //1D 8-bit normalized format.
      k8X2,             //2D 8-bit normalized format.
      k8X4,             //4D 8-bit normalized format.
      k16X1,            //1D 16-bit normalized format.
      k16X2,            //2D 16-bit normalized format.
      k16X4,            //4D 16-bit normalized format.
      k8X1S,            //1D 8-bit signed integer format.
      k8X2S,            //2D 8-bit signed integer format.
      k8X4S,            //4D 8-bit signed integer format.
      k16X1S,           //1D 16-bit signed integer format.
      k16X2S,           //2D 16-bit signed integer format.
      k16X4S,           //4D 16-bit signed integer format.
      k32X1S,           //1D 32-bit signed integer format.
      k32X2S,           //2D 32-bit signed integer format.
      k32X3S,           //3D 32-bit signed integer format.
      k32X4S,           //4D 32-bit signed integer format.
      k8X1U,            //1D 8-bit unsigned integer format.
      k8X2U,            //2D 8-bit unsigned integer format.
      k8X4U,            //4D 8-bit unsigned integer format.
      k16X1U,           //1D 16-bit unsigned integer format.
      k16X2U,           //2D 16-bit unsigned integer format.
      k16X4U,           //4D 16-bit unsigned integer format.
      k32X1U,           //1D 32-bit unsigned integer format.
      k32X2U,           //2D 32-bit unsigned integer format.
      k32X3U,           //3D 32-bit unsigned integer format.
      k32X4U,           //4D 32-bit unsigned integer format.
      kCOUNT,           //Not a valid format. Keep just before kUNKNOWN.

      //Unknown format (used for non-standard buffers, like structured or raw.
      kUNKNOWN = 0xffff
    };
  }

  /**
   * @brief Different types of GPU views that control how GPU sees a hardware
   *        buffer.
   */
  namespace GPU_VIEW_USAGE {
    enum E {
      /**
       * Buffer is seen as a default shader resource, used primarily for reading.
       * (for example a texture for sampling)
       */
      kDEFAULT = 0x01,
      /**
       * Buffer is seen as a render target that color pixels will be written to
       * after pixel shader stage.
       */
      kRENDERTARGET = 0x02,
      /**
       * Buffer is seen as a depth stencil target that depth and stencil
       * information is written to.
       */
      kDEPTHSTENCIL = 0x04,
      /**
       * Buffer that allows you to write to any part of it from within a GPU
       * program.
       */
      kRANDOMWRITE = 0x08
    };
  }

  /**
   * @brief Combinable set of bits that describe a set of physical GPU's.
   */
  namespace GPU_DEVICE_FLAGS {
    enum E {
      /**
       * Use the default set of devices. This may be the primary device or
       * multiple devices. Cannot be used together with other device flags.
       */
      kDEFAULT = 0,
      /**
       * Use only the primary GPU.
       */
      kPRIMARY = 0x01,
      /**
       * Use the second GPU.
       */
      kGPU2 = 0x02,
      /**
       * Use the third GPU.
       */
      kGPU3 = 0x04,
      /**
       * Use the fourth GPU.
       */
      kGPU4 = 0x08,
      /**
       * Use the fifth GPU.
       */
      kGPU5 = 0x10
    };
  }

  /**
   * @brief Type of parameter block usages. Signifies how often will parameter
   *        blocks be changed.
   */
  namespace GPU_PARAM_BLOCK_USAGE {
    enum E {
      kSTATIC,  //Buffer will be rarely, if ever, updated.
      kDYNAMIC  //Buffer will be updated often (for example every frame).
    };
  }

  /**
   * @brief Type of a parameter in a GPU program.
   */
  enum GPUParamType {
    GPT_DATA,     //Raw data type like float, Vector3, Color, etc.
    GPT_TEXTURE,  //Texture type (2D, 3D, cube, etc.)
    GPT_BUFFER,   //Data buffer (raw, structured, etc.)
    GPT_SAMPLER   //Sampler type (2D, 3D, cube, etc.)
  };

  /**
   * @brief Type of GPU data parameters that can be used as inputs to a GPU
   *        program.
   */
  namespace GPU_PARAM_DATA_TYPE {
    enum E {
      kFLOAT1 = 1,      //1D floating point value.
      kFLOAT2 = 2,      //2D floating point value.
      kFLOAT3 = 3,      //3D floating point value.
      kFLOAT4 = 4,      //4D floating point value.
      kMATRIX_2X2 = 11, //2x2 matrix.
      kMATRIX_2X3 = 12, //2x3 matrix.
      kMATRIX_2X4 = 13, //2x4 matrix.
      kMATRIX_3X2 = 14, //3x2 matrix.
      kMATRIX_3X3 = 15, //3x3 matrix.
      kMATRIX_3X4 = 16, //3x4 matrix.
      kMATRIX_4X2 = 17, //4x2 matrix.
      kMATRIX_4X3 = 18, //4x3 matrix.
      kMATRIX_4X4 = 19, //4x4 matrix.
      kINT1 = 20,       //1D signed integer value.
      kINT2 = 21,       //2D signed integer value.
      kINT3 = 22,       //3D signed integer value.
      kINT4 = 23,       //4D signed integer value.
      kBOOL = 24,       //Boolean value.
      kSTRUCT = 25,     //Variable size structure.
      kCOLOR = 26,      //Same as GPDT_FLOAT4, but easier to deduct usage.
      kCOUNT = 27,      //Keep at end before GPDT_UNKNOWN
      kUNKNOWN = 0xffff
    };
  }

  /**
   * @brief Available texture types.
   */
  enum GE_SCRIPT_EXPORT(m:Rendering) TextureType {
    //One dimensional texture. Just a row of pixels.
    TEX_TYPE_1D         GE_SCRIPT_EXPORT(n:Texture1D) = 1,
    //Two dimensional texture.
    TEX_TYPE_2D         GE_SCRIPT_EXPORT(n:Texture2D) = 2,
    //Three dimensional texture.
    TEX_TYPE_3D         GE_SCRIPT_EXPORT(n:Texture3D) = 3,
    //Texture consisting out of six 2D textures describing an inside of a cube.
    //Allows special sampling.
    TEX_TYPE_CUBE_MAP   GE_SCRIPT_EXPORT(n:TextureCube) = 4
  };

  /**
   * @brief Projection type to use by the camera.
   */
  enum GE_SCRIPT_EXPORT() ProjectionType {
    /**
     * Projection type where object size remains constant and parallel lines
     * remain parallel.
     */
    PT_ORTHOGRAPHIC   GE_SCRIPT_EXPORT(n:Orthographic),
    /**
     * Projection type that emulates human vision. Objects farther away appear
     * smaller.
     */
    PT_PERSPECTIVE    GE_SCRIPT_EXPORT(n:Perspective)
  };

  /**
   * @brief Contains data about a type used for GPU data parameters.
   */
  struct GPUParamDataTypeInfo
  {
    uint32 baseTypeSize;
    uint32 size;
    uint32 alignment;
    uint32 numRows;
    uint32 numColumns;
  };

  /**
   * @brief Contains a lookup table for various information of all types used
   *        for data GPU parameters. Sizes are in bytes.
   */
  struct GPUDataParamInfos
  {
    GPUDataParamInfos() {
      memset(lookup, 0, sizeof(lookup));
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kFLOAT1)] = { 4, 4, 4, 1, 1 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kFLOAT2)] = { 4, 8, 8, 1, 2 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kFLOAT3)] = { 4, 16, 16, 1, 3 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kFLOAT4)] = { 4, 16, 16, 1, 4 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kCOLOR)] = { 4, 16, 16, 1, 4 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kMATRIX_2X2)] = { 4, 16, 8, 2, 2 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kMATRIX_2X3)] = { 4, 32, 16, 2, 3 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kMATRIX_2X4)] = { 4, 32, 16, 2, 4 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kMATRIX_3X2)] = { 4, 24, 8, 3, 2 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kMATRIX_3X3)] = { 4, 48, 16, 3, 3 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kMATRIX_3X4)] = { 4, 48, 16, 3, 4 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kMATRIX_4X2)] = { 4, 32, 8, 4, 2 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kMATRIX_4X3)] = { 4, 64, 16, 4, 3 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kMATRIX_4X4)] = { 4, 64, 16, 4, 4 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kINT1)] = { 4, 4, 4, 1, 1 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kINT2)] = { 4, 8, 8, 1, 2 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kINT3)] = { 4, 12, 16, 1, 3 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kINT4)] = { 4, 16, 16, 1, 4 };
      lookup[static_cast<uint32>(GPU_PARAM_DATA_TYPE::kBOOL)] = { 4, 4, 4, 1, 1 };
    }

    GPUParamDataTypeInfo lookup[GPU_PARAM_DATA_TYPE::kCOUNT];
  };

  /**
   * @brief Type of GPU object parameters that can be used as inputs to a GPU
   *        program.
   */
  namespace GPU_PARAM_OBJECT_TYPE {
    enum E {
      //Sampler state for a 1D texture.
      kSAMPLER1D = 1,
      //Sampler state for a 2D texture.
      kSAMPLER2D = 2,
      //Sampler state for a 3D texture.
      kSAMPLER3D = 3,
      //Sampler state for a cube texture.
      kSAMPLERCUBE = 4,
      //Sampler state for a 2D texture with multiple samples.
      kSAMPLER2DMS = 5,
      //1D texture.
      kTEXTURE1D = 11,
      //2D texture.
      kTEXTURE2D = 12,
      //3D texture.
      kTEXTURE3D = 13,
      //Cube texture.
      kTEXTURECUBE = 14,
      //2D texture with multiple samples.
      kTEXTURE2DMS = 15,
      //Buffer containing raw bytes (no interpretation).
      kBYTE_BUFFER = 32,
      //Buffer containing a set of structures.
      kSTRUCTURED_BUFFER = 33,
      //Read-write buffer containing a set of primitives.
      kRWTYPED_BUFFER = 41,
      //Read-write buffer containing raw bytes (no interpretation).
      kRWBYTE_BUFFER = 42,
      //Read-write buffer containing a set of structures.
      kRWSTRUCTURED_BUFFER = 43,
      //Read-write buffer containing a set of structures, with a counter.
      kRWSTRUCTURED_BUFFER_WITH_COUNTER = 44,
      //Buffer that can be used for appending data in a stack-like fashion.
      kRWAPPEND_BUFFER = 45,
      //Buffer that can be used for consuming data in a stack-like fashion.
      kRWCONSUME_BUFFER = 46,
      //1D texture with unordered read/writes.
      kRWTEXTURE1D = 50,
      //2D texture with unordered read/writes.
      kRWTEXTURE2D = 51,
      //3D texture with unordered read/writes.
      kRWTEXTURE3D = 52,
      //2D texture with multiple samples and unordered read/writes.
      kRWTEXTURE2DMS = 53,
      //1D texture with multiple array entries.
      kTEXTURE1DARRAY = 54,
      //2D texture with multiple array entries.
      kTEXTURE2DARRAY = 55,
      //Cubemap texture with multiple array entries.
      kTEXTURECUBEARRAY = 56,
      //2D texture with multiple samples and array entries.
      kTEXTURE2DMSARRAY = 57,
      //1D texture with multiple array entries and unordered read/writes.
      kRWTEXTURE1DARRAY = 58,
      //2D texture with multiple array entries and unordered read/writes.
      kRWTEXTURE2DARRAY = 59,
      //2D texture with multiple array entries, samples and unordered read/writes.
      kRWTEXTURE2DMSARRAY = 60,
      kUNKNOWN = 0xffff
    };
  }

  /**
   * @brief Types of GPU queues.
   */
  namespace GPU_QUEUE_TYPE {
    enum E {
      /**
       * Queue used for rendering. Allows the use of draw commands, but also all
       * commands supported by compute or upload buffers.
       */
      kGRAPHICS,

      /**
       * Discrete queue used for compute operations. Allows the use of dispatch
       * and upload commands.
       */
      kCOMPUTE,

      /**
       * Queue used for memory transfer operations only. No rendering or compute
       * dispatch allowed.
       */
      kUPLOAD,

      /*
       * Keep at the end
       */
      kCOUNT
    };
  }

  /**
   * @brief These values represent a hint to the driver when writing to a GPU
   *        buffer.
   */
  namespace BUFFER_WRITE_TYPE {
    enum E {
      /**
       * Default flag with least restrictions. Can cause a CPU-GPU sync point
       * so avoid using it often (every frame) as that might limit your
       * performance significantly.
       */
      kNORMAL,

      /**
       * Tells the driver to completely discard the contents of the buffer you
       * are writing to. The driver will (most likely) internally allocate
       * another buffer with same specifications (which is fairly fast) and you
       * will avoid CPU-GPU stalls.
       */
      kDISCARD,

      /**
       * Guarantees the driver that you will not be updating any part of the
       * buffer that is currently used. This will also avoid CPU-GPU stalls,
       * without requiring you to discard the entire buffer. However it is hard
       * to guarantee when GPU has finished using a buffer.
       */
      kNO_OVERWRITE
    };
  }

  /**
   * @brief Suggested queue priority numbers used for sorting objects in the
   * render queue. Objects with higher priority will be renderer sooner.
   */
  namespace QUEUE_PRIORITY {
    enum E {
      kOpaque = 100000,
      kTransparent = 90000,
      kSkybox = 80000,
      kOverlay = 70000
    };
  }

  /**
   * @brief Type of sorting to perform on an object when added to a render
   *        queue.
   */
  namespace QUEUE_SORT_TYPE {
    enum E {
      /**
       * All objects with the same priority will be rendered front to back based
       * on their center.
       */
      kFrontToBack,
      /**
       * All objects with the same priority will be rendered back to front based
       * on their center.
       */
      kBackToFront,
      /**
       * Objects will not be sorted and will be processed in the order they were
       * added to the queue.
       */
      kNone
    };
  }

  /**
   * @brief Flags that may be assigned to a shader that let the renderer know
   *        how to interpret the shader.
   */
  namespace SHADER_FLAGS {
    enum E {
      /**
       * Signifies that the shader is rendering a transparent object.
       */
      kTransparent = 0x1,

      /**
       * Signifies the shader should use the forward rendering pipeline,
       * if relevant.
       */
      kForward = 0x2
    };
  }

  /**
   * @brief Valid types of a mesh used for physics.
   */
  enum class GE_SCRIPT_EXPORT() PhysicsMeshType {
    /**
     * A regular triangle mesh. Mesh can be of arbitrary size but cannot be
     * used for triggers and non-kinematic objects. Incurs a significantly
     * larger performance impact than convex meshes.
     */
    Triangle,
    
    /**
     * Mesh representing a convex shape. Mesh will not have more than 256
     * vertices. Incurs a significantly lower performance impact than triangle
     * meshes.
     */
    Convex
  };

  /**
   * @brief Determines the type of the source image for generating cubemaps.
   */
  namespace CUBEMAP_SOURCE_TYPE {
    enum E {
      /**
       * Source is a single image that will be replicated on all cubemap faces.
       */
      kSingle,

      /**
       * Source is a list of 6 images, either sequentially next to each other or
       * in a cross format. The system will automatically guess the layout and
       * orientation based on the aspect ratio.
       */
      kFaces,

      /**
       * Source is a single spherical panoramic image.
       */
      kSpherical,

      /**
       * Source is a single cylindrical panoramic image.
       */
      kCylindrical
    };
  }

  /**
   * @brief Bits that map to a specific surface of a render target. Combine the
   *        bits to generate a mask that references only specific render target
   *        surfaces.
   */
  namespace RENDER_SURFACE_MASK_BITS {
    enum E {
      kNONE = 0,
      kCOLOR0 = 1 << 0,
      kCOLOR1 = 1 << 1,
      kCOLOR2 = 1 << 2,
      kCOLOR3 = 1 << 3,
      kCOLOR4 = 1 << 4,
      kCOLOR5 = 1 << 5,
      kCOLOR6 = 1 << 6,
      kCOLOR7 = 1 << 7,
      kDEPTH = 1 << 30,
      kSTENCIL = 1 << 31,
      kDEPTH_STENCIL = (1 << 30) | (1 << 31),
      kALL = 0xFF
    };
  }

  using RenderSurfaceMask = Flags<RENDER_SURFACE_MASK_BITS::E>;
  GE_FLAGS_OPERATORS(RENDER_SURFACE_MASK_BITS::E);

  /**
   * @brief Controls what kind of mobility restrictions a scene object has.
   *        This is used primarily as a performance hint to other systems.
   *        Generally the more restricted the mobility the higher performance
   *        can be achieved.
   */
  enum class GE_SCRIPT_EXPORT() ObjectMobility {
    /**
     * Scene object can be moved and has no mobility restrictions.
     */
    Movable,
    
    /**
     * Scene object isn't allowed to be moved but is allowed to be visually
     * changed in other ways (e.g. changing the displayed mesh or light
     * intensity (depends on attached components).
     */
    Immovable,
    
    /**
     * Scene object isn't allowed to be moved nor is it allowed to be visually
     * changed. Object must be fully static.
     */
    Static
  };

  /**
   * @brief Texture addressing mode, per component.
   */
  struct UVWAddressingMode {
    bool
    operator==(const UVWAddressingMode& rhs) const {
      return u == rhs.u && v == rhs.v && w == rhs.w;
    }

    TEXTURE_ADDRESSING_MODE::E u{ TEXTURE_ADDRESSING_MODE::kWRAP };
    TEXTURE_ADDRESSING_MODE::E v{ TEXTURE_ADDRESSING_MODE::kWRAP };
    TEXTURE_ADDRESSING_MODE::E w{ TEXTURE_ADDRESSING_MODE::kWRAP };
  };

  /**
   * @brief References a subset of surfaces within a texture.
   */
  struct GE_SCRIPT_EXPORT(m:Rendering, pl:true) TextureSurface
  {
    TextureSurface(uint32 _mipLevel = 0,
                   uint32 _numMipLevels = 1,
                   uint32 _face = 0,
                   uint32 _numFaces = 1)
      : mipLevel(_mipLevel),
        numMipLevels(_numMipLevels),
        face(_face),
        numFaces(_numFaces)
    {}

    /**
     * @brief First mip level to reference.
     */
    uint32 mipLevel;

    /**
     * @brief Number of mip levels to reference. Must be greater than zero.
     */
    uint32 numMipLevels;

    /**
     * @brief First face to reference. Face can represent a single cubemap
     *        face, or a single array entry in a texture array. If cubemaps are
     *        laid out in a texture array then every six sequential faces
     *        represent a single array entry.
     */
    uint32 face;

    /**
     * @brief Number of faces to reference, if the texture has more than one.
     */
    uint32 numFaces;

    /**
     * @brief Surface that covers all texture sub-resources.
     */
    static GE_CORE_EXPORT const TextureSurface COMPLETE;
  };

  /**
   * @brief Meta-data describing a chunk of audio.
   */
  struct AudioDataInfo
  {
    /**
     * Total number of audio samples in the audio data (includes all channels).
     */
    uint32 numSamples;

    /**
     * Number of audio samples per second, per channel.
     */
    uint32 sampleRate;

    /**
     * Number of channels. Each channel has its own set of samples.
     */
    uint32 numChannels;

    /**
     * Number of bits per sample.
     */
    uint32 bitDepth;
  };

  /**
   * @brief Helper class for syncing dirty data from sim CoreObject to core
   *        CoreObject and other way around.
   */
  class CoreSyncData
  {
   public:
    CoreSyncData() : m_data(nullptr), m_size(0) {}
    CoreSyncData(uint8* data, uint32 size) : m_data(data), m_size(size) {}

    /**
    * @brief Gets the internal data and checks the data is of valid size. */
    template<class T>
    const T&
    getData() const {
      GE_ASSERT(sizeof(T) == m_size);
      return *reinterpret_cast<T*>(m_data);
    }

    /**
     * @brief Returns a pointer to internal data buffer.
     */
    uint8*
    getBuffer() const {
      return m_data;
    }

    /**
     * @brief Returns the size of the internal data buffer.
     */
    uint32
    getBufferSize() const {
      return m_size;
    }

   private:
    uint8* m_data;
    uint32 m_size;
  };

  typedef Map<String, String> NameValuePairList;

  template<bool Core>
  struct TMeshType {};

  template<>
  struct TMeshType<false>
  {
    typedef HMesh Type;
  };
  
  template<>
  struct TMeshType<true>
  {
    typedef SPtr<geCoreThread::Mesh> Type;
  };

  template<bool Core>
  struct TMaterialPtrType {};
  
  template<>
  struct TMaterialPtrType<false>
  {
    typedef HMaterial Type;
  };

  template<>
  struct TMaterialPtrType<true>
  {
    typedef SPtr<geCoreThread::Material> Type;
  };

  template<bool Core>
  struct TTextureType {};

  template<>
  struct TTextureType<false>
  {
    typedef HTexture Type;
  };

  template<>
  struct TTextureType<true>
  {
    typedef SPtr<geCoreThread::Texture> Type;
  };

  GE_ALLOW_MEMCPY_SERIALIZATION(TextureSurface);
}

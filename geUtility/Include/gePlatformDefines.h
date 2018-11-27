/*****************************************************************************/
/**
 * @file    gePlatformDefines.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2015/02/09
 * @brief   Some definitions to describe the target platform
 *
 * This definitions are meant to describe the target platform
 *
 * @bug	    No known bugs.
 */
/*****************************************************************************/
#pragma once

/*****************************************************************************/
/**
 * Initial platform/compiler-related stuff to set.
 */
/*****************************************************************************/
#define GE_PLATFORM_WIN32   1                 //Windows Platform
#define GE_PLATFORM_LINUX   2                 //Linux Platform
#define GE_PLATFORM_OSX     3                 //Apple Platform
#define GE_PLATFORM_IOS     4                 //iPhone Platform
#define GE_PLATFORM_ANDROID 5                 //Android Platform
#define GE_PLATFORM_PS4     6                 //PlayStation 4 Platform

#define GE_COMPILER_MSVC 1                    //Visual Studio Compiler
#define GE_COMPILER_GNUC 2                    //GCC Compiler
#define GE_COMPILER_INTEL 3                   //Intel Compiler
#define GE_COMPILER_CLANG 4                   //Clang Compiler

#define GE_ARCHITECTURE_x86_32 1              //Intel x86 32 bits
#define GE_ARCHITECTURE_x86_64 2              //Intel x86 64 bits

#define GE_ENDIAN_LITTLE 1                    //Little Endian
#define GE_ENDIAN_BIG 2                       //Big Endian

//Define the actual endian type (little endian for Windows, Linux, Apple and PS4)
#define GE_ENDIAN GE_ENDIAN_LITTLE

#define GE_VERSION_MAJOR 0                    //Engine version mayor value
#define GE_VERSION_MINOR 4                    //Engine version minor value
#define GE_VERSION_PATCH 0                    //Engine version patch value
#define GE_EDITOR_BUILD 1                     //This is an Editor Build

//Define if on a crash we want to report warnings on unknown symbols
#define GE_DEBUG_DETAILED_SYMBOLS 1

/*****************************************************************************/
/**
 *Compiler type and version.
 */
/*****************************************************************************/
#if defined(__clang__)
#   define GE_COMPILER GE_COMPILER_CLANG
#   define GE_COMP_VER __clang_version__
#   define GE_THREADLOCAL __thread
#   define GE_STDCALL __attribute__((stdcall))
#   define GE_CDECL __attribute__((cdecl))
#   define GE_FALLTHROUGH [[clang::fallthrough]];
#elif defined(__GNUC__) //Check after Clang, as Clang defines this too
#   define GE_COMPILER GE_COMPILER_GNUC
#   define GE_COMP_VER (((__GNUC__)*100) + (__GNUC_MINOR__*10) + __GNUC_PATCHLEVEL__)
#   define GE_THREADLOCAL __thread
#   define GE_STDCALL __attribute__((stdcall))
#   define GE_CDECL __attribute__((cdecl))
#   define GE_FALLTHROUGH __attribute__((fallthrough));
#elif defined (__INTEL_COMPILER)
#   define GE_COMPILER GE_COMPILER_INTEL
#   define GE_COMP_VER __INTEL_COMPILER
#   define GE_STDCALL __stdcall
#   define GE_CDECL __cdecl
#   define GE_FALLTHROUGH
/** 
 * GE_THREADLOCAL define is down below because Intel compiler defines it
 * differently based on platform
 */

//Check after Clang and Intel, we could be building with either with VS
#elif defined(_MSC_VER)
#   define GE_COMPILER GE_COMPILER_MSVC
#   define GE_COMP_VER _MSC_VER
#   define GE_THREADLOCAL __declspec(thread)
#   define GE_STDCALL __stdcall
#   define GE_CDECL __cdecl
#   define GE_FALLTHROUGH
#   undef __PRETTY_FUNCTION__
#   define __PRETTY_FUNCTION__ __FUNCSIG__
#else
//No know compiler found, send the error to the output (if any)
#   pragma error "No known compiler. "
#endif

/*****************************************************************************/
/**
 * See if we can use __forceinline or if we need to use __inline instead
 */
/*****************************************************************************/
#if GE_COMPILER == GE_COMPILER_MSVC           //If we are compiling on Visual Studio
# if GE_COMP_VER >= 1200                      //If we are on Visual Studio 6 or higher
#   define FORCEINLINE __forceinline          //Set __forceinline
#   ifndef RESTRICT
#     define RESTRICT __restrict              //No alias hint
#   endif
# endif
#elif defined(__MINGW32__)                    //If we are on a Unix type system
# if !defined(FORCEINLINE)
#   define FORCEINLINE __inline               //Set __inline
#   ifndef RESTRICT
#     define RESTRICT                         //No alias hint
#   endif
# endif
#else                                         //Any other compiler
# define FORCEINLINE __inline                 //Set __inline
# ifndef RESTRICT
#   define RESTRICT __restrict                //No alias hint
# endif
#endif

/*****************************************************************************/
/**
 * Finds the current platform
 */
/*****************************************************************************/
#if defined(__WIN32__) || defined(_WIN32)     //If it's a Windows platform
# define GE_PLATFORM GE_PLATFORM_WIN32
#elif defined( __APPLE_CC__ )                 //It's an Apple platform
# define GE_PLATFORM GE_PLATFORM_OSX
#elif defined( __ORBIS__ )                    //It's a PlayStation 4
# define GE_PLATFORM GE_PLATFORM_PS4
#else                                         //Will consider it as a Linux platform
# define GE_PLATFORM GE_PLATFORM_LINUX
#endif

/*****************************************************************************/
/**
 * Find the architecture type
 */
/*****************************************************************************/
#if defined(__x86_64__) || defined(_M_X64)    //If this is a x64 compile
# define GE_ARCH_TYPE GE_ARCHITECTURE_x86_64
#else                                         //If it's a x86 compile
# define GE_ARCH_TYPE GE_ARCHITECTURE_x86_32
#endif

/*****************************************************************************/
/**
 * Memory Alignment macros
 */
/*****************************************************************************/
#if GE_COMPILER == GE_COMPILER_MSVC           //If we are compiling on Visual Studio
# define MS_ALIGN(n)  __declspec(align(n))
# ifndef GCC_PACK
#   define GCC_PACK(n)
# endif
# ifndef GCC_ALIGN
#   define GCC_ALIGN(n)
# endif
#elif ( GE_COMPILER == GE_COMPILER_GNUC && GE_PLATFORM == GE_PLATFORM_PS4 )
# define MS_ALIGN(n)
# define GCC_PACK(n)
# define GCC_ALIGN(n) __attribute__( (__aligned__(n)) )
#else                                         //If we are on a Unix type system
# define MS_ALIGN(n)
# define GCC_PACK(n)  __attribute__( (packed, aligned(n)) )
# define GCC_ALIGN(n) __attribute__( (aligned(n)) )
#endif

/*****************************************************************************/
/**
 * For throw override (deprecated on c++11 but Visual Studio does not have handle noexcept
 */
/*****************************************************************************/
#if GE_COMPILER == GE_COMPILER_MSVC
# define _NOEXCEPT noexcept
#elif GE_COMPILER == GE_COMPILER_INTEL
# define _NOEXCEPT noexcept
#elif GE_COMPILER == GE_COMPILER_GNUC
# define _NOEXCEPT noexcept
#else
# define _NOEXCEPT
#endif

/*****************************************************************************/
/**
 * Library export specifics
 */
/*****************************************************************************/
#if GE_PLATFORM == GE_PLATFORM_WIN32
# if GE_COMPILER == GE_COMPILER_MSVC
#   if defined( GE_STATIC_LIB )
#     define GE_UTILITY_EXPORT
#   else
#     if defined( GE_UTILITY_EXPORTS )
#       define GE_UTILITY_EXPORT __declspec( dllexport )
#     else
#       define GE_UTILITY_EXPORT __declspec( dllimport )
#     endif
#   endif
# else  //Any other Compiler
#   if defined( GE_STATIC_LIB )
#     define GE_UTILITY_EXPORT
#   else
#     if defined( GE_UTILITY_EXPORTS )
#       define GE_UTILITY_EXPORT __attribute__ ((dllexport))
#     else
#       define GE_UTILITY_EXPORT __attribute__ ((dllimport))
#     endif
#   endif
# endif
# define GE_UTILITY_HIDDEN
#else //Linux/Mac settings
# define GE_UTILITY_EXPORT __attribute__ ((visibility ("default")))
# define GE_UTILITY_HIDDEN __attribute__ ((visibility ("hidden")))
#endif

//DLL export for plugins
#if GE_PLATFORM == GE_PLATFORM_WIN32
# if GE_COMPILER == GE_COMPILER_MSVC
#   define GE_PLUGIN_EXPORT __declspec(dllexport)
# else
#   define GE_PLUGIN_EXPORT __attribute__ ((dllexport))
# endif
# define GE_UTILITY_HIDDEN
#else //Linux/Mac settings
# define GE_PLUGIN_EXPORT __attribute__ ((visibility ("default")))
#endif

/*****************************************************************************/
/**
* Windows specific Settings
*/
/*****************************************************************************/
//Win32 compilers use _DEBUG for specifying debug builds. For MinGW, we set DEBUG
#if GE_PLATFORM == GE_PLATFORM_WIN32
# if defined(_DEBUG) || defined(DEBUG)
#   define GE_DEBUG_MODE 1                    //Specifies that we are on a DEBUG build
# else
#   define GE_DEBUG_MODE 0                    //We are not on a DEBUG build
# endif
# if GE_COMPILER == GE_COMPILER_INTEL
#   define GE_THREADLOCAL __declspec(thread)  //Set the local thread for the Intel compiler
# endif
#endif  //GE_PLATFORM == GE_PLATFORM_WIN32

/*****************************************************************************/
/**
 * Linux/Apple specific Settings
 */
/*****************************************************************************/
#if GE_PLATFORM == GE_PLATFORM_LINUX || GE_PLATFORM == GE_PLATFORM_OSX
# define stricmp strcasecmp

//If we are on a DEBUG build
# if defined(_DEBUG) || defined(DEBUG)
#   define GE_DEBUG_MODE 1                  //Specifies that we are on a DEBUG build
# else
#   define GE_DEBUG_MODE 0                  //We are not on a DEBUG build
# endif
# if GE_COMPILER == GE_COMPILER_INTEL
#   define GE_THREADLOCAL __thread          //Set the local thread for the Intel compiler
# endif
#endif	//GE_PLATFORM == GE_PLATFORM_LINUX || GE_PLATFORM == GE_PLATFORM_OSX

/*****************************************************************************/
/**
 * PS4 specific Settings
 */
/*****************************************************************************/
#if GE_PLATFORM == GE_PLATFORM_PS4
//If we are on a DEBUG build
# if defined(_DEBUG) || defined(DEBUG)
#   define GE_DEBUG_MODE 1                  //Specifies that we are on a DEBUG build
# else
#   define GE_DEBUG_MODE 0                  //We are not on a DEBUG build
# endif
#endif	//GE_PLATFORM == GE_PLATFORM_LINUX || GE_PLATFORM == GE_PLATFORM_OSX

/*****************************************************************************/
/**
 * Definition of Debug macros
 */
/*****************************************************************************/
#if GE_DEBUG_MODE
# define GE_DEBUG_ONLY(x) x
# define GE_ASSERT(x) assert(x)
#else
# define GE_DEBUG_ONLY(x)
# define GE_ASSERT(x)
#endif

/*****************************************************************************/
/**
 * Disable some compiler warnings
 */
/*****************************************************************************/

//If we are compiling with Visual Studio
#if GE_COMPILER == GE_COMPILER_MSVC
  /**
   * TODO:  This is not deactivated anywhere, therefore it applies to any file
   * that includes this header. Right now I don't have an easier way to apply
   * these warnings globally so I'm keeping it this way.
   */

  //Secure versions aren't multi platform, so we won't be using them
# define _CRT_SECURE_NO_WARNINGS

  /**
   * Disable: nonstandard extension used: nameless struct/union.
   * Happens when a struct or union is defined without a name.
   */
# pragma warning(disable : 4201)

  /**
   * Disable: "<type> needs to have DLL interface to be used by clients'
   * Happens on STL member variables which are not public therefore is ok
   */
# pragma warning (disable: 4251)

  //Disable: 'X' Function call with parameters that may be unsafe
# pragma warning(disable: 4996) 

  /**
   * Disable: decorated name length exceeded, name was truncated. Happens with
   * really long type names. Even fairly standard use of std::unordered_map
   * with custom parameters, meaning I can't really do much to avoid it.
   * It shouldn't effect execution but might cause problems if you compile
   * library with one compiler and use it in another.
   */
# pragma warning(disable: 4503)

  /**
   * Disable: nonstandard extension used: override specifier 'keyword'.
   * Happens when a keyword was used that is not in the C++ standard, for
   * example, one of the override specifiers that also works under /clr.
   */
//# pragma warning(disable : 4481)
#endif

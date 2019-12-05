#ifndef MYSTSTEM_H_
#define MYSTSTEM_H_

#include <cstddef>
#include <cstdint>
using ::std::int8_t;
using ::std::int16_t;
using ::std::int32_t;
using ::std::int64_t;

using ::std::uint8_t;
using ::std::uint16_t;
using ::std::uint32_t;
using ::std::uint64_t;
using ::std::uint64_t;

//unix only?
using ::std::size_t;

// POSIX socket
#ifdef _WIN32
#  include <crtdefs.h>
#  include <winsock2.h>
// wincock2 is also for endian converter
// https://stackoverflow.com/questions/5971332/redefinition-errors-in-winsock2-h
#  include <io.h>
#else
#  include <arpa/inet.h>
#  include <unistd.h>
#endif
using socket_t = decltype(socket(0, 0, 0));


// dynamic lib
#ifdef _WIN32
#  include <intrin.h>
#  pragma intrinsic(_ReturnAddress)
#  ifndef DLL_INTERFACE
#    define DLL_INTERFACE __declspec(dllimport)
#  else
#    define DLL_INTERFACE __declspec(dllexport)
#    pragma warning(disable: 4251)
#    pragma warning(disable: 4996)
#  endif
#else
#  include <dlfcn.h>
#endif

// 
// From RAPIDJSON
#define TYPE_LITTLEENDIAN  0   //!< Little endian machine
#define TYPE_BIGENDIAN     1   //!< Big endian machine
// Detect with GCC 4.6's macro
#  ifdef __BYTE_ORDER__
#    if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#      define LOCAL_HOST_ENDIAN TYPE_LITTLEENDIAN
#    elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#      define LOCAL_HOST_ENDIAN TYPE_BIGENDIAN
#    else
#      error Unknown machine endianness detected. User needs to define LOCAL_HOST_ENDIAN.
#    endif // __BYTE_ORDER__
// Detect with GLIBC's endian.h
#  elif defined(__GLIBC__)
#    include <endian.h>
#    if (__BYTE_ORDER == __LITTLE_ENDIAN)
#      define LOCAL_HOST_ENDIAN TYPE_LITTLEENDIAN
#    elif (__BYTE_ORDER == __BIG_ENDIAN)
#      define LOCAL_HOST_ENDIAN TYPE_BIGENDIAN
#    else
#      error Unknown machine endianness detected. User needs to define LOCAL_HOST_ENDIAN.
#   endif // __GLIBC__
// Detect with _LITTLE_ENDIAN and _BIG_ENDIAN macro
#  elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
#    define LOCAL_HOST_ENDIAN TYPE_LITTLEENDIAN
#  elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
#    define LOCAL_HOST_ENDIAN TYPE_BIGENDIAN
// Detect with architecture macros
#  elif defined(__sparc) || defined(__sparc__) || defined(_POWER) || defined(__powerpc__) || defined(__ppc__) || defined(__hpux) || defined(__hppa) || defined(_MIPSEB) || defined(_POWER) || defined(__s390__)
#    define LOCAL_HOST_ENDIAN TYPE_BIGENDIAN
#  elif defined(__i386__) || defined(__alpha__) || defined(__ia64) || defined(__ia64__) || defined(_M_IX86) || defined(_M_IA64) || defined(_M_ALPHA) || defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) || defined(__bfin__)
#    define LOCAL_HOST_ENDIAN TYPE_LITTLEENDIAN
#  elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
#    define LOCAL_HOST_ENDIAN TYPE_LITTLEENDIAN
#  else
#    error Unknown machine endianness detected. User needs to define LOCAL_HOST_ENDIAN.   
#  endif


#ifdef __GLIBC__
#  include <endian.h>
#  define BE16TOH be16toh
#  define BE32TOH be32toh
#  define BE64TOH be64toh
#  define LE16TOH le16toh
#  define LE32TOH le32toh
#  define LE64TOH le64toh
#elif defined(__APPLE__) && (LOCAL_HOST_ENDIAN == TYPE_LITTLEENDIAN)  
#  include <machine/endian.h>
#  define BE16TOH NTOHS
#  define BE32TOH NTOHL
#  define BE64TOH NTOHLL
#  define LE16TOH
#  define LE32TOH
#  define LE64TOH
#elif defined(_WIN32) && (LOCAL_HOST_ENDIAN == TYPE_LITTLEENDIAN)
#  define BE16TOH ntohs
#  define BE32TOH ntohl
#  define BE64TOH ntohll
#  define LE16TOH
#  define LE32TOH
#  define LE64TOH
#else
#  error Unable to set endian coverting functions. Need little-endian Windows, little-endian MacOS, GLIBC
#endif


#define MYSYSTEM_INCLUDE_RAPIDJSON
#ifdef MYSYSTEM_INCLUDE_RAPIDJSON
// SIMD optimization
// #define RAPIDJSON_SSE2
// or
// #define RAPIDJSON_SSE42
// or
// #define RAPIDJSON_NEON 
// or
// not enable

#  include <string>
#  define RAPIDJSON_HAS_STDSTRING 1
#  define RAPIDJSON_NO_INT64DEFINE
   namespace rapidjson { typedef ::std::uint64_t uint64_t; typedef ::std::int64_t int64_t;}
#  define RAPIDJSON_NO_SIZETYPEDEFINE
   namespace rapidjson { typedef ::std::size_t SizeType;}
#  include "rapidjson/rapidjson.h"
#  include "rapidjson/allocators.h"
#  include "rapidjson/encodedstream.h"
#  include "rapidjson/filereadstream.h"
#  include "rapidjson/memorystream.h"
#  include "rapidjson/pointer.h"
#  include "rapidjson/reader.h"
#  include "rapidjson/stringbuffer.h"
#  include "rapidjson/cursorstreamwrapper.h"
#  include "rapidjson/encodings.h"
#  include "rapidjson/filewritestream.h"
#  include "rapidjson/istreamwrapper.h"
#  include "rapidjson/prettywriter.h"
#  include "rapidjson/schema.h"
#  include "rapidjson/writer.h"
#  include "rapidjson/document.h"
#  include "rapidjson/fwd.h"
#  include "rapidjson/memorybuffer.h"
#  include "rapidjson/ostreamwrapper.h"
#  include "rapidjson/stream.h"
#  include "rapidjson/error/en.h"
#endif


#endif

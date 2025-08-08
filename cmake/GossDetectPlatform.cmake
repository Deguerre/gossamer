if (CMAKE_HOST_APPLE)
    # OS X is a Unix, but it's not a normal Unix as far as search paths go.
    message(STATUS "This is an Apple platform")
    set(PLATFORM_FLAGS "-DGOSS_PLATFORM_OSX")
    set(PLATFORM_LDFLAGS)
    set(GOSS_SEARCH_PATHS
        /usr
        /Library/Frameworks
        /Applications/Xcode.app/Contents/Developer/Platforms.MacOSX.platform/Developer/SDKs
        /usr/local
        /opt/local
    )
elseif (CMAKE_HOST_UNIX)
    message(STATUS "This is a Unix-like platform")
    set(PLATFORM_FLAGS "-DGOSS_PLATFORM_UNIX")
    set(PLATFORM_LDFLAGS "-L/lib64 -L/usr/lib/x86_64-linux-gnu/ -lpthread")
    set(GOSS_SEARCH_PATHS
        /usr
        /usr/local
        /opt/local
    )
elseif (CMAKE_HOST_WIN32)
    message(STATUS "This is a Windows platform")
    set(PLATFORM_FLAGS "-DGOSS_PLATFORM_WINDOWS")
    set(PLATFORM_LDFLAGS)
    set(GOSS_SEARCH_PATHS)
else ()
    message(WARNING "This build has not yet been tested on this platform")
    set(PLATFORM_FLAGS)
    set(PLATFORM_LDFLAGS)
    set(GOSS_SEARCH_PATHS)
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(STATUS "The compiler is Clang")
    set(COMPILER_VENDOR "-DGOSS_COMPILER_CLANG")
    set(PLATFORM_CXX_FLAGS
	    "-Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-float-equal"
	)
    set(COMPILER_LDFLAGS "-stdlib=libc++")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    message(STATUS "The compiler is GNU")
    set(COMPILER_VENDOR "-DGOSS_COMPILER_GNU")
    set(PLATFORM_CXX_FLAGS "-Winline -Wall -fomit-frame-pointer -ffast-math ${PLATFORM_GLIBCXX_ABI_FLAG}")
    set(COMPILER_LDFLAGS)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    message(STATUS "The compiler is Intel")
    set(COMPILER_VENDOR "-DGOSS_COMPILER_INTEL")
    message(WARNING "This build has not yet been tested with ICC")
    set(PLATFORM_CXX_FLAGS)
    set(COMPILER_LDFLAGS)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    message(STATUS "The compiler is Microsoft")
    set(COMPILER_VENDOR "-DGOSS_COMPILER_MSVC")
    set(PLATFORM_CXX_FLAGS "/Wall /fp:fast /Gm- /utf-8")
    set(COMPILER_LDFLAGS "")
else()
    message(WARNING "This build may not have been ported to this compiler")
endif()

# Probe for which instruction set we are on
set(PLATFORM_ISA_FLAGS)
    include(CheckCXXSourceRuns)
    set(CMAKE_REQUIRED_FLAGS)

    # Check for AVX
    if (MSVC)
        if (NOT MSVC_VERSION LESS 1600)
            set(CMAKE_REQUIRED_FLAGS "/arch:AVX")
        endif ()
    else ()
        set(CMAKE_REQUIRED_FLAGS "-mavx")
    endif ()

    check_cxx_source_runs("
        #include <immintrin.h>
        int main()
        {
          __m256 a, b, c;
          const float src[8] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
          float dst[8];
          a = _mm256_loadu_ps( src );
          b = _mm256_loadu_ps( src );
          c = _mm256_add_ps( a, b );
          _mm256_storeu_ps( dst, c );
          for( int i = 0; i < 8; i++ ){
            if( ( src[i] + src[i] ) != dst[i] ){
              return -1;
            }
          }
          return 0;
        }"
            HAVE_AVX_EXTENSIONS)

    # Check for AVX2
    if (MSVC)
        if (NOT MSVC_VERSION LESS 1800)
            set(CMAKE_REQUIRED_FLAGS "/arch:AVX2")
        endif ()
    else ()
        set(CMAKE_REQUIRED_FLAGS "-mavx2")
    endif ()

    # Check for AVX2 including BMI2.
    check_cxx_source_runs("
        #include <immintrin.h>
        #include <stdint.h>
        int main()
        {
          __m256i a, b, c;
          const int src[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
          int dst[8];
          a =  _mm256_loadu_si256( (__m256i*)src );
          b =  _mm256_loadu_si256( (__m256i*)src );
          c = _mm256_add_epi32( a, b );
          _mm256_storeu_si256( (__m256i*)dst, c );
          for( int i = 0; i < 8; i++ ){
            if( ( src[i] + src[i] ) != dst[i] ){
              return -1;
            }
          }
          for (int i = 0; i < 32; ++i) {
              int64_t bit = int64_t(1) << i;
              if (_pdep_u64(bit, 0x5555555555555555) != bit*bit) {
                  return -1;
              }
          }
          return 0;
        }" HAVE_AVX2_EXTENSIONS)

    # Set Flags according to check results
    if (MSVC)
        if (HAVE_AVX2_EXTENSIONS AND NOT MSVC_VERSION LESS 1800)
            message(STATUS "We have AVX2 extensions")
            set(PLATFORM_ISA_FLAGS "${PLATFORM_ISA_FLAGS} /arch:AVX2")
        elseif (HAVE_AVX_EXTENSIONS AND NOT MSVC_VERSION LESS 1600)
            message(STATUS "We have AVX extensions")
            set(PLATFORM_ISA_FLAGS "${PLATFORM_ISA_FLAGS} /arch:AVX")
        endif ()
    else ()
        if (HAVE_AVX2_EXTENSIONS)
            message(STATUS "We have AVX2 extensions")
            set(PLATFORM_ISA_FLAGS "${PLATFORM_ISA_FLAGS} -mavx2")
        elseif (HAVE_AVX_EXTENSIONS)
            message(STATUS "We have AVX extensions")
            set(PLATFORM_ISA_FLAGS "${PLATFORM_ISA_FLAGS} -mavx")
        endif ()
    endif ()



# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PLATFORM_FLAGS} ${COMPILER_VENDOR} ${PLATFORM_ISA_FLAGS} ${PLATFORM_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PLATFORM_LDFLAGS} ${COMPILER_LDFLAGS}")

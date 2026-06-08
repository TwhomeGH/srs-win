cmake_minimum_required(VERSION 3.10)
project(st-srs VERSION 1.9 LANGUAGES C ASM_MASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# Source files common to all platforms
set(ST_C_SOURCES
    sched.c
    stk.c
    sync.c
    key.c
    io.c
    event.c
    common.c
)

# Platform-specific assembly
set(ST_ASM_SOURCES "")

if(WIN32)
    # Windows MSVC x64
    set(CMAKE_ASM_MASM_LINK_EXECUTABLE "${CMAKE_LINKER}")
    enable_language(ASM_MASM)
    list(APPEND ST_ASM_SOURCES md_win64.asm)
    add_definitions(-DWIN32 -D_WIN32 -DWIN64 -D_WIN64 -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DFD_SETSIZE=8192)
    # VirtualAlloc in stk.c handles stack allocation; md_win64.asm provides context switching
    add_definitions(-DMD_NO_PROTECT)
elseif(APPLE)
    find_program(ASM_COMPILER NAMES as clang)
    list(APPEND ST_ASM_SOURCES md_darwin.S)
    add_definitions(-DDARWIN -DMD_HAVE_KQUEUE -DMD_HAVE_SELECT)
else()
    list(APPEND ST_ASM_SOURCES md_linux.S md_linux2.S)
    add_definitions(-DLINUX -DMD_HAVE_EPOLL -DMD_HAVE_SELECT)
endif()

# Create the static library
add_library(st-srs STATIC ${ST_C_SOURCES} ${ST_ASM_SOURCES})

# Platform-specific link dependencies
if(WIN32)
    target_link_libraries(st-srs PUBLIC ws2_32)
endif()

# Include directories
target_include_directories(st-srs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Debug flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(st-srs PRIVATE DEBUG)
endif()

# Generate st.h (SRS includes <st.h> which is a copy of public.h)
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/st.h)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/st.h "#include \"public.h\"")
endif()

# Export the public header location
target_include_directories(st-srs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

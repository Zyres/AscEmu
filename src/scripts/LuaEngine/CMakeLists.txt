# Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>

if(NOT BUILD_LUAENGINE)
  message("\tLuaEngine was skipped because BUILD_LUAENGINE is unchecked.")
  return()
endif()

# set up our project name
project(LuaEngine CXX)

set(PCH_HEADERS PCH/LuaEnginePCH.hpp)

#SRCS - contains the source files that compose this project.
set(SRCS
    GameobjectFunctions.cpp
    LUAEngine.cpp
    LuaGlobal.cpp
    LuaHelpers.cpp
    # Required for linker
    ../../world/Server/ServerState.cpp
    ../../world/Spell/SpellCastTargets.cpp
)

set(HEADERS
    AuraFunctions.h
    FunctionTables.h
    GameobjectFunctions.h
    GlobalFunctions.h
    ItemFunctions.h
    LUAEngine.h
    LUAFunctions.h
    LuaGlobal.h
    LuaHelpers.h
    LuaMacros.h
    LuaSqlApi.h
    PacketFunctions.h
    SpellFunctions.h
    TaxiFunctions.h
    UnitFunctions.h
)

include_directories(
   ${OPENSSL_INCLUDE_DIR}
   ${PCRE_INCLUDE_DIR}
   ${CMAKE_SOURCE_DIR}/dep/recastnavigation/Detour/Include
   ${CMAKE_SOURCE_DIR}/src/collision
   ${CMAKE_SOURCE_DIR}/src/collision/Management
   ${CMAKE_SOURCE_DIR}/src/collision/Maps
   ${CMAKE_SOURCE_DIR}/src/collision/Models
   ${CMAKE_SOURCE_DIR}/dep/g3dlite/include
   ${CMAKE_SOURCE_DIR}/src/shared
   ${CMAKE_SOURCE_DIR}/src/world
   ${CMAKE_SOURCE_DIR}/dep/lualib/src
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${ZLIB_INCLUDE_DIRS}
   ${CMAKE_SOURCE_DIR}/dep/utf8cpp
)

add_library(${PROJECT_NAME} SHARED ${SRCS} ${HEADERS})
target_link_libraries(${PROJECT_NAME} world lualib)

if(USE_PCH)
	gen_cxx_pch(${PROJECT_NAME} ${PCH_HEADERS})
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${ASCEMU_SCRIPTLIB_PATH} LIBRARY DESTINATION ${ASCEMU_SCRIPTLIB_PATH})
unset(sources)
unset(headers)

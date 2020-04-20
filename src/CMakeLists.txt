set(KeepMovingGarbageRobotGameLogic_SOURCES
    GameInterface.hpp
    GameLogic.cpp
    GameLogic.hpp
    TileSet.cpp
    EntityBehavior.cpp
    Renderer.cpp
    Collisions.cpp
)

set(KeepMovingGarbageRobot_SOURCES
    Main.cpp
)

if(LIVE_CODING_SUPPORT)
    add_definitions(-DUSE_LIVE_CODING)
    add_library(KeepMovingGarbageRobotGameLogic MODULE ${KeepMovingGarbageRobotGameLogic_SOURCES})
    target_link_libraries(KeepMovingGarbageRobotGameLogic ${KeepMovingGarbageRobot_DEP_LIBS})
else()
    set(KeepMovingGarbageRobot_SOURCES ${KeepMovingGarbageRobotGameLogic_SOURCES} ${KeepMovingGarbageRobot_SOURCES} )
endif()

add_executable(KeepMovingGarbageRobot WIN32 ${KeepMovingGarbageRobot_SOURCES})
set_target_properties(KeepMovingGarbageRobot PROPERTIES LINK_FLAGS "${ASSET_FLAGS}")
target_link_libraries(KeepMovingGarbageRobot ${KeepMovingGarbageRobot_DEP_LIBS})

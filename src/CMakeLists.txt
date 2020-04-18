set(SimpleGameTemplateGameLogic_SOURCES
    GameInterface.hpp
    GameLogic.cpp
    GameLogic.hpp
)

set(SimpleGameTemplate_SOURCES
    Main.cpp
)

if(LIVE_CODING_SUPPORT)
    add_definitions(-DUSE_LIVE_CODING)
    add_library(SimpleGameTemplateGameLogic MODULE ${SimpleGameTemplateGameLogic_SOURCES})
    target_link_libraries(SimpleGameTemplateGameLogic ${SimpleGameTemplate_DEP_LIBS})
else()
    set(SimpleGameTemplate_SOURCES ${SimpleGameTemplateGameLogic_SOURCES} ${SimpleGameTemplate_SOURCES} )
endif()

add_executable(SimpleGameTemplate ${SimpleGameTemplate_SOURCES})
set_target_properties(SimpleGameTemplate PROPERTIES LINK_FLAGS "${ASSET_FLAGS}")
target_link_libraries(SimpleGameTemplate ${SimpleGameTemplate_DEP_LIBS})

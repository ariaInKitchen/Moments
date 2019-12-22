file( GLOB moments-SOURCES "*.cpp" )
file( GLOB moments-HEADERS "*.h" )

add_library(moments SHARED)
target_sources(moments PRIVATE
    ${moments-SOURCES})
target_include_directories(moments PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}"
    "${pkg-Elastos.SDK.Contact_INCLUDE_DIRS}")

target_link_libraries(moments PUBLIC
            sqlite3
            PeerNode)

install(TARGETS moments
        LIBRARY DESTINATION lib/PeerNodePlugins)

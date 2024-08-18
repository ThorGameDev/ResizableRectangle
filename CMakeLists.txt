cmake_minimum_required(VERSION 3.30)

#Un Comment for a windows build
#SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.dll.a") 
#set(CMAKE_EXE_LINKER_FLAGS "-static")
#set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" CONFIG)
#set(BUILD_SHARED_LIBS OFF)
#Remember to add the libSDL dll file to the build directory

project(Change)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

set(SOURCES gui.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})
#Un comment for a windows build
#target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} -static)
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    #set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-Os -s USE_SDL=2")
endif()

#FILE(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION "${CMAKE_BINARY_DIR}")

set(RESOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${RESOURCE_DIR}
        ${CMAKE_BINARY_DIR}/resources
)

if(EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME} PROPERTIES
	    #COMPILE_FLAGS "-s USE_FILESYSTEM=1 -s FORCE_FILESYSTEM=1"
        LINK_FLAGS "--preload-file ${RESOURCE_DIR}@/resources"
    )
endif()

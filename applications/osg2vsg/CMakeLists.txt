find_package(OpenGL)

if(WIN32)
    set(OPENGL_LIBRARY ${OPENGL_gl_LIBRARY})
else()
    set(OPENGL_LIBRARY OpenGL::GL)
endif()

if(NOT ANDROID)
    find_package(Threads)
endif()

if (UNIX)
    find_library(DL_LIBRARY dl)
endif()

set(SOURCES
    osg2vsg.cpp)

add_executable(osg2vsg_viewer ${SOURCES})

target_include_directories(osg2vsg_viewer PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    ${OSG_INCLUDE_DIR}
)

set_target_properties(osg2vsg_viewer PROPERTIES OUTPUT_NAME osg2vsg)

target_link_libraries(osg2vsg_viewer
    osg2vsg
    vsg::vsg
    ${GLSLANG}
    Vulkan::Vulkan
    ${OSGDB_LIBRARIES} ${OSGUTIL_LIBRARIES} ${OSG_LIBRARIES} ${OPENTHREADS_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${OPENGL_LIBRARY} ${DL_LIBRARY}
)

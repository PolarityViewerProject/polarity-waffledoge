# -*- cmake -*-
project(llplugintest)

include(00-Common)
include(OpenGL)
include(LLCommon)
include(LLPlugin)
include(Linking)
include(LLSharedLibs)
include(PluginAPI)
include(LLImage)
include(LLMath)
include(LLMessage)
include(LLRender)
include(LLWindow)
include(Glut)

include_directories(
    ${LLPLUGIN_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
)
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    )

if (DARWIN)
    include(CMakeFindFrameworks)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
endif (DARWIN)

### demo_plugin

#set(demo_plugin_SOURCE_FILES
#    demo_plugin.cpp
#    )
#
#add_library(demo_plugin
#    SHARED
#    ${demo_plugin_SOURCE_FILES}
#)
#
#target_link_libraries(demo_plugin
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#  ${PLUGIN_API_WINDOWS_LIBRARIES}
#)
#
#add_dependencies(demo_plugin
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#)
#
#if (DARWIN)
#  # Don't prepend 'lib' to the executable name, and don't embed a full path in the library's install name
#  set_target_properties(
#    demo_plugin
#    PROPERTIES
#    PREFIX ""
#    BUILD_WITH_INSTALL_RPATH 1
#    INSTALL_NAME_DIR "@executable_path"
#  )
#endif (DARWIN)

### plugin_host

#set(plugin_host_SOURCE_FILES
#    plugin_host.cpp
#    )
#
#add_executable(plugin_host
#    WIN32
#    ${plugin_host_SOURCE_FILES}
#)
#
#set_target_properties(plugin_host
#    PROPERTIES
#    WIN32_EXECUTABLE
#    FALSE
#)
#
#target_link_libraries(plugin_host
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#  ${PLUGIN_API_WINDOWS_LIBRARIES}
#)
#
#add_dependencies(plugin_host
#  demo_plugin
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#)

### plugin_process_launcher

#set(plugin_process_launcher_SOURCE_FILES
#    plugin_process_launcher.cpp
#    )
#
#add_executable(plugin_process_launcher
#    WIN32
#    ${plugin_process_launcher_SOURCE_FILES}
#)
#
#set_target_properties(plugin_process_launcher
#    PROPERTIES
#    WIN32_EXECUTABLE
#    FALSE
#)
#
#target_link_libraries(plugin_process_launcher
#  ${LLPLUGIN_LIBRARIES}
#  ${LLMESSAGE_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#  ${PLUGIN_API_WINDOWS_LIBRARIES}
#)
#
#add_dependencies(plugin_process_launcher
#  SLPlugin
#  demo_plugin
#  ${LLPLUGIN_LIBRARIES}
#  ${LLMESSAGE_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#)

### media_simple_test

#set(media_simple_test_SOURCE_FILES
#    media_simple_test.cpp
#    )
#
#add_executable(media_simple_test
#    WIN32
#    ${media_simple_test_SOURCE_FILES}
#)
#
#add_dependencies(media_simple_test stage_third_party_libs)
#
#set_target_properties(media_simple_test
#    PROPERTIES
#    WIN32_EXECUTABLE
#    FALSE
#)
#
#target_link_libraries(media_simple_test
#  ${GLUT_LIBRARY}
#  ${OPENGL_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#)

### media_plugin_test

#set(media_plugin_test_SOURCE_FILES
#    media_plugin_test.cpp
#    )
#
#add_executable(media_plugin_test
#    WIN32
#    ${media_plugin_test_SOURCE_FILES}
#)
#
#set_target_properties(media_plugin_test
#    PROPERTIES
#    WIN32_EXECUTABLE
#    FALSE
#)
#
#target_link_libraries(media_plugin_test
#  ${GLUT_LIBRARY}
#  ${OPENGL_LIBRARIES}
#  ${LLPLUGIN_LIBRARIES}
#  ${LLMESSAGE_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#  ${PLUGIN_API_WINDOWS_LIBRARIES}
#)
#
#add_dependencies(media_plugin_test
#  stage_third_party_libs
#  SLPlugin
#  demo_media_plugin
#  ${LLPLUGIN_LIBRARIES}
#  ${LLMESSAGE_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#)

### demo_media_plugin

#set(demo_media_plugin_SOURCE_FILES
#    demo_media_plugin.cpp
#    )
#
#add_library(demo_media_plugin
#    SHARED
#    ${demo_media_plugin_SOURCE_FILES}
#)
#
#target_link_libraries(demo_media_plugin
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#  ${PLUGIN_API_WINDOWS_LIBRARIES}
#)
#
#add_dependencies(demo_media_plugin
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#)
#
#if (DARWIN)
#  # Don't prepend 'lib' to the executable name, and don't embed a full path in the library's install name
#  set_target_properties(
#    demo_media_plugin
#    PROPERTIES
#    PREFIX ""
#    BUILD_WITH_INSTALL_RPATH 1
#    INSTALL_NAME_DIR "@executable_path"
#  )
#endif (DARWIN)

### demo_media_plugin_2

#set(demo_media_plugin_2_SOURCE_FILES
#    demo_media_plugin_2.cpp
#    )
#
#add_library(demo_media_plugin_2
#    SHARED
#    ${demo_media_plugin_2_SOURCE_FILES}
#)
#
#target_link_libraries(demo_media_plugin_2
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#  ${PLUGIN_API_WINDOWS_LIBRARIES}
#)
#
#add_dependencies(demo_media_plugin_2
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#)
#
#if (DARWIN)
#  # Don't prepend 'lib' to the executable name, and don't embed a full path in the library's install name
#  set_target_properties(
#    demo_media_plugin_2
#    PROPERTIES
#    PREFIX ""
#    BUILD_WITH_INSTALL_RPATH 1
#    INSTALL_NAME_DIR "@executable_path"
#  )
#endif (DARWIN)

# Gather build products of the various dependencies into the build directory for the testbed.

if(WINDOWS)
  #********************
  # Plugin test library deploy
  #
  # Debug config runtime files required for the plugin test mule
  set(plugintest_debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
  set(plugintest_debug_files
    libeay32.dll
    libglib-2.0-0.dll
    libgmodule-2.0-0.dll
    libgobject-2.0-0.dll
    libgthread-2.0-0.dll
    ssleay32.dll
    )
  copy_if_different(
    ${plugintest_debug_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/Debug"
    out_targets
    ${plugintest_debug_files}
    )
  set(plugin_test_targets ${plugin_test_targets} ${out_targets})
  
  # Release & ReleaseDebInfo config runtime files required for the plugin test mule
  set(plugintest_release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
  set(plugintest_release_files
    libeay32.dll
    libglib-2.0-0.dll
    libgmodule-2.0-0.dll
    libgobject-2.0-0.dll
    libgthread-2.0-0.dll
    ssleay32.dll
    )
  copy_if_different(
    ${plugintest_release_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/Release"
    out_targets
    ${plugintest_release_files}
    )
  set(plugin_test_targets ${plugin_test_targets} ${out_targets})

  copy_if_different(
    ${plugintest_release_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo"
    out_targets
    ${plugintest_release_files}
    )
  set(plugin_test_targets ${plugin_test_targets} ${out_targets})

   add_custom_target(copy_plugintest_libs ALL
     DEPENDS 
     ${plugin_test_targets}
     )

endif(WINDOWS)

if (DARWIN)
  set(plugintest_release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
  set(plugintest_release_files
    libexception_handler.dylib
    libaprutil-1.0.dylib
    libapr-1.0.dylib
    )
  copy_if_different(
    ${plugintest_release_src_dir}
    "${PLUGINS_DESTINATION_DIR}"
    out_targets
    ${plugintest_release_files}
    )
  set(plugin_test_targets ${plugin_test_targets} ${out_targets})

  add_custom_target(copy_plugintest_libs ALL
    DEPENDS 
    ${plugin_test_targets}
    )

endif (DARWIN)


#
# Tool library used by rocprofiler
#

rocprofiler_activate_clang_tidy()

set(KOKKOSP_HEADERS)
set(KOKKOSP_SOURCES kokkosp.cpp)

add_library(rocprofiler-sdk-tool-kokkosp SHARED)
target_sources(rocprofiler-sdk-tool-kokkosp PRIVATE ${KOKKOSP_SOURCES} ${KOKKOSP_HEADERS})

target_link_libraries(
    rocprofiler-sdk-tool-kokkosp
    PRIVATE rocprofiler-sdk-roctx::rocprofiler-sdk-roctx-shared-library
            rocprofiler-sdk::rocprofiler-headers
            rocprofiler-sdk::rocprofiler-common-library rocprofiler-sdk::rocprofiler-glog)

set_target_properties(
    rocprofiler-sdk-tool-kokkosp
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/rocprofiler-sdk
               BUILD_RPATH "\$ORIGIN:\$ORIGIN/.."
               INSTALL_RPATH "\$ORIGIN:\$ORIGIN/..")

install(
    TARGETS rocprofiler-sdk-tool-kokkosp
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/rocprofiler-sdk
    COMPONENT tools
    EXPORT rocprofiler-sdk-tool-targets)

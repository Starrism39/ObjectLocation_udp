aux_source_directory(${PROJECT_SOURCE_DIR}/src/pkg/modules DET_SRC_DIR)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/utils/ DET_SRC_DIR)

add_executable(pkgServer pkgServer.cpp ${DET_SRC_DIR})
target_link_libraries(pkgServer PRIVATE ${OpenCV_LIBS} Threads::Threads)
set_target_properties(pkgServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/bin)

add_executable(pkgClient pkgClient.cpp ${DET_SRC_DIR})
target_link_libraries(pkgClient PRIVATE ${OpenCV_LIBS} Threads::Threads)
set_target_properties(pkgClient PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/bin)
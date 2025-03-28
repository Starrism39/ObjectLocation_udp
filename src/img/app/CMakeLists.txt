aux_source_directory(${PROJECT_SOURCE_DIR}/src/img/modules DET_SRC_DIR)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/utils/ DET_SRC_DIR)

add_executable(imgTest imgTest.cpp ${DET_SRC_DIR})
target_link_libraries(imgTest PRIVATE ${OpenCV_LIBS} Threads::Threads)
set_target_properties(imgTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/bin)

add_executable(imgServer imgServer.cpp ${DET_SRC_DIR})
target_link_libraries(imgServer PRIVATE ${OpenCV_LIBS} Threads::Threads)
set_target_properties(imgServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/bin)

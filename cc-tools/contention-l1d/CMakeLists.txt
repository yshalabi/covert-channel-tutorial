add_executable (measure-l1d-contention measure_l1d_contention.c)
add_executable (create-l1d-contention create_l1d_contention.c)

#target_link_libraries (measure-l1d-contention LINK_PUBLIC l1d)
#target_link_libraries (create-l1d-contention LINK_PUBLIC l1d)

ExternalProject_Get_Property(mastik source_dir)
include_directories(${source_dir}/src ${PROJECT_SOURCE_DIR}/cc-libs)
add_dependencies(create-l1d-contention mastik)
add_dependencies(measure-l1d-contention mastik)
target_link_libraries(create-l1d-contention ${source_dir}/src/l1.o)
target_link_libraries(measure-l1d-contention ${source_dir}/src/l1.o)
#add_custom_target(contention-viz)
add_custom_command(
	TARGET measure-l1d-contention POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/Viz_L1D_contention.sh
    ${CMAKE_BINARY_DIR}/graph-l1d-contention)

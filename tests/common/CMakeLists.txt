find_package(HDF5 REQUIRED)

add_library(common_libs STATIC
    impiReader.cpp
    meshloader.cpp
    )

target_link_libraries(common_libs
    ${OSPRAY_LIBRARIES}
    ${HDF5_C_LIBRARIES}
    ${HDF5_C_HL_LIBRARIES}
    )

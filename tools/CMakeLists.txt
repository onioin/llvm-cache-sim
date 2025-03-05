set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fPIC")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

add_executable(inject-cache-sim
        inject.cpp)

target_include_directories(inject-cache-sim
        PRIVATE
        "../llvm-pass/include"
        ${CMAKE_CURRENT_BINARY_DIR})

llvm_map_components_to_libnames(REQ_LLVM_LIBRARIES
        ${LLVM_TARGETS_TO_BUILD}
        asmparser linker bitreader bitwriter irreader
        target mc support)

target_link_libraries(inject-cache-sim
        PRIVATE
        InjectCacheSim
        FindMemCall
        ${REQ_LLVM_LIBRARIES})

add_executable(multiple-inject
        multiple-inject.cpp)

target_link_libraries(multiple-inject
        PRIVATE
        LLVMSupport)

target_link_libraries(multiple-inject
        INTERFACE
        LLVMSupport)

target_include_directories(multiple-inject
        PRIVATE
        "./include")


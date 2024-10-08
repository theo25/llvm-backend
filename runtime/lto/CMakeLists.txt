add_library(lto STATIC
  alloc.cpp
)
add_library(lto-static STATIC
  alloc.cpp
)

target_compile_options(lto PRIVATE "-flto")

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/runtime/lto/alloc.cpp.o
	COMMAND ${CMAKE_C_COMPILER_AR} x ${CMAKE_BINARY_DIR}/runtime/lto/liblto.a alloc.cpp.o
	DEPENDS lto
)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/runtime/lto/alloc.cpp.o.ll
	COMMAND ${LLVM_DIS} ${CMAKE_BINARY_DIR}/runtime/lto/alloc.cpp.o
	DEPENDS ${CMAKE_BINARY_DIR}/runtime/lto/alloc.cpp.o
)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/runtime/lto/alloc_cpp.cpp
	COMMAND xxd -i alloc.cpp.o.ll alloc_cpp.cpp
	DEPENDS ${CMAKE_BINARY_DIR}/runtime/lto/alloc.cpp.o.ll
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/runtime/lto
)

add_library(alloc-cpp STATIC
  ${CMAKE_BINARY_DIR}/runtime/lto/alloc_cpp.cpp
)

set_target_properties(alloc-cpp PROPERTIES EXPORT_COMPILE_COMMANDS Off)

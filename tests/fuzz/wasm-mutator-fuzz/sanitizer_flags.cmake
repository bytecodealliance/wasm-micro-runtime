if(NOT IN_OSS_FUZZ)
  message(STATUS "Enable ASan and UBSan in non-oss-fuzz environment for vmlib")

  add_compile_options(-fprofile-instr-generate -fcoverage-mapping)

  #
  # Sync up with the content of infra/base-images/base-builder/Dockerfile in oss-fuzz
  #

  # SANITIZER_FLAGS_address
  add_compile_options(-fsanitize=address -fsanitize-address-use-after-scope)

  # SANITIZER_FLAGS_undefined
  add_compile_options(
    -fsanitize=array-bounds,bool,builtin,enum,function,integer-divide-by-zero,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,unsigned-integer-overflow,unreachable,vla-bound,vptr
    -fno-sanitize-recover=array-bounds,bool,builtin,enum,function,integer-divide-by-zero,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,unreachable,vla-bound,vptr
  )

  add_link_options(-fsanitize=address,undefined -fprofile-instr-generate)
endif()

# Always disable unsigned-integer-overflow 
if(CMAKE_C_COMPILER_ID MATCHES ".*Clang")
  add_compile_options(-fno-sanitize=unsigned-integer-overflow)
endif()

# '-fsanitize=vptr' not allowed with '-fno-rtti
# But, LLVM by default, disables the use of `rtti` in the compiler
add_compile_options(-fsanitize=fuzzer -fno-sanitize=vptr)
add_link_options(-fsanitize=fuzzer -fno-sanitize=vptr)

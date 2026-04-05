cmake_minimum_required(VERSION 3.13)
project(instruction_metering_wasm_apps)

add_executable(resume_counter resume_counter.c)
set_target_properties(resume_counter PROPERTIES SUFFIX .wasm)
target_link_options(resume_counter PRIVATE
  "-Wl,--no-entry"
  "-Wl,--export=countdown"
  "-Wl,--export=noop")
install(TARGETS resume_counter DESTINATION .)

add_executable(resume_nested resume_nested.c)
set_target_properties(resume_nested PROPERTIES SUFFIX .wasm)
target_link_options(resume_nested PRIVATE
  "-Wl,--no-entry"
  "-Wl,--export=install"
  "-Wl,--export=noop")
install(TARGETS resume_nested DESTINATION .)

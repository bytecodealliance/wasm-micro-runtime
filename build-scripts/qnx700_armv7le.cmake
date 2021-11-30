# see https://cristianadam.eu/20181202/a-better-qnx-cmake-toolchain-file/
##message("platforms/qnx700/qnx7_armv7.cmake v 2020-09-23 pa")

set(CMAKE_SYSTEM_NAME QNX)

set(arch gcc_ntoarmv7le)
set(ntoarch armv7)
set(QNX_PROCESSOR armv7)

set(CMAKE_C_COMPILER /opt/qnx/qnx700/host/linux/x86_64/usr/bin/qcc)
set(CMAKE_C_COMPILER_TARGET ${arch})
set(CMAKE_C_FLAGS -Vgcc_ntoarmv7le)

# set(CMAKE_CXX_COMPILER qcc -lang-c++)
set(CMAKE_CXX_COMPILER /opt/qnx/qnx700/host/linux/x86_64/usr/bin/q++)
set(CMAKE_CXX_COMPILER_TARGET ${arch})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Vgcc_ntoarmv7le -Wc,-std=c++14")

set(CMAKE_ASM_COMPILER /opt/qnx/qnx700/host/linux/x86_64/usr/bin/qcc -V${arch})
set(CMAKE_ASM_DEFINE_FLAG "-Wa,--defsym,")

set(CMAKE_RANLIB $ENV{QNX_HOST}/usr/bin/nto${ntoarch}-ranlib
    CACHE PATH "QNX ranlib Program" FORCE)
set(CMAKE_AR $ENV{QNX_HOST}/usr/bin/nto${ntoarch}-ar
    CACHE PATH "QNX qr Program" FORCE)
		
# Problem: empty libiwasm_EXPORTS gets added to definitions without trailing =
# This would output correctly add_definitions(-Dlibiwasm_EXPORTS=)

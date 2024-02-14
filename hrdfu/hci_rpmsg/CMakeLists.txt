cmake_minimum_required(VERSION 3.20.0)

set(BOARD nrf5340dk_nrf5340_cpunet)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hci_rpmsg)

add_compile_definitions("$<$<BOOL:${FW_VERSION}>:FW_VERSION=${FW_VERSION}>")

target_sources(app PRIVATE src/main.c)

# Include source directory
zephyr_include_directories(src)

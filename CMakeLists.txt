cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	if(DEFINED ENV{VITASDK})
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
	else()
		message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
	endif()
endif()

set(SHORT_NAME qrdemo)
project(${SHORT_NAME})
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "QR Demo")
set(VITA_TITLEID  "CXZV00003")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(${SHORT_NAME}
	main.c
)

target_link_libraries(${SHORT_NAME}
	quirc
	vita2d
	m
	SceGxm_stub
	ScePgf_stub
	SceSysmodule_stub
	SceLibKernel_stub 
	SceCtrl_stub
	SceCamera_stub 
	SceDisplay_stub 
	SceCommonDialog_stub
	SceAppMgr_stub 
)

vita_create_self(eboot.bin ${SHORT_NAME} UNCOMPRESSED)
vita_create_vpk(${SHORT_NAME}.vpk ${VITA_TITLEID} eboot.bin
	VERSION ${VITA_VERSION}
	NAME ${VITA_APP_NAME}
)

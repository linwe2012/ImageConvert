cmake_minimum_required(VERSION 2.8.11)
project(Image_Convert)

set(SRC_DIR "ImageConvert")

set(LIB_DIR ${SRC_DIR}/lib)
set(INCLUDE_DIR ${SRC_DIR}/include)

set(SRC_FILES
	${SRC_DIR}/bmp.c
	${SRC_DIR}/bmp.h
	${SRC_DIR}/camera.h
	${SRC_DIR}/config_macros.h
	${SRC_DIR}/def_utils.h
	${SRC_DIR}/glad.c
	${SRC_DIR}/glim.cpp
	${SRC_DIR}/glim.h
	${SRC_DIR}/shader.h
	${SRC_DIR}/Window.h
	)
link_directories(${LIB_DIR})
include_directories(${INCLUDE_DIR})
add_executable(imconvert ${SRC_FILES})
	


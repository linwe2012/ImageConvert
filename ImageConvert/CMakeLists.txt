cmake_minimum_required(VERSION 2.8.11)
project(ImageConvert)

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
	${SRC_DIR}/Kaleidoscope.cpp
	${SRC_DIR}/Kaleidoscope_Debugger.h
	${SRC_DIR}/Kaleidoscope_utils.cpp
	${SRC_DIR}/Kaleidoscope_utils.h
	${SRC_DIR}/VariablesManagementl.h
	${SRC_DIR}/Wrapper.h
	)
	
IF (UNIX) #Unix Like System
set(CMAKE_EXE_LINKER_FLAGS "-lGL") #add opengl to lib
set(LIB glfw3)
ELSEIF(WIN32)
set(LIB glfw3.lib opengl32.lib)
ENDIF ()

link_directories(${LIB_DIR})
link_libraries(${LIB})
include_directories(${INCLUDE_DIR})
add_executable(ImageConvert ${SRC_FILES})
	


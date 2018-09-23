cmake_minimum_required(VERSION 3.1)

get_filename_component(FIPS_ROOT_DIR "./build/fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

fips_setup(PROJECT Zany80_Lua)
project(Zany80_Lua VERSION 0.0.1.3)

if(FIPS_WINDOWS)
  ENABLE_LANGUAGE(RC)
  set(CMAKE_RC_COMPILE_OBJECT
    "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
  set(ICON_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/misc/icon.rc)
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
endif()

if (NOT DEFINED LUA_JIT)
	set(LUA_JIT FALSE CACHE BOOL "")
endif()

fips_include_directories("${PROJECT_SOURCE_DIR}/headers" "${PROJECT_BINARY_DIR}")

configure_file("${PROJECT_SOURCE_DIR}/src/config.h.in" "${PROJECT_BINARY_DIR}/config.h")
fips_begin_app(zany80 windowed)
fips_deps(Gfx IMUI)
if (FIPS_EMSCRIPTEN)
	fips_deps(HttpFS)
else()
	fips_deps(LocalFS)
endif()
if (LUA_JIT)
	fips_deps(luajit-5.1)
else()
	fips_deps(lua)
endif()
fips_src(src/)
fips_end_app()

fips_finish()

if(FIPS_WINDOWS)
	set(CPACK_GENERATOR "NSIS")
	install (TARGETS zany80
		RUNTIME DESTINATION .
		LIBRARY DESTINATION .
	)
else()
	set(CPACK_GENERATOR "TGZ;DEB")
	install (FILES misc/zany80.desktop DESTINATION share/applications)
	install (FILES misc/zany80.png DESTINATION share/pixmaps)
	install (TARGETS zany80
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
	)
endif()

if (DEFINED EXTRA_INSTALL_DIR)
	install(DIRECTORY "${EXTRA_INSTALL_DIR}/" DESTINATION .)
endif()

include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The Zany80 Fantasy Computer")
set(CPACK_PACKAGE_VENDOR "Noam Preil")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Zany80")
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\misc\\\\zany80.bmp")
set(CPACK_NSIS_INSTALLED_ICON_NAME "\\\\zany80.exe")
set(CPACK_NSIS_DISPLAY_NAME "Zany80 Fantasy Computer")
set(CPACK_NSIS_PACKAGE_NAME "Zany80 Fantasy Computer")
set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\zany80.github.io")
set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\zany80.github.io")
set(CPACK_NSIS_CONTACT "pleasantatk@gmail.com")
set(CPACK_NSIS_MODIFY_PATH "OFF")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL "ON")
set(CPACK_NSIS_EXECUTABLES_DIRECTORY "./")
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "\\\\zany80.exe")
set(CPACK_NSIS_HELP_LINK "https://zany80.github.io")
set(CPACK_NSIS_URL_INFO_ABOUT "https://zany80.github.io")
set(CPACK_NSIS_CONTACT "Feel free to send me a message at pleasantatk@gmail.com for assistance!")
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}\\\\misc\\\\zany.ico")
set(CPACK_PACKAGE_EXECUTABLES "Zany80;Zany80")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "pixelherodev")
include(CPack)

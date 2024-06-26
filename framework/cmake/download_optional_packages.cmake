macro(get_optional_packages)
	set(options "")
	set(oneValueArgs TARGET)
	set(multiValueArgs PACKAGES)
	cmake_parse_arguments(
		FUNC_ARGS  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set(PACKAGES_TARGETS "")
	if (PACKAGE_MANAGER STREQUAL "FetchContent")
		include(FetchContent)
		if ("catch2" IN_LIST FUNC_ARGS_PACKAGES)
			set(CATCH_BUILD_TESTING CACHE BOOL OFF)
			set(CATCH_ENABLE_WERROR CACHE BOOL OFF)
			set(CATCH_INSTALL_DOCS CACHE BOOL OFF)
			set(CATCH_INSTALL_HELPERS CACHE BOOL OFF)
			FetchContent_Declare(catch2
				GIT_REPOSITORY "https://github.com/catchorg/Catch2.git"
				GIT_TAG "v2.13.0" # https://github.com/catchorg/Catch2/tags
				GIT_SHALLOW TRUE)
			FetchContent_MakeAvailable(catch2)
			list(APPEND PACKAGES_TARGETS Catch2)
		endif()
		if ("stb" IN_LIST FUNC_ARGS_PACKAGES)
			FetchContent_Declare(stb
				GIT_REPOSITORY "https://github.com/nothings/stb.git"
				GIT_SHALLOW TRUE)
			FetchContent_GetProperties(stb)
			if(NOT stb_POPULATED)
				FetchContent_Populate(stb)
			endif()

			add_library(stb INTERFACE)
			target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
			list(APPEND PACKAGES_TARGETS stb)
		endif()
		if ("assimp" IN_LIST FUNC_ARGS_PACKAGES)
			# We only want the library itself, no additional example/test/tool applications.
			set(ASSIMP_BUILD_ASSIMP_TOOLS CACHE BOOL OFF)
			set(ASSIMP_BUILD_TESTS CACHE BOOL OFF)
			set(ASSIMP_BUILD_INSTALL CACHE BOOL OFF)
			set(ASSIMP_NO_EPXORT CACHE BOOL ON)
			FetchContent_Declare(assimp
				GIT_REPOSITORY "https://github.com/assimp/assimp.git"
				GIT_TAG "v5.0.1" # https://github.com/assimp/assimp/tags
				GIT_SHALLOW TRUE)
			FetchContent_MakeAvailable(assimp)
			list(APPEND PACKAGES_TARGETS assimp)
		endif()
		if ("tinyobjloader" IN_LIST FUNC_ARGS_PACKAGES)
			FetchContent_Declare(tinyobjloader
				GIT_REPOSITORY "https://github.com/tinyobjloader/tinyobjloader.git"
				GIT_TAG "v2.0.0rc6" # https://github.com/tinyobjloader/tinyobjloader/tags
				GIT_SHALLOW TRUE)
			FetchContent_MakeAvailable(tinyobjloader)
			list(APPEND PACKAGES_TARGETS tinyobjloader)
		endif()
		if ("tbb" IN_LIST FUNC_ARGS_PACKAGES)
			FetchContent_Declare(tbb
				GIT_REPOSITORY "https://github.com/wjakob/tbb.git"
				GIT_TAG "141b0e310e1fb552bdca887542c9c1a8544d6503" # https://github.com/wjakob/tbb/commits/master
				GIT_SHALLOW TRUE)
			#FetchContent_MakeAvailable(tbb)

			FetchContent_GetProperties(tbb)
			if(NOT tbb_POPULATED)
			  FetchContent_Populate(tbb)
			  add_subdirectory(${tbb_SOURCE_DIR})
			  if (EXISTS "${tbb_BINARY_DIR}/tbbd.dll")
			  	configure_file("${tbb_BINARY_DIR}/tbbd.dll" "${CMAKE_BINARY_DIR}/tbbd.dll" COPYONLY)
			  endif()
			  if (EXISTS "${tbb_BINARY_DIR}/tbb.dll")
			  	configure_file("${tbb_BINARY_DIR}/tbb.dll" "${CMAKE_BINARY_DIR}/tbb.dll" COPYONLY)
			  endif()
			endif()

			list(APPEND PACKAGES_TARGETS tbb)
		endif()
	elseif ((PACKAGE_MANAGER STREQUAL "vcpkg") OR (PACKAGE_MANAGER STREQUAL "system"))
		if ("catch2" IN_LIST FUNC_ARGS_PACKAGES)
			find_package(Catch2 CONFIG REQUIRED)
			list(APPEND PACKAGES_TARGETS Catch2::Catch2)
		endif()
		if ("stb" IN_LIST FUNC_ARGS_PACKAGES)
			find_path(STB_INCLUDE_DIRS "stb.h")
			add_library(stb INTERFACE)
			target_include_directories(stb INTERFACE ${STB_INCLUDE_DIRS})
			list(APPEND PACKAGES_TARGETS stb)
		endif()
		if ("assimp" IN_LIST FUNC_ARGS_PACKAGES)
			find_package(assimp CONFIG REQUIRED)
			list(APPEND PACKAGES_TARGETS assimp::assimp)
		endif()
		if ("tinyobjloader" IN_LIST FUNC_ARGS_PACKAGES)
			find_package(tinyobjloader CONFIG REQUIRED)
			list(APPEND PACKAGES_TARGETS tinyobjloader::tinyobjloader)
		endif()
		if ("tbb" IN_LIST FUNC_ARGS_PACKAGES)
			find_package(TBB CONFIG REQUIRED)
			list(APPEND PACKAGES_TARGETS TBB::tbb)
		endif()
	elseif (PACKAGE_MANAGER STREQUAL "conan")
		set(CONAN_REQUIRES "")
		if ("catch2" IN_LIST FUNC_ARGS_PACKAGES)
			list(APPEND CONAN_REQUIRES "catch2/2.13.0")
			list(APPEND PACKAGES_TARGETS CONAN_PKG::catch2)
		endif()
		if ("stb" IN_LIST FUNC_ARGS_PACKAGES)
			list(APPEND CONAN_REQUIRES "stb/20200203")
			list(APPEND PACKAGES_TARGETS CONAN_PKG::stb)
		endif()
		if ("assimp" IN_LIST FUNC_ARGS_PACKAGES)
			list(APPEND CONAN_REQUIRES "assimp/5.0.1")
			list(APPEND PACKAGES_TARGETS CONAN_PKG::assimp)
		endif()
		if ("tinyobjloader" IN_LIST FUNC_ARGS_PACKAGES)
			list(APPEND CONAN_REQUIRES "tinyobjloader/1.0.6")
			list(APPEND PACKAGES_TARGETS CONAN_PKG::tinyobjloader)
		endif()
		if ("tbb" IN_LIST FUNC_ARGS_PACKAGES)
			list(APPEND CONAN_REQUIRES "tbb/2020.2")
			list(APPEND PACKAGES_TARGETS CONAN_PKG::tbb)
		endif()
		if (CONAN_REQUIRES)
			conan_cmake_run(
				REQUIRES ${CONAN_REQUIRES}
				BASIC_SETUP CMAKE_TARGETS
				BUILD missing)
		endif()
	else()
		message(FATAL_ERROR "Unknown package manager ${PACKAGE_MANAGER}")
	endif()
	add_library(${FUNC_ARGS_TARGET} INTERFACE)
	target_link_libraries(${FUNC_ARGS_TARGET} INTERFACE ${PACKAGES_TARGETS})
endmacro()

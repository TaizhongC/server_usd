cmake_minimum_required(VERSION 3.16)

project(server_usd VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(ENABLE_USD_LITE "Link against libs/USD_Lite if present" ON)

# Third-party single-file libraries
add_library(mongoose STATIC libs/mongoose/mongoose.c)
target_include_directories(mongoose PUBLIC ${CMAKE_SOURCE_DIR}/libs/mongoose)
if(MSVC)
  target_compile_definitions(mongoose PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
set_source_files_properties(libs/mongoose/mongoose.c PROPERTIES LANGUAGE C)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
  src/*.cpp
)

add_executable(server_usd ${SOURCES})
# Enable SEH catching in Engine.cpp so hardware faults in USD get translated.
set_source_files_properties(src/core/Engine.cpp PROPERTIES COMPILE_OPTIONS "/EHa")
string(REPLACE "\\" "/" WEB_ROOT_PATH "${CMAKE_SOURCE_DIR}/web")
target_compile_definitions(server_usd PRIVATE WEB_ROOT_DIR="${WEB_ROOT_PATH}")
target_include_directories(server_usd PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/libs/json
  ${CMAKE_SOURCE_DIR}/libs/mongoose
)

target_link_libraries(server_usd PRIVATE mongoose)

if(MSVC)
  # Silence noisy compiler warnings; keep permissive- for stricter C++ conformance.
  target_compile_options(server_usd PRIVATE /W0 /permissive-)
  target_compile_definitions(server_usd PRIVATE _WINSOCKAPI_ WIN32_LEAN_AND_MEAN)
  target_link_libraries(server_usd PRIVATE ws2_32 secur32 crypt32 iphlpapi)
else()
  target_compile_options(server_usd PRIVATE -Wall -Wextra -Wpedantic)
  target_link_libraries(server_usd PRIVATE pthread)
endif()

# Optional USD_Lite linkage (requires libs/USD_Lite with pre-built binaries)
if(ENABLE_USD_LITE)
  set(USD_ROOT "${CMAKE_SOURCE_DIR}/libs/USD_Lite")
  if(EXISTS "${USD_ROOT}/include" AND EXISTS "${USD_ROOT}/lib")
    set(USD_BIN_DIR "${USD_ROOT}/bin")
    set(USD_LIB_BIN "${USD_ROOT}/lib")
    target_include_directories(server_usd PRIVATE ${USD_ROOT}/include)
    target_link_directories(server_usd PRIVATE ${USD_ROOT}/lib)
    target_compile_definitions(server_usd PRIVATE PXR_MONOLITHIC_BUILD NOMINMAX HAS_USD_LITE=1)
    target_link_libraries(server_usd PRIVATE usd_ms)

    add_custom_command(TARGET server_usd POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${USD_BIN_DIR}/usd_ms.dll"
      "${USD_BIN_DIR}/tbb.dll"
      "${USD_BIN_DIR}/tbbmalloc.dll"
      $<TARGET_FILE_DIR:server_usd>
    )
    add_custom_command(TARGET server_usd POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${USD_LIB_BIN}/usd"
      "$<TARGET_FILE_DIR:server_usd>/usd"
    )
  else()
    message(WARNING "USD_Lite not found under vendor/USD_Lite; building without USD.")
  endif()
endif()

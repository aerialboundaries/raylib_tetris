cmake_minimum_required(VERSION 3.14)

get_filename_component(PROJECT_EXEC_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_EXEC_NAME} C)

# 1. 基本設定
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wall -Wextra -pedantic-errors)

# 3. ソースファイルの自動収集
file(GLOB SRCS "*.c")
file(GLOB HDRS "*.h")

# 4. raylibの依存関係取得
if(EMSCRIPTEN)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG       6.0 
        GIT_SHALLOW    TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
elseif(NOT CMAKE_CROSSCOMPILING)
    find_package(raylib CONFIG QUIET)
endif()

# 5. 実行ファイルの定義
add_executable(${PROJECT_NAME} ${SRCS})

# 6. インクルードパスの設定
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()

# 7. プラットフォーム別のビルド・リンク設定
if(EMSCRIPTEN)
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")

    set(EMSCRIPTEN_LINK_FLAGS "-s USE_GLFW=3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Font")
        string(APPEND EMSCRIPTEN_LINK_FLAGS " --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Font@/Font")
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Sound")
        string(APPEND EMSCRIPTEN_LINK_FLAGS " --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Sound@/Sound")
    endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${EMSCRIPTEN_LINK_FLAGS}")

elseif(WIN32 OR MINGW OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".exe")

    # 独立した raylib のパス指定（WSL経由 / Windows直接の双方に対応）
    if(EXISTS "/mnt/c/raylib/include")
        target_include_directories(${PROJECT_NAME} BEFORE PRIVATE "/mnt/c/raylib/include")
        target_link_directories(${PROJECT_NAME} PRIVATE "/mnt/c/raylib/lib")
    elseif(EXISTS "C:/raylib/include")
        target_include_directories(${PROJECT_NAME} BEFORE PRIVATE "C:/raylib/include")
        target_link_directories(${PROJECT_NAME} PRIVATE "C:/raylib/lib")
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE raylib opengl32 gdi32 winmm)

else()
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib GL m pthread dl rt X11)
endif()

# 8. カスタムターゲット: make run の定義
if(NOT EMSCRIPTEN)
    add_custom_target(run
        COMMAND $<TARGET_FILE:${PROJECT_NAME}>
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "--- プログラムを実行します ---"
    )
endif()

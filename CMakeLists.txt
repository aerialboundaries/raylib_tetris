cmake_minimum_required(VERSION 3.14)

# カレントフォルダ名をプロジェクト名にする

get_filename_component(PROJECT_EXEC_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_EXEC_NAME} C)

# 1. 基本設定
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wall -Wextra -pedantic-errors)


# 2. 実行ファイルの出力先をプロジェクトルートに指定

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

# 3. ソースファイルの自動収集
file(GLOB SRCS "*.c")
file(GLOB HDRS "*.h")

# 4. raylibの依存関係取得
# Web(Emscripten)のみFetchContentを使用し、Linux/Windowsネイティブ・クロスコンパイルは既存環境を使用
if(EMSCRIPTEN)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.5 # 公式最新タグ（6.0リリース時は 6.0 に変更可能）
        GIT_SHALLOW    TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
elseif(NOT CMAKE_CROSSCOMPILING)
    # Native Linux ビルドの時だけ find_package を実行
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
    # ----------------------------------------------------
    # Web (Emscripten) ビルド
    # ----------------------------------------------------
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
    
    # 出力ファイルの拡張子を .html に設定
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")

    # アセット（Font, Sound）のプリロード設定とEmscripten専用フラグ
    set(EMSCRIPTEN_LINK_FLAGS "-s USE_GLFW=3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1")
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Font")
        string(APPEND EMSCRIPTEN_LINK_FLAGS " --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Font@/Font")
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Sound")
        string(APPEND EMSCRIPTEN_LINK_FLAGS " --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Sound@/Sound")
    endif()

    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${EMSCRIPTEN_LINK_FLAGS}")


elseif(WIN32 OR MINGW OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # ----------------------------------------------------
    # Windows ビルド (Windowsネイティブ または WSLからのMinGWクロスコンパイル)
    # ----------------------------------------------------

    
    # 実行ファイルに .exe 拡張子を明示的に付与

    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".exe")

    # Windows用raylibのパス指定
    # WSL(Linux)側から参照する場合(/mnt/c/)とWindowsネイティブ(C:/)の両方に対応
    if(EXISTS "/mnt/c/raylib/w64devkit/include")
        target_include_directories(${PROJECT_NAME} BEFORE PRIVATE "/mnt/c/raylib/w64devkit/include")
        target_link_directories(${PROJECT_NAME} PRIVATE "/mnt/c/raylib/w64devkit/lib")
    elseif(EXISTS "C:/raylib/w64devkit/include")
        target_include_directories(${PROJECT_NAME} BEFORE PRIVATE "C:/raylib/w64devkit/include")

        target_link_directories(${PROJECT_NAME} PRIVATE "C:/raylib/w64devkit/lib")
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE raylib opengl32 gdi32 winmm)

else()
    # ----------------------------------------------------
    # Linux (WSL Native) ビルド
    # ----------------------------------------------------
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib GL m pthread dl rt X11)

endif()

# 8. カスタムターゲット: make run の定義 (Emscripten以外で有効)
if(NOT EMSCRIPTEN)
    add_custom_target(run
        COMMAND $<TARGET_FILE:${PROJECT_NAME}>
        DEPENDS ${PROJECT_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "--- プログラムを実行します ---"

    )
endif()

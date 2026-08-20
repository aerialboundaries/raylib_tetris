# ====================================================
# 汎用 CMakeLists.txt (-I 強制出力版)
# ====================================================
cmake_minimum_required(VERSION 3.12)

# カレントフォルダ名をプロジェクト名にする
get_filename_component(PROJECT_EXEC_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_EXEC_NAME} C)

# 1. 基本設定
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wall -Wextra -pedantic-errors)

# 2. 実行ファイルの出力先をプロジェクトルートに指定
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

# 3. ソースファイルの自動収集
file(GLOB SRCS "*.c")
file(GLOB HDRS "*.h")

# 4. 実行ファイルの定義
add_executable(${PROJECT_NAME} ${SRCS})

# 5. インクルードパスの設定（BEFOREを付与してCMakeによるフラグ除去を防ぎます）
target_include_directories(${PROJECT_NAME} BEFORE PRIVATE "C:/raylib/w64devkit/include")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
    target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()

# 6. ライブラリのリンク
if(WIN32 OR MINGW)
    target_link_directories(${PROJECT_NAME} PRIVATE "C:/raylib/w64devkit/lib")
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib opengl32 gdi32 winmm)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib GL m pthread dl rt X11)
endif()

# 7. カスタムターゲット: make run の定義
add_custom_target(run
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "--- プログラムを実行します ---"
)

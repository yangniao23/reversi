# CMakeのバージョンを設定
cmake_minimum_required(VERSION 3.13)
# プロジェクト名と使用する言語を設定
project(test_cmake C)
# a.outという実行ファイルをmain.cppとhello.cppから作成
file(GLOB MY_SOURCE *.h *.c)


add_subdirectory(zhash-c)

add_compile_options(-Wall -Werror -W -O0 -fsplit-stack -g)
#add_compile_options(-Wall -Werror -W -O3)
add_executable(reversi.out ${MY_SOURCE})
include_directories(zhash-c/src)
target_link_libraries(reversi.out zhash-c)
# CMakeのバージョンを設定
cmake_minimum_required(VERSION 3.13)
# プロジェクト名と使用する言語を設定
project(test_cmake C)
# a.outという実行ファイルをmain.cppとhello.cppから作成
file(GLOB MY_SOURCE **.h **.c)

enable_testing()


add_compile_options(-Wall -O0 -g)
add_executable(reversi ${MY_SOURCE})

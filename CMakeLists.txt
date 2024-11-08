cmake_minimum_required(VERSION 3.5.1)

project(spoke_remote)

add_executable( spoke_remote
                spoke_remote.cpp
)

target_link_libraries( spoke_remote raylib )


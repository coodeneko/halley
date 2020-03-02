if (MSVC)
    find_package(yaml-cpp CONFIG REQUIRED)
    set(YAMLCPP_LIBRARY "yaml-cpp")
else ()
    find_package(YamlCpp)
endif ()

find_package(Boost COMPONENTS system filesystem REQUIRED)
find_Package(Freetype REQUIRED)

add_subdirectory(tools)
add_subdirectory(cmd)
add_subdirectory(editor)
add_subdirectory(runner)

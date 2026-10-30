# CMakeList.txt: проект CMake для software3lab; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#

# Добавьте источник в исполняемый файл этого проекта.
add_executable (software3lab "software3lab.cpp" "software3lab.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET software3lab PROPERTY CXX_STANDARD 20)
endif()

# TODO: Добавьте тесты и целевые объекты, если это необходимо.

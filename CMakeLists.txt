cmake_minimum_required(VERSION 3.16)
project(TodoApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
set(CMAKE_AUTOMOC ON)

# Core library sources
set(CORE_SOURCES
    src/core/models/Todo.cpp
    src/core/database/TodoDatabase.cpp
)

# GUI sources
set(GUI_SOURCES
    src/gui/main.cpp
    src/gui/MainWindow.cpp
    src/gui/AddTodoDialog.cpp
    src/gui/EditTodoDialog.cpp
)

# Create core library
add_library(TodoCore ${CORE_SOURCES})
target_include_directories(TodoCore PUBLIC src/core)

# Link SQLite3
find_library(SQLITE3_LIBRARY sqlite3)
target_link_libraries(TodoCore PUBLIC ${SQLITE3_LIBRARY})

# Create executable
add_executable(TodoApp ${GUI_SOURCES})

# Link libraries
target_link_libraries(TodoApp PRIVATE TodoCore Qt6::Core Qt6::Widgets)
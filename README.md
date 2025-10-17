# Qt6 Todo App

A simple desktop todo list app I built with Claude to learn Qt6 and CMake. Uses C++ and SQLite for storage.

## What It Does

Basic todo list with the essentials - add tasks, mark them complete, organize by category, set priorities and due dates. Nothing fancy, just a functional app to practice Qt development.

## Why Todo App
I wanted to make an app that would be cross compatible with my Macbook and my new "dumb" phone (Mudita Kompakt).

## Built With

- C++17
- Qt6 Widgets
- SQLite3
- CMake

## Build Requirements

- C++17 compatible compiler (GCC, Clang, or MSVC)
- CMake 3.16 or higher
- Qt6 (Core and Widgets modules)
- SQLite3

### macOS

```bash
# Install dependencies via Homebrew
brew install cmake qt@6 sqlite3
```

### Ubuntu/Debian

```bash
# Install dependencies via apt
sudo apt install cmake qt6-base-dev libsqlite3-dev
```

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
./TodoApp
```

## What I Learned

- Qt signals/slots and event handling
- Custom item rendering with QStyledItemDelegate
- SQLite C API basics
- CMake project setup
- Using smart pointers and modern C++ features

---

Built by Walker Thompson

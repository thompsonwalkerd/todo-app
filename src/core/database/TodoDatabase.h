#ifndef TODO_DATABASE_H
#define TODO_DATABASE_H

#include <string>
#include <vector>
#include <memory>
#include <sqlite3.h>
#include "../models/Todo.h"

class TodoDatabase {
private:
    sqlite3* db;  // Raw pointer to SQLite database
    std::string db_path;
    
    // Helper methods
    bool executeSQL(const std::string& sql);
    void handleError(const std::string& operation);

public:
    // Constructor/Destructor (RAII pattern)
    TodoDatabase(const std::string& path);
    ~TodoDatabase();
    
    // Prevent copying (database connections shouldn't be copied)
    TodoDatabase(const TodoDatabase&) = delete;
    TodoDatabase& operator=(const TodoDatabase&) = delete;
    
    // Database initialization
    bool initialize();
    
    // CRUD Operations
    bool createTodo(Todo& todo);  // Creates and sets the ID
    std::vector<Todo> getAllTodos();
    std::vector<Todo> getTodosByCategory(const std::string& category);
    std::unique_ptr<Todo> getTodoById(int id);
    bool updateTodo(const Todo& todo);
    bool deleteTodo(int id);
    
    // Category management
    std::vector<std::string> getAllCategories();
    
    // Utility
    bool isOpen() const { return db != nullptr; }
    void close();
};

#endif // TODO_DATABASE_H
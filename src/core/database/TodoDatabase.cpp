#include "TodoDatabase.h"
#include <iostream>
#include <sstream>

TodoDatabase::TodoDatabase(const std::string& path)
    : db(nullptr), db_path(path) {

    int result = sqlite3_open(path.c_str(), &db);

    if (result != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
    }
}

TodoDatabase::~TodoDatabase() {
    close();
}

void TodoDatabase::close() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool TodoDatabase::initialize() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS todos (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            description TEXT,
            category TEXT DEFAULT 'general',
            completed INTEGER DEFAULT 0,
            created_at INTEGER NOT NULL,
            updated_at INTEGER NOT NULL,
            due_date INTEGER,
            priority INTEGER DEFAULT 2,
            CHECK(priority >= 1 AND priority <= 3)
        );
        
        CREATE INDEX IF NOT EXISTS idx_category ON todos(category);
        CREATE INDEX IF NOT EXISTS idx_completed ON todos(completed);
        CREATE INDEX IF NOT EXISTS idx_due_date ON todos(due_date);
    )";
    
    return executeSQL(sql);
}

bool TodoDatabase::executeSQL(const std::string& sql) {
    if (!db) return false;

    char* error_msg = nullptr;
    int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error_msg);

    if (result != SQLITE_OK) {
        std::cerr << "SQL Error: " << error_msg << std::endl;
        sqlite3_free(error_msg);
        return false;
    }

    return true;
}

void TodoDatabase::handleError(const std::string& operation) {
    if (db) {
        std::cerr << operation << " failed: " << sqlite3_errmsg(db) << std::endl;
    }
}

bool TodoDatabase::createTodo(Todo& todo) {
    if (!db) return false;

    const char* sql = R"(
        INSERT INTO todos (title, description, category, completed,
                          created_at, updated_at, due_date, priority)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        handleError("Prepare INSERT");
        return false;
    }

    // Bind parameters (using prepared statements prevents SQL injection)
    sqlite3_bind_text(stmt, 1, todo.getTitle().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, todo.getDescription().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, todo.getCategory().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, todo.isCompleted() ? 1 : 0);
    sqlite3_bind_int64(stmt, 5, todo.getCreatedAt());
    sqlite3_bind_int64(stmt, 6, todo.getUpdatedAt());

    if (todo.getDueDate().has_value()) {
        sqlite3_bind_int64(stmt, 7, todo.getDueDate().value());
    } else {
        sqlite3_bind_null(stmt, 7);
    }

    sqlite3_bind_int(stmt, 8, todo.getPriority());

    int result = sqlite3_step(stmt);

    if (result == SQLITE_DONE) {
        int new_id = sqlite3_last_insert_rowid(db);
        todo.setId(new_id);
        sqlite3_finalize(stmt);
        return true;
    } else {
        handleError("Execute INSERT");
        sqlite3_finalize(stmt);
        return false;
    }
}

std::vector<Todo> TodoDatabase::getAllTodos() {
    std::vector<Todo> todos;
    if (!db) return todos;

    const char* sql = "SELECT * FROM todos ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        handleError("Prepare SELECT");
        return todos;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Todo todo;
        
        todo.setId(sqlite3_column_int(stmt, 0));
        todo.setTitle(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        
        if (sqlite3_column_text(stmt, 2)) {
            todo.setDescription(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        }
        
        if (sqlite3_column_text(stmt, 3)) {
            todo.setCategory(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        }
        
        todo.setCompleted(sqlite3_column_int(stmt, 4) == 1);

        // created_at and updated_at are stored but not settable via current API

        if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
            todo.setDueDate(sqlite3_column_int64(stmt, 7));
        }
        
        todo.setPriority(sqlite3_column_int(stmt, 8));
        
        todos.push_back(todo);
    }
    
    sqlite3_finalize(stmt);
    return todos;
}

std::vector<Todo> TodoDatabase::getTodosByCategory(const std::string& category) {
    std::vector<Todo> todos;
    if (!db) return todos;
    
    const char* sql = "SELECT * FROM todos WHERE category = ? ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        handleError("Prepare SELECT by category");
        return todos;
    }
    
    sqlite3_bind_text(stmt, 1, category.c_str(), -1, SQLITE_TRANSIENT);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Todo todo;
        todo.setId(sqlite3_column_int(stmt, 0));
        todo.setTitle(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        
        if (sqlite3_column_text(stmt, 2)) {
            todo.setDescription(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        }
        
        if (sqlite3_column_text(stmt, 3)) {
            todo.setCategory(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        }
        
        todo.setCompleted(sqlite3_column_int(stmt, 4) == 1);
        
        if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
            todo.setDueDate(sqlite3_column_int64(stmt, 7));
        }
        
        todo.setPriority(sqlite3_column_int(stmt, 8));
        todos.push_back(todo);
    }
    
    sqlite3_finalize(stmt);
    return todos;
}

std::unique_ptr<Todo> TodoDatabase::getTodoById(int id) {
    if (!db) return nullptr;
    
    const char* sql = "SELECT * FROM todos WHERE id = ?;";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        handleError("Prepare SELECT by ID");
        return nullptr;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auto todo = std::make_unique<Todo>();
        
        todo->setId(sqlite3_column_int(stmt, 0));
        todo->setTitle(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        
        if (sqlite3_column_text(stmt, 2)) {
            todo->setDescription(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        }
        
        if (sqlite3_column_text(stmt, 3)) {
            todo->setCategory(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        }
        
        todo->setCompleted(sqlite3_column_int(stmt, 4) == 1);
        
        if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
            todo->setDueDate(sqlite3_column_int64(stmt, 7));
        }
        
        todo->setPriority(sqlite3_column_int(stmt, 8));
        
        sqlite3_finalize(stmt);
        return todo;
    }
    
    sqlite3_finalize(stmt);
    return nullptr;
}

bool TodoDatabase::updateTodo(const Todo& todo) {
    if (!db) return false;
    
    const char* sql = R"(
        UPDATE todos 
        SET title = ?, description = ?, category = ?, completed = ?,
            updated_at = ?, due_date = ?, priority = ?
        WHERE id = ?;
    )";
    
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        handleError("Prepare UPDATE");
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, todo.getTitle().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, todo.getDescription().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, todo.getCategory().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, todo.isCompleted() ? 1 : 0);
    sqlite3_bind_int64(stmt, 5, todo.getUpdatedAt());
    
    if (todo.getDueDate().has_value()) {
        sqlite3_bind_int64(stmt, 6, todo.getDueDate().value());
    } else {
        sqlite3_bind_null(stmt, 6);
    }
    
    sqlite3_bind_int(stmt, 7, todo.getPriority());
    sqlite3_bind_int(stmt, 8, todo.getId());
    
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return result == SQLITE_DONE;
}

bool TodoDatabase::deleteTodo(int id) {
    if (!db) return false;
    
    const char* sql = "DELETE FROM todos WHERE id = ?;";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        handleError("Prepare DELETE");
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return result == SQLITE_DONE;
}

std::vector<std::string> TodoDatabase::getAllCategories() {
    std::vector<std::string> categories;
    if (!db) return categories;
    
    const char* sql = "SELECT DISTINCT category FROM todos ORDER BY category;";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        handleError("Prepare SELECT categories");
        return categories;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* cat = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (cat) {
            categories.push_back(cat);
        }
    }
    
    sqlite3_finalize(stmt);
    return categories;
}
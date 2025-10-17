#ifndef TODO_H
#define TODO_H

#include <string>
#include <ctime>
#include <optional>  // For optional due_date

class Todo {
private:
    int id;
    std::string title;
    std::string description;
    std::string category;  // "work", "personal", etc.
    bool completed;
    time_t created_at;
    time_t updated_at;
    std::optional<time_t> due_date;  // Optional - not all todos have deadlines
    int priority; // 1=low, 2=medium, 3=high

public:
    // Constructors
    Todo();
    Todo(const std::string& title, 
     const std::string& description = "", 
     const std::string& category = "general", 
     int priority = 2);
    
    // Getters
    int getId() const { return id; }
    std::string getTitle() const { return title; }
    std::string getDescription() const { return description; }
    std::string getCategory() const { return category; }
    bool isCompleted() const { return completed; }
    time_t getCreatedAt() const { return created_at; }
    time_t getUpdatedAt() const { return updated_at; }
    std::optional<time_t> getDueDate() const { return due_date; }
    int getPriority() const { return priority; }
    
    // Setters
    void setId(int newId) { id = newId; }
    void setTitle(const std::string& newTitle);
    void setDescription(const std::string& newDesc);
    void setCategory(const std::string& newCategory);
    void setCompleted(bool status);
    void setPriority(int newPriority);
    void setDueDate(time_t date);
    void clearDueDate();
    
    // Utility methods
    void updateTimestamp();
    bool isOverdue() const;  // Check if past due date and not completed
    int daysUntilDue() const;  // Returns days until due (negative if overdue)
};

#endif // TODO_H
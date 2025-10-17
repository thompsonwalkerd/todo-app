#include "Todo.h"
#include <cmath>

Todo::Todo() 
    : id(0), title(""), description(""), category("general"), completed(false), 
      priority(2), created_at(std::time(nullptr)), updated_at(std::time(nullptr)),
      due_date(std::nullopt) {  // nullopt means "no value"
}

Todo::Todo(const std::string& title, const std::string& description, 
           const std::string& category, int priority)
    : id(0), title(title), description(description), category(category),
      completed(false), priority(priority), 
      created_at(std::time(nullptr)), updated_at(std::time(nullptr)),
      due_date(std::nullopt) {
}

void Todo::setTitle(const std::string& newTitle) {
    title = newTitle;
    updateTimestamp();
}

void Todo::setDescription(const std::string& newDesc) {
    description = newDesc;
    updateTimestamp();
}

void Todo::setCategory(const std::string& newCategory) {
    category = newCategory;
    updateTimestamp();
}

void Todo::setCompleted(bool status) {
    completed = status;
    updateTimestamp();
}

void Todo::setPriority(int newPriority) {
    if (newPriority >= 1 && newPriority <= 3) {
        priority = newPriority;
        updateTimestamp();
    }
}

void Todo::setDueDate(time_t date) {
    due_date = date;
    updateTimestamp();
}

void Todo::clearDueDate() {
    due_date = std::nullopt;
    updateTimestamp();
}

void Todo::updateTimestamp() {
    updated_at = std::time(nullptr);
}

bool Todo::isOverdue() const {
    if (!due_date.has_value() || completed) {
        return false;  // No due date or already completed
    }
    
    return std::time(nullptr) > due_date.value();
}

int Todo::daysUntilDue() const {
    if (!due_date.has_value()) {
        return 0;  // No due date
    }
    
    time_t now = std::time(nullptr);
    double seconds_diff = std::difftime(due_date.value(), now);
    return static_cast<int>(seconds_diff / 86400);  // 86400 seconds in a day
}
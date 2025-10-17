#include "MainWindow.h"
#include "AddTodoDialog.h"
#include "EditTodoDialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QMenu>
#include <QGraphicsDropShadowEffect>
#include <QShortcut>
#include <QTimer>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    
    // Initialize database
    db = std::make_unique<TodoDatabase>("todos.db");
    
    if (!db->isOpen()) {
        QMessageBox::critical(this, "Error", "Failed to open database!");
        return;
    }
    
    if (!db->initialize()) {
        QMessageBox::critical(this, "Error", "Failed to initialize database!");
        return;
    }
    
    setupUI();
    connectSignals();
    loadTodos();
}

MainWindow::~MainWindow() {
    // Database automatically closes via RAII
}

void MainWindow::setupUI() {
    setWindowTitle("Todo");
    resize(700, 800);
    
    // Set app-wide stylesheet
    setStyleSheet(R"(
        QMainWindow {
            background-color: #FFFFFF;
        }
        QWidget {
            font-family: -apple-system, 'Helvetica Neue', sans-serif;
            font-size: 14px;
            color: #000000;
        }
        QListWidget {
            background-color: #FFFFFF;
            border: none;
            outline: none;
        }
        QListWidget::item {
            border-bottom: 1px solid #F0F0F0;
            padding: 0px;
            background-color: #FFFFFF;
            color: #000000;
        }
        QListWidget::item:selected {
            background-color: #F8F8F8;
            color: #000000;
        }
        QListWidget::item:hover {
            background-color: #F8F8F8;
            color: #000000;
        }
        QListWidget::item:selected:hover {
            background-color: #F0F0F0;
            color: #000000;
        }
        QPushButton {
            background-color: #000000;
            color: #FFFFFF;
            border: none;
            font-weight: 500;
            font-size: 24px;
        }
        QPushButton:hover {
            background-color: #2A2A2A;
        }
        QPushButton:pressed {
            background-color: #000000;
        }
    )");
    
    // Central widget
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // === Top Bar ===
    QWidget* topBarWidget = new QWidget(this);
    topBarWidget->setStyleSheet("background-color: #FFFFFF; border-bottom: 1px solid #E5E5E5;");
    topBar = new QHBoxLayout(topBarWidget);
    topBar->setContentsMargins(20, 16, 20, 16);
    
    // Current category label (replaces title)
    categoryFilter = new QComboBox(this);
    categoryFilter->setStyleSheet(R"(
        QComboBox {
            border: none;
            background: transparent;
            font-size: 20px;
            font-weight: 600;
            color: #000000;
            padding: 0;
        }
        QComboBox::drop-down {
            border: none;
            width: 0;
        }
        QComboBox::down-arrow {
            width: 0;
            height: 0;
        }
        QComboBox QAbstractItemView {
            border: 1px solid #E5E5E5;
            background-color: #FFFFFF;
            selection-background-color: #F5F5F5;
            selection-color: #000000;
            outline: none;
            font-size: 14px;
            font-weight: normal;
            padding: 4px;
        }
        QComboBox QAbstractItemView::item {
            padding: 8px 12px;
            border-radius: 4px;
        }
        )");
    categoryFilter->addItem("All");
    categoryFilter->setFocusPolicy(Qt::NoFocus);
    
    // Filter button (funnel icon)
    QPushButton* filterButton = new QPushButton("⋮", this);
    filterButton->setFixedSize(36, 36);
    filterButton->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            color: #000000;
            border: 1px solid #D1D1D1;
            border-radius: 18px;
            font-size: 20px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #F0F0F0;
            border-color: #8A8A8A;
        }
        QPushButton:pressed {
            background-color: #E5E5E5;
            border-color: #000000;
        }
    )");
    connect(filterButton, &QPushButton::clicked, this, [this, filterButton]() {
        // Show category menu
        QMenu menu(this);
        menu.setStyleSheet(R"(
            QMenu {
                background-color: #FFFFFF;
                border: 1px solid #D1D1D1;
                border-radius: 6px;
                padding: 4px;
            }
            QMenu::item {
                padding: 8px 16px;
                border-radius: 4px;
            }
            QMenu::item:selected {
                background-color: #F5F5F5;
            }
        )");
        
        QAction* allAction = menu.addAction("All");
        connect(allAction, &QAction::triggered, this, [this]() {
            categoryFilter->setCurrentIndex(0);
        });
        
        auto categories = db->getAllCategories();
        if (!categories.empty()) {
            menu.addSeparator();
            for (const auto& cat : categories) {
                QAction* action = menu.addAction(QString::fromStdString(cat));
                connect(action, &QAction::triggered, this, [this, cat]() {
                    for (int i = 0; i < categoryFilter->count(); i++) {
                        if (categoryFilter->itemText(i) == QString::fromStdString(cat)) {
                            categoryFilter->setCurrentIndex(i);
                            break;
                        }
                    }
                });
            }
        }
        
        // Show menu below button
        QPoint pos = filterButton->mapToGlobal(QPoint(0, filterButton->height() + 4));
        menu.exec(pos);
    });
    
    topBar->addWidget(categoryFilter);
    topBar->addStretch();
    topBar->addWidget(filterButton);
    
    mainLayout->addWidget(topBarWidget);
    
    // === Todo List ===
    todoList = new QListWidget(this);
    TodoItemDelegate* delegate = new TodoItemDelegate(this);
    todoList->setItemDelegate(delegate);

    // Connect checkbox click signal
    connect(delegate, &TodoItemDelegate::checkboxClicked, this, &MainWindow::onCheckboxClicked);

    mainLayout->addWidget(todoList);
    
    // === Bottom Status Bar ===
    QWidget* bottomBar = new QWidget(this);
    bottomBar->setStyleSheet("background-color: #FFFFFF;");
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(20, 16, 20, 16);
    
    statusLabel = new QLabel("0 items", this);
    statusLabel->setStyleSheet("color: #999999; font-size: 13px;");
    statusLabel->setAlignment(Qt::AlignCenter);
    
    bottomLayout->addWidget(statusLabel);
    
    mainLayout->addWidget(bottomBar);
    
    // === Floating Add Button (bottom right) ===
    addButton = new QPushButton("+", centralWidget);
    addButton->setFixedSize(56, 56);
    addButton->setStyleSheet(R"(
        QPushButton {
            background-color: #000000;
            color: #FFFFFF;
            border: none;
            border-radius: 28px;
            font-size: 28px;
            font-weight: 300;
            padding-bottom: 2px;
        }
        QPushButton:hover {
            background-color: #2A2A2A;
        }
        QPushButton:pressed {
            background-color: #000000;
        }
    )");

    // Add shadow effect for depth
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(12);
    shadow->setXOffset(0);
    shadow->setYOffset(4);
    shadow->setColor(QColor(0, 0, 0, 38));  // ~15% opacity black
    addButton->setGraphicsEffect(shadow);
    addButton->raise();  // Keep it on top
    
    // Position the floating button
    connect(centralWidget, &QWidget::destroyed, [this]() {
        // Cleanup if needed
    });
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    
    // Position floating add button in bottom right
    if (addButton && centralWidget) {
        int x = centralWidget->width() - addButton->width() - 20;
        int y = centralWidget->height() - addButton->height() - 20;
        addButton->move(x, y);
    }
}

void MainWindow::connectSignals() {
    connect(addButton, &QPushButton::clicked, this, &MainWindow::onAddTodo);
    connect(todoList, &QListWidget::itemClicked, this, &MainWindow::onTodoClicked);
    connect(categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onCategoryFilterChanged);
}

void MainWindow::loadTodos() {
    refreshTodoList();
    
    // Load categories into filter
    auto categories = db->getAllCategories();
    categoryFilter->clear();
    categoryFilter->addItem("All");
    for (const auto& cat : categories) {
        categoryFilter->addItem(QString::fromStdString(cat));
    }
}

void MainWindow::refreshTodoList() {
    todoList->clear();
    
    // Get todos based on filter
    std::vector<Todo> todos;
    QString currentFilter = categoryFilter->currentText();
    
    if (currentFilter == "All") {
        todos = db->getAllTodos();
    } else {
        todos = db->getTodosByCategory(currentFilter.toStdString());
    }
    
    // Sort: incomplete first, then by priority, then by due date
    std::sort(todos.begin(), todos.end(), [](const Todo& a, const Todo& b) {
        if (a.isCompleted() != b.isCompleted()) return !a.isCompleted();
        if (a.getPriority() != b.getPriority()) return a.getPriority() > b.getPriority();
        
        bool aDue = a.getDueDate().has_value();
        bool bDue = b.getDueDate().has_value();
        if (aDue != bDue) return aDue;
        if (aDue && bDue) return a.getDueDate().value() < b.getDueDate().value();
        
        return false;
    });
    
    // Add each todo to the list
    for (const auto& todo : todos) {
        QString itemText;
        
        // Keep priority indicator in data for sorting/logic
        if (todo.getPriority() == 3 && !todo.isCompleted()) {
            itemText += "● ";
        } else {
            itemText += "  ";
        }
        
        // Title
        itemText += QString::fromStdString(todo.getTitle());
        itemText += "\n";
        
        // Metadata line - will be shown in tiny uppercase
        QString metadata = "";
        
        // Category (only in "All" view)
        if (currentFilter == "All" && !todo.getCategory().empty()) {
            metadata += QString::fromStdString(todo.getCategory());
        }
        
        // Due date info
        if (todo.isOverdue() && !todo.isCompleted()) {
            if (!metadata.isEmpty()) metadata += " • ";
            metadata += "overdue";
        } else if (todo.getDueDate().has_value() && !todo.isCompleted()) {
            int days = todo.daysUntilDue();
            if (days == 0) {
                if (!metadata.isEmpty()) metadata += " • ";
                metadata += "due today";
            } else if (days == 1) {
                if (!metadata.isEmpty()) metadata += " • ";
                metadata += "due tomorrow";
            } else if (days > 0 && days <= 7) {
                if (!metadata.isEmpty()) metadata += " • ";
                metadata += QString("due in %1d").arg(days);
            }
        }
        
        itemText += metadata;
        
        QListWidgetItem* item = new QListWidgetItem(itemText);
        
        // Completed styling
        if (todo.isCompleted()) {
            QFont font = item->font();
            font.setStrikeOut(true);
            item->setFont(font);
            item->setForeground(QColor("#E0E0E0"));
        } 
        // Overdue styling
        else if (todo.isOverdue()) {
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }
        // High priority styling
        else if (todo.getPriority() == 3) {
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }
        
        // Store todo ID
        item->setData(Qt::UserRole, todo.getId());
        
        todoList->addItem(item);
    }
    
    updateStatusBar();
}

void MainWindow::updateStatusBar() {
    auto todos = db->getAllTodos();
    int total = todos.size();
    int completed = 0;
    int overdue = 0;
    
    for (const auto& todo : todos) {
        if (todo.isCompleted()) completed++;
        if (todo.isOverdue() && !todo.isCompleted()) overdue++;
    }
    
    QString status;
    if (total == 0) {
        status = "No items";
    } else if (total == 1) {
        status = "1 item";
    } else {
        status = QString("%1 items").arg(total);
    }
    
    if (completed > 0) {
        status += QString(" · %1 completed").arg(completed);
    }
    
    if (overdue > 0) {
        status += QString(" · %1 overdue").arg(overdue);
    }
    
    statusLabel->setText(status);
}

void MainWindow::onAddTodo() {
    AddTodoDialog dialog(db.get(), this);
    
    if (dialog.exec() == QDialog::Accepted) {
        Todo newTodo = dialog.getTodo();
        
        if (db->createTodo(newTodo)) {
            std::cout << "Created todo: " << newTodo.getTitle() << std::endl;
            loadTodos();
        } else {
            QMessageBox::warning(this, "Error", "Failed to create todo!");
        }
    }
}

void MainWindow::onTodoClicked(QListWidgetItem* item) {
    // Don't open dialog if checkbox was clicked
    if (checkboxWasClicked) {
        return;
    }

    int todoId = item->data(Qt::UserRole).toInt();
    auto todo = db->getTodoById(todoId);

    if (!todo) return;
    
    // Create custom dialog
    QDialog dialog(this);
    dialog.setWindowTitle(" ");
    dialog.setMinimumWidth(400);
    dialog.setStyleSheet(R"(
        QDialog {
            background-color: #FFFFFF;
        }
        QLabel {
            color: #000000;
        }
        QPushButton {
            border: 1px solid #E5E5E5;
            padding: 12px 24px;
            border-radius: 8px;
            background-color: #FFFFFF;
            font-weight: 500;
            font-size: 14px;
            color: #000000;
        }
        QPushButton:hover {
            background-color: #FAFAFA;
            border-color: #000000;
        }
    )");
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    
    // Title
    QLabel* titleLabel = new QLabel(QString::fromStdString(todo->getTitle()), &dialog);
    titleLabel->setStyleSheet("font-size: 24pt; font-weight: bold; color: #000000;");
    titleLabel->setWordWrap(true);
    layout->addWidget(titleLabel);

    layout->addSpacing(12);

    // Category (if exists)
    if (!todo->getCategory().empty()) {
        QLabel* catLabelHeader = new QLabel("CATEGORY", &dialog);
        catLabelHeader->setStyleSheet("color: #999999; font-size: 10px; font-weight: 600; letter-spacing: 1.2px;");
        layout->addWidget(catLabelHeader);

        QLabel* catValue = new QLabel(QString::fromStdString(todo->getCategory()), &dialog);
        catValue->setStyleSheet("color: #000000; font-size: 14px; margin-top: 2px;");
        layout->addWidget(catValue);

        layout->addSpacing(12);
    }

    // Priority
    QLabel* priorityLabelHeader = new QLabel("PRIORITY", &dialog);
    priorityLabelHeader->setStyleSheet("color: #999999; font-size: 10px; font-weight: 600; letter-spacing: 1.2px;");
    layout->addWidget(priorityLabelHeader);

    QHBoxLayout* priorityLayout = new QHBoxLayout();
    priorityLayout->setSpacing(4);
    priorityLayout->setContentsMargins(0, 2, 0, 0);

    int priority = todo->getPriority();
    for (int i = 1; i <= 3; i++) {
        QLabel* bar = new QLabel(&dialog);
        bar->setFixedSize(30, 6);
        QString color = (i <= priority) ? "#000000" : "#E5E5E5";
        bar->setStyleSheet("background-color: " + color + "; border-radius: 3px;");
        priorityLayout->addWidget(bar);
    }
    priorityLayout->addStretch();

    layout->addLayout(priorityLayout);
    layout->addSpacing(12);

    // Due date (if exists)
    if (todo->getDueDate().has_value()) {
        QLabel* dueLabelHeader = new QLabel("DUE DATE", &dialog);
        dueLabelHeader->setStyleSheet("color: #999999; font-size: 10px; font-weight: 600; letter-spacing: 1.2px;");
        layout->addWidget(dueLabelHeader);

        QDateTime dt = QDateTime::fromSecsSinceEpoch(todo->getDueDate().value());
        QLabel* dueValue = new QLabel(dt.toString("dd.MM.yyyy"), &dialog);
        dueValue->setStyleSheet("color: #000000; font-size: 14px; margin-top: 2px;");
        layout->addWidget(dueValue);

        layout->addSpacing(12);
    }

    // Description (if exists)
    if (!todo->getDescription().empty()) {
        QLabel* descLabelHeader = new QLabel("DESCRIPTION", &dialog);
        descLabelHeader->setStyleSheet("color: #999999; font-size: 10px; font-weight: 600; letter-spacing: 1.2px;");
        layout->addWidget(descLabelHeader);

        QLabel* descValue = new QLabel(QString::fromStdString(todo->getDescription()), &dialog);
        descValue->setStyleSheet("color: #000000; font-size: 14px; line-height: 1.5; margin-top: 2px;");
        descValue->setWordWrap(true);
        layout->addWidget(descValue);

        layout->addSpacing(12);
    }
    
    layout->addSpacing(8);
    
    // Buttons - all on one row
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);

    QPushButton* editBtn = new QPushButton("Edit", &dialog);
    editBtn->setFixedSize(70, 32);
    editBtn->setStyleSheet(R"(
        QPushButton {
            border: 1px solid #D1D1D1;
            background-color: #FFFFFF;
            color: #000000;
            padding: 6px 12px;
            border-radius: 6px;
            font-weight: 500;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #F0F0F0;
            border-color: #8A8A8A;
        }
    )");

    QPushButton* toggleBtn = new QPushButton(
        todo->isCompleted() ? "Incomplete" : "Complete",
        &dialog
    );
    toggleBtn->setFixedSize(90, 32);
    toggleBtn->setStyleSheet(R"(
        QPushButton {
            border: 1px solid #D1D1D1;
            background-color: #FFFFFF;
            color: #000000;
            padding: 6px 12px;
            border-radius: 6px;
            font-weight: 500;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #F0F0F0;
            border-color: #8A8A8A;
        }
    )");

    QPushButton* deleteBtn = new QPushButton("Delete", &dialog);
    deleteBtn->setFixedSize(70, 32);
    deleteBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #FF3B30;
            color: #FFFFFF;
            border: none;
            padding: 6px 12px;
            border-radius: 6px;
            font-weight: 500;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #D32F28;
        }
    )");

    QPushButton* cancelBtn = new QPushButton("Done", &dialog);
    cancelBtn->setFixedSize(70, 32);
    cancelBtn->setStyleSheet(R"(
        QPushButton {
            border: 1px solid #D1D1D1;
            background-color: #FFFFFF;
            color: #000000;
            padding: 6px 12px;
            border-radius: 6px;
            font-weight: 500;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #F0F0F0;
            border-color: #8A8A8A;
        }
    )");

    buttonLayout->addWidget(editBtn);
    buttonLayout->addWidget(toggleBtn);
    buttonLayout->addWidget(deleteBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelBtn);

    layout->addLayout(buttonLayout);

    // Keyboard shortcuts - set shortcuts with proper context
    // On Mac, the "Delete" key is actually Backspace
    QShortcut* deleteShortcut = new QShortcut(QKeySequence(Qt::Key_Backspace), &dialog);
    QShortcut* escapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), &dialog);

    connect(deleteShortcut, &QShortcut::activated, deleteBtn, &QPushButton::click);
    connect(escapeShortcut, &QShortcut::activated, cancelBtn, &QPushButton::click);

    // Connect buttons
    connect(editBtn, &QPushButton::clicked, [&dialog, this, &todo]() {
        dialog.accept();  // Close detail dialog first

        EditTodoDialog editDialog(*todo, db.get(), this);
        if (editDialog.exec() == QDialog::Accepted) {
            Todo updatedTodo = editDialog.getTodo();
            if (db->updateTodo(updatedTodo)) {
                refreshTodoList();
            } else {
                QMessageBox::warning(this, "Error", "Failed to update todo!");
            }
        }
    });

    connect(toggleBtn, &QPushButton::clicked, [&dialog, this, &todo]() {
        todo->setCompleted(!todo->isCompleted());
        db->updateTodo(*todo);
        refreshTodoList();
        dialog.accept();
    });

    connect(deleteBtn, &QPushButton::clicked, [&dialog, this, todoId]() {
        QMessageBox msgBox(&dialog);
        msgBox.setWindowTitle("Delete Todo");
        msgBox.setText("Are you sure you want to delete this todo");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setIcon(QMessageBox::Warning);

        if (msgBox.exec() == QMessageBox::Yes) {
            db->deleteTodo(todoId);
            refreshTodoList();
            dialog.accept();
        }
    });

    connect(cancelBtn, &QPushButton::clicked, [&dialog]() {
        dialog.reject();
    });

    dialog.exec();
}

void MainWindow::onCategoryFilterChanged(int index) {
    refreshTodoList();
}

void MainWindow::onDeleteTodo() {
    // Handled in onTodoClicked
}

void MainWindow::onCheckboxClicked(const QModelIndex& index) {
    // Set flag to prevent dialog from opening
    checkboxWasClicked = true;

    // Get the todo ID from the clicked item
    QListWidgetItem* item = todoList->item(index.row());
    if (!item) return;

    int todoId = item->data(Qt::UserRole).toInt();
    auto todo = db->getTodoById(todoId);

    if (!todo) return;

    // Toggle completion status
    todo->setCompleted(!todo->isCompleted());
    db->updateTodo(*todo);

    // Defer refresh to avoid crash during event handling
    QTimer::singleShot(0, this, [this]() {
        refreshTodoList();
        // Reset flag after refresh
        checkboxWasClicked = false;
    });
}
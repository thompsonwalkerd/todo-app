#include "AddTodoDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>

AddTodoDialog::AddTodoDialog(TodoDatabase* database, QWidget *parent)
    : QDialog(parent) {

    setWindowTitle("New Todo");
    setMinimumWidth(500);
    setStyleSheet(R"(
        QDialog {
            background-color: #FFFFFF;
        }
        QLineEdit, QTextEdit {
            border: 1px solid #D1D1D1;
            padding: 8px;
            border-radius: 6px;
            background-color: #FFFFFF;
            font-size: 14px;
        }
        QLineEdit::placeholder, QTextEdit::placeholder {
            color: #AAAAAA;
        }
        QLineEdit:focus, QTextEdit:focus {
            border-color: #000000;
            border-width: 2px;
            outline: none;
        }
        QComboBox {
            border: 1px solid #D1D1D1;
            padding: 8px 12px;
            padding-right: 32px;
            background-color: #FFFFFF;
            border-radius: 6px;
        }
        QComboBox:focus {
            border-color: #000000;
            border-width: 2px;
            outline: none;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: center right;
            width: 20px;
            border: none;
            background: transparent;
        }
        QDateEdit {
            border: 1px solid #D1D1D1;
            padding: 8px 12px;
            padding-right: 32px;
            background-color: #FFFFFF;
            border-radius: 6px;
        }
        QDateEdit:disabled {
            background-color: #F5F5F5;
            color: #AAAAAA;
            border-color: #E5E5E5;
        }
        QDateEdit:focus {
            border-color: #000000;
            border-width: 2px;
            outline: none;
        }
        QDateEdit::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: center right;
            width: 20px;
            border: none;
            background: transparent;
        }
        QLabel {
            color: #000000;
            font-weight: 500;
        }
        QPushButton {
            border: 1px solid #D1D1D1;
            padding: 10px 20px;
            border-radius: 6px;
            background-color: #FFFFFF;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #F5F5F5;
            border-color: #000000;
        }
    )");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    
    // Form layout
    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(12);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    // Title (more prominent) with inline error
    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(6);

    titleInput = new QLineEdit(this);
    titleInput->setPlaceholderText("Add title");
    titleInput->setMinimumWidth(350);
    titleLayout->addWidget(titleInput);

    // Error label (hidden by default, shown inline with title)
    errorLabel = new QLabel(this);
    errorLabel->setStyleSheet(R"(
        QLabel {
            color: #FF3B30;
            font-size: 12px;
            padding: 4px 0px;
        }
    )");
    errorLabel->setWordWrap(true);
    errorLabel->hide();
    titleLayout->addWidget(errorLabel);

    formLayout->addRow("Title", titleLayout);
    
    // Description
    descriptionInput = new QTextEdit(this);
    descriptionInput->setPlaceholderText("Optional details...");
    descriptionInput->setMaximumHeight(80);
    formLayout->addRow("Notes", descriptionInput);
    
    // Category (non-editable dropdown)
    categoryInput = new QComboBox(this);
    categoryInput->setEditable(false);

    // Add empty option first
    categoryInput->addItem("None", "");

    // Populate with existing categories from database
    auto categories = database->getAllCategories();
    for (const auto& cat : categories) {
        if (!cat.empty()) {
            categoryInput->addItem(QString::fromStdString(cat));
        }
    }

    formLayout->addRow("Category", categoryInput);
    
    // Priority
    priorityCombo = new QComboBox(this);
    priorityCombo->addItem("Low", 1);
    priorityCombo->addItem("Medium", 2);
    priorityCombo->addItem("High", 3);
    priorityCombo->setCurrentIndex(1);
    formLayout->addRow("Priority", priorityCombo);
    
    // Due date
    hasDueDateCheckbox = new QCheckBox("Set due date", this);
    dueDateInput = new QDateEdit(this);
    dueDateInput->setCalendarPopup(true);
    dueDateInput->setDisplayFormat("dd.MM.yyyy");
    dueDateInput->setDate(QDate::currentDate());
    dueDateInput->setEnabled(false);
    
    QHBoxLayout* dueDateLayout = new QHBoxLayout();
    dueDateLayout->addWidget(hasDueDateCheckbox);
    dueDateLayout->addWidget(dueDateInput);
    dueDateLayout->addStretch();
    formLayout->addRow("Due", dueDateLayout);
    
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(0);

    cancelButton = new QPushButton("Cancel", this);
    cancelButton->setFixedSize(70, 32);
    cancelButton->setStyleSheet(R"(
        QPushButton {
            border: 1px solid #D1D1D1;
            padding: 6px 12px;
            border-radius: 6px;
            background-color: #FFFFFF;
            color: #000000;
            font-weight: 500;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #F0F0F0;
            border-color: #8A8A8A;
        }
        QPushButton:pressed {
            background-color: #E5E5E5;
        }
    )");

    saveButton = new QPushButton("Save", this);
    saveButton->setFixedSize(70, 32);
    saveButton->setStyleSheet(R"(
        QPushButton {
            background-color: #000000;
            color: #FFFFFF;
            border: none;
            padding: 6px 12px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #2A2A2A;
        }
        QPushButton:pressed {
            background-color: #000000;
        }
    )");

    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);

    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(saveButton, &QPushButton::clicked, this, &AddTodoDialog::onSave);
    connect(cancelButton, &QPushButton::clicked, this, &AddTodoDialog::onCancel);
    connect(hasDueDateCheckbox, &QCheckBox::toggled, this, &AddTodoDialog::onDueDateToggled);

    // Keyboard shortcuts
    saveButton->setShortcut(QKeySequence(Qt::Key_Return));
    cancelButton->setShortcut(QKeySequence(Qt::Key_Escape));

    // Focus title input
    titleInput->setFocus();
}

void AddTodoDialog::onDueDateToggled(bool checked) {
    dueDateInput->setEnabled(checked);
}

void AddTodoDialog::onSave() {
    QString title = titleInput->text().trimmed();

    if (title.isEmpty()) {
        showError("Title cannot be empty");
        titleInput->setFocus();
        return;
    }

    clearError();

    // Create todo
    todo = Todo(
        title.toStdString(),
        descriptionInput->toPlainText().toStdString(),
        categoryInput->currentText().trimmed().toStdString(),
        priorityCombo->currentData().toInt()
    );

    // Set due date if checked
    if (hasDueDateCheckbox->isChecked()) {
        QDate date = dueDateInput->date();
        QDateTime dateTime(date, QTime(23, 59, 59));  // End of day
        todo.setDueDate(dateTime.toSecsSinceEpoch());
    }

    accept();  // Close dialog with success
}

void AddTodoDialog::showError(const QString& message) {
    errorLabel->setText(message);
    errorLabel->show();
}

void AddTodoDialog::clearError() {
    errorLabel->hide();
}

void AddTodoDialog::onCancel() {
    reject();  // Close dialog without saving
}
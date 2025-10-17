#ifndef ADDTODODIALOG_H
#define ADDTODODIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDateEdit>
#include <QCheckBox>
#include <QLabel>
#include "models/Todo.h"
#include "database/TodoDatabase.h"

class AddTodoDialog : public QDialog {
    Q_OBJECT

private:
    QLineEdit* titleInput;
    QTextEdit* descriptionInput;
    QComboBox* categoryInput;
    QComboBox* priorityCombo;
    QCheckBox* hasDueDateCheckbox;
    QDateEdit* dueDateInput;
    QLabel* errorLabel;

    QPushButton* saveButton;
    QPushButton* cancelButton;

    Todo todo;

    void showError(const QString& message);
    void clearError();

private slots:
    void onSave();
    void onCancel();
    void onDueDateToggled(bool checked);

public:
    AddTodoDialog(TodoDatabase* database, QWidget *parent = nullptr);
    Todo getTodo() const { return todo; }
};

#endif // ADDTODODIALOG_H
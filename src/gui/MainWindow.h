#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QComboBox>
#include <QLabel>
#include <QMenu>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QShortcut>
#include <QEvent>
#include <QMouseEvent>
#include <QTimer>
#include <memory>
#include "database/TodoDatabase.h"
#include "models/Todo.h"

class TodoItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    TodoItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    bool editorEvent(QEvent* event, QAbstractItemModel* model,
                     const QStyleOptionViewItem& option, const QModelIndex& index) override {
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

            // Check if click is within checkbox area (relative to item rect)
            int leftMargin = 18;
            int checkboxSize = 22;
            int checkboxX = option.rect.left() + leftMargin;
            int checkboxY = option.rect.top() + 18;

            QRect checkboxRect(checkboxX, checkboxY, checkboxSize, checkboxSize);

            if (checkboxRect.contains(mouseEvent->pos())) {
                // Checkbox was clicked - emit a signal to toggle completion
                emit checkboxClicked(index);
                return true;
            }
        }
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // Draw background with improved hover state
        if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, QColor("#F8F8F8"));  // More visible hover
        } else {
            painter->fillRect(option.rect, QColor("#FFFFFF"));
        }

        // Get data
        QString text = index.data(Qt::DisplayRole).toString();
        QColor textColor = index.data(Qt::ForegroundRole).value<QColor>();
        bool isCompleted = (textColor == QColor("#E0E0E0"));
        bool hasStrikethrough = index.data(Qt::FontRole).value<QFont>().strikeOut();

        QStringList lines = text.split('\n');
        QString title = lines.size() > 0 ? lines[0] : "";
        QString metadata = lines.size() > 1 ? lines[1] : "";

        // Remove priority indicator from title for display
        if (title.startsWith("● ")) {
            title = title.mid(2);
        } else if (title.startsWith("  ")) {
            title = title.mid(2);
        }

        int leftMargin = 18;
        int checkboxSize = 22;
        int checkboxX = leftMargin;
        int checkboxY = option.rect.top() + 18;

        // Check if mouse is over checkbox area for enhanced hover
        bool isHoveringCheckbox = false;
        if (option.state & QStyle::State_MouseOver) {
            // Checkbox hover area is slightly larger for better UX
            QRect checkboxRect(checkboxX - 4, checkboxY - 4, checkboxSize + 8, checkboxSize + 8);
            isHoveringCheckbox = true;  // Simplified - could track actual mouse pos
        }

        // Draw checkbox circle with enhanced hover state
        QColor checkboxBorder = isCompleted ? QColor("#D0D0D0") :
                                (isHoveringCheckbox ? QColor("#A0A0A0") : QColor("#D1D1D1"));
        painter->setPen(QPen(checkboxBorder, 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(checkboxX, checkboxY, checkboxSize, checkboxSize);

        // Draw checkmark if completed with smoother curves
        if (isCompleted) {
            painter->setPen(QPen(QColor("#B0B0B0"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            QPainterPath checkmark;
            checkmark.moveTo(checkboxX + 6, checkboxY + 11);
            checkmark.lineTo(checkboxX + 9, checkboxY + 15);
            checkmark.lineTo(checkboxX + 16, checkboxY + 7);
            painter->drawPath(checkmark);
        }
        
        int textStartX = leftMargin + checkboxSize + 14;

        // Draw title
        QFont titleFont = option.font;
        titleFont.setPointSize(16);
        titleFont.setWeight(QFont::Medium);
        painter->setFont(titleFont);
        painter->setPen(textColor);

        QRect titleRect(textStartX, option.rect.top() + 15, option.rect.width() - textStartX - 16, 26);
        
        // Draw title with strikethrough if needed
        if (hasStrikethrough) {
            QFontMetrics fm(titleFont);
            int textWidth = fm.horizontalAdvance(title);
            painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, title);
            
            // Draw strikethrough line
            int lineY = titleRect.top() + titleRect.height() / 2;
            painter->setPen(QPen(textColor, 1.5));
            painter->drawLine(textStartX, lineY, textStartX + textWidth, lineY);
        } else {
            painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, title);
        }
        
        // Draw metadata (category, due date) - tiny and subtle
        if (!metadata.trimmed().isEmpty()) {
            QFont metaFont = option.font;
            metaFont.setPointSize(11);
            metaFont.setCapitalization(QFont::AllUppercase);
            metaFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
            metaFont.setWeight(QFont::Normal);
            painter->setFont(metaFont);
            painter->setPen(QColor("#A8A8A8"));

            QRect metaRect(textStartX, option.rect.top() + 43, option.rect.width() - textStartX - 16, 18);
            painter->drawText(metaRect, Qt::AlignLeft | Qt::AlignVCenter, metadata.trimmed());
        }
        
        // Draw bottom border
        painter->setPen(QColor("#F0F0F0"));
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
        
        painter->restore();
    }
    
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QString text = index.data(Qt::DisplayRole).toString();
        QStringList lines = text.split('\n');
        bool hasMetadata = lines.size() > 1 && !lines[1].trimmed().isEmpty();

        return QSize(option.rect.width(), hasMetadata ? 76 : 60);
    }

signals:
    void checkboxClicked(const QModelIndex& index);
};

class MainWindow : public QMainWindow {
    Q_OBJECT  // Required for Qt signals/slots

private:
    // Database
    std::unique_ptr<TodoDatabase> db;

    // UI Components
    QWidget* centralWidget;
    QVBoxLayout* mainLayout;

    // Top bar
    QHBoxLayout* topBar;
    QComboBox* categoryFilter;
    QPushButton* addButton;

    // Todo list
    QListWidget* todoList;

    // Flag to prevent dialog opening when checkbox is clicked
    bool checkboxWasClicked = false;
    
    // Bottom bar
    QLabel* statusLabel;
    
    // Helper methods
    void setupUI();
    void connectSignals();
    void loadTodos();
    void refreshTodoList();
    void updateStatusBar();

private slots:
    void onAddTodo();
    void onTodoClicked(QListWidgetItem* item);
    void onCategoryFilterChanged(int index);
    void onDeleteTodo();
    void onCheckboxClicked(const QModelIndex& index);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent* event) override;
};

#endif // MAINWINDOW_H
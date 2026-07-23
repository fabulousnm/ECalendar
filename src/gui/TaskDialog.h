//TaskDialog.h 任务创建与编辑
#ifndef ECALENDER_TASKDIALOG_H
#define ECALENDER_TASKDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QRadioButton>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

#include "core/Task.h"


class TaskDialog : public QDialog {
    Q_OBJECT//qt宏

public:
    
    explicit TaskDialog(QWidget* parent = nullptr);

     //编辑or创建模式
    explicit TaskDialog(bool editMode, const Task& task, QWidget* parent = nullptr);

   
    Task getTask() const;

private slots:
    //确认按钮
    void onConfirmClicked();

private:
    bool           m_editMode;     // true=编辑模式, false=添加模式
    Task           m_editTask;     // 编辑模式下预填的任务

    QLineEdit*     m_nameEdit;     // 任务名称输入框
    QDateTimeEdit* m_startTimeEdit;// 开始时间选择
    QRadioButton*  m_highRadio;    // 高优先级单选按钮
    QRadioButton*  m_mediumRadio;  // 中优先级单选按钮
    QRadioButton*  m_lowRadio;     // 低优先级单选按钮
    QComboBox*     m_categoryCombo;// 分类下拉框
    QDateTimeEdit* m_remindTimeEdit;// 提醒时间选择
    QLabel*        m_errorLabel;   // 错误提示标签

    void setupUI();
};

#endif // ECALENDER_TASKDIALOG_H


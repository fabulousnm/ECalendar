//

#ifndef ECALENDER_REMINDERPOPUP_H
#define ECALENDER_REMINDERPOPUP_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>

#include "core/Task.h"

//
class ReminderPopup : public QDialog {
    Q_OBJECT

public:
    /*用户操作结果,我知道了或五分钟后再提醒 */
    enum Action { DISMISS = 0, SNOOZE = 1 };


    explicit ReminderPopup(const Task& task, QWidget* parent = nullptr);

    /** 获取用户的选择 */
    Action action() const { return m_action; }

private slots:
    void onDismissClicked();
    void onSnoozeClicked();

private:
    Task     m_task;
    Action   m_action = DISMISS;

    void setupUI();
};

#endif // ECALENDER_REMINDERPOPUP_H


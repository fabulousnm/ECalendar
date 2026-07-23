//ReminderPopup.cpp

#include "gui/ReminderPopup.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QFont>
#include <QApplication>
#include <QScreen>


ReminderPopup::ReminderPopup(const Task& task, QWidget* parent)
    : QDialog(parent)
    , m_task(task)
    , m_action(DISMISS)
{
    // 无标题栏、始终居于顶层
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setFixedSize(380, 260);
    setObjectName("reminderPopup");

    setupUI();

    // 居中显示
    if (parent) {
        QRect parentRect = parent->geometry();//获取父窗口位置大小
        move(parentRect.center() - rect().center());//作差
    } else {
        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            QRect screenRect = screen->availableGeometry();//屏幕可用区域
            move(screenRect.center() - rect().center());
        }
    }
}

// 初始化界面
void ReminderPopup::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(10);

    //  标题栏
    QHBoxLayout* titleLayout = new QHBoxLayout();
    QLabel* iconLabel = new QLabel("⏰ 任务提醒", this);
    QFont titleFont = iconLabel->font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    iconLabel->setFont(titleFont);
    iconLabel->setStyleSheet("color: #F5A623;");//橙色
    titleLayout->addWidget(iconLabel);
    titleLayout->addStretch();//居中
    mainLayout->addLayout(titleLayout);

    // 分隔线 
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);//水平
    line->setStyleSheet("color: #E8ECF1;");
    mainLayout->addWidget(line);

    // 任务信息 按照优先级分别为红色橙色灰色
    QString priorityText;
    QString priorityColor;
    switch (m_task.priority) {
        case 1: priorityText = "高";  priorityColor = "#E74C3C"; break;
        case 2: priorityText = "中";  priorityColor = "#F5A623"; break;
        default:priorityText = "低";  priorityColor = "#95A5A6"; break;
    }

    QLabel* nameLabel = new QLabel(
        QString("任务：%1").arg(QString::fromStdString(m_task.name)), this);//转化为Qstring
    nameLabel->setStyleSheet("font-size: 11pt; color: #333333;");
    mainLayout->addWidget(nameLabel);

    QLabel* timeLabel = new QLabel(
        QString("时间：%1").arg(QString::fromStdString(m_task.startTime)), this);
    timeLabel->setStyleSheet("font-size: 10pt; color: #666666;");
    mainLayout->addWidget(timeLabel);

    QLabel* priorityLabel = new QLabel(
        QString("优先级：<span style='color:%1; font-weight:bold;'>%2</span>")//用html格式化，有颜色且加粗
            .arg(priorityColor, priorityText), this);
    priorityLabel->setTextFormat(Qt::RichText);
    priorityLabel->setStyleSheet("font-size: 10pt; color: #666666;");
    mainLayout->addWidget(priorityLabel);

    mainLayout->addStretch();

    // 按钮区域 
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    btnLayout->addStretch();

    // 「5分钟后再提醒」按钮
    QPushButton* snoozeBtn = new QPushButton("🔄 5分钟后再提醒", this);
    snoozeBtn->setObjectName("snoozeBtn");
    snoozeBtn->setMinimumWidth(150);
    snoozeBtn->setMinimumHeight(34);
    snoozeBtn->setCursor(Qt::PointingHandCursor);
    snoozeBtn->setStyleSheet(
        "QPushButton { background-color: #F0F0F0; color: #333333; border: 1px solid #D0D0D0; "
        "border-radius: 4px; padding: 4px 12px; font-size: 10pt; }"
        "QPushButton:hover { background-color: #E0E0E0; }");//内嵌样式
    connect(snoozeBtn, &QPushButton::clicked, this, &ReminderPopup::onSnoozeClicked);

    // 「我知道了」按钮
    QPushButton* okBtn = new QPushButton("我知道了", this);
    okBtn->setObjectName("primaryBtn");
    okBtn->setMinimumWidth(120);
    okBtn->setMinimumHeight(34);
    okBtn->setCursor(Qt::PointingHandCursor);
    connect(okBtn, &QPushButton::clicked, this, &ReminderPopup::onDismissClicked);

    btnLayout->addWidget(snoozeBtn);
    btnLayout->addWidget(okBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);
}

//槽函数实现
void ReminderPopup::onDismissClicked() {
    m_action = DISMISS;
    accept();
}

void ReminderPopup::onSnoozeClicked() {
    m_action = SNOOZE;
    accept();
}


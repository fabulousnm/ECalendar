//TaskDialog.cpp
#include "gui/TaskDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDateTime>
#include <QFont>
#include <QMessageBox>


TaskDialog::TaskDialog(QWidget* parent)
    : QDialog(parent)
    , m_editMode(false)
    , m_nameEdit(nullptr)
    , m_startTimeEdit(nullptr)
    , m_highRadio(nullptr)
    , m_mediumRadio(nullptr)
    , m_lowRadio(nullptr)
    , m_categoryCombo(nullptr)
    , m_remindTimeEdit(nullptr)
    , m_errorLabel(nullptr)
{
    setupUI();
    setWindowTitle("添加新任务");
}
//编辑模式
TaskDialog::TaskDialog(bool editMode, const Task& task, QWidget* parent)
    : QDialog(parent)
    , m_editMode(editMode)
    , m_editTask(task)
    , m_nameEdit(nullptr)
    , m_startTimeEdit(nullptr)
    , m_highRadio(nullptr)
    , m_mediumRadio(nullptr)
    , m_lowRadio(nullptr)
    , m_categoryCombo(nullptr)
    , m_remindTimeEdit(nullptr)
    , m_errorLabel(nullptr)
{
    setupUI();
    setWindowTitle("编辑任务");

    // 预填已有数据
    m_nameEdit->setText(QString::fromStdString(task.name));

    // 解析开始时间
    QDateTime startDt = QDateTime::fromString(
        QString::fromStdString(task.startTime), "yyyy-MM-dd HH:mm");
    if (startDt.isValid()) {
        m_startTimeEdit->setDateTime(startDt);
    }

    // 设置优先级
    switch (task.priority) {
        case 1: m_highRadio->setChecked(true);   break;
        case 2: m_mediumRadio->setChecked(true); break;
        default: m_lowRadio->setChecked(true);   break;
    }

    // 设置分类
    QString cat = QString::fromStdString(task.category);
    int catIndex = m_categoryCombo->findText(cat);//寻找匹配文本
    if (catIndex >= 0) {
        m_categoryCombo->setCurrentIndex(catIndex);
    }

    // 设置提醒时间
    if (!task.remindTime.empty()) {
        QDateTime remindDt = QDateTime::fromString(
            QString::fromStdString(task.remindTime), "yyyy-MM-dd HH:mm");
        if (remindDt.isValid()) {
            m_remindTimeEdit->setDateTime(remindDt);
        }
    }
}

//初始化
void TaskDialog::setupUI() {
    setFixedSize(420, 420);
    setObjectName("taskDialog");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(12);

    //  任务名称
    QLabel* nameLabel = new QLabel("任务名称", this);
    nameLabel->setStyleSheet("font-size: 10pt; color: #333333;");
    mainLayout->addWidget(nameLabel);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("请输入任务名称");
    m_nameEdit->setMinimumHeight(32);
    mainLayout->addWidget(m_nameEdit);

    //  开始时间
    QLabel* timeLabel = new QLabel("开始时间", this);
    timeLabel->setStyleSheet("font-size: 10pt; color: #333333;");
    mainLayout->addWidget(timeLabel);

    m_startTimeEdit = new QDateTimeEdit(this);
    m_startTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_startTimeEdit->setCalendarPopup(true);//弹出日历
    m_startTimeEdit->setDateTime(QDateTime::currentDateTime());//默认当下时间
    m_startTimeEdit->setMinimumHeight(32);
    mainLayout->addWidget(m_startTimeEdit);

    // 优先级
    QLabel* priorityLabel = new QLabel("优先级", this);
    priorityLabel->setStyleSheet("font-size: 10pt; color: #333333;");
    mainLayout->addWidget(priorityLabel);

    QHBoxLayout* priorityLayout = new QHBoxLayout();
    priorityLayout->setSpacing(16);
    m_highRadio = new QRadioButton("高", this);
    m_highRadio->setStyleSheet("color: #E74C3C; font-weight: bold;");
    m_mediumRadio = new QRadioButton("中", this);
    m_mediumRadio->setStyleSheet("color: #F5A623; font-weight: bold;");
    m_mediumRadio->setChecked(true); // 默认选中"中"
    m_lowRadio = new QRadioButton("低", this);
    m_lowRadio->setStyleSheet("color: #95A5A6; font-weight: bold;");

    priorityLayout->addWidget(m_highRadio);
    priorityLayout->addWidget(m_mediumRadio);
    priorityLayout->addWidget(m_lowRadio);
    priorityLayout->addStretch();
    mainLayout->addLayout(priorityLayout);

    // ---- 分类 ----
    QLabel* categoryLabel = new QLabel("分类", this);
    categoryLabel->setStyleSheet("font-size: 10pt; color: #333333;");
    mainLayout->addWidget(categoryLabel);

    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->addItems({"学习", "娱乐", "生活"});
    m_categoryCombo->setMinimumHeight(32);
    mainLayout->addWidget(m_categoryCombo);

    // ---- 提醒时间 ----
    QLabel* remindLabel = new QLabel("提醒时间", this);
    remindLabel->setStyleSheet("font-size: 10pt; color: #333333;");
    mainLayout->addWidget(remindLabel);

    m_remindTimeEdit = new QDateTimeEdit(this);
    m_remindTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_remindTimeEdit->setCalendarPopup(true);
    // 默认提醒时间为开始时间前 15 分钟
    m_remindTimeEdit->setDateTime(
        m_startTimeEdit->dateTime().addSecs(-900));
    m_remindTimeEdit->setMinimumHeight(32);
    mainLayout->addWidget(m_remindTimeEdit);

    //  错误提示
    m_errorLabel = new QLabel("", this);
    m_errorLabel->setObjectName("errorLabel");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();
    mainLayout->addWidget(m_errorLabel);

    mainLayout->addStretch();

    //  按钮区域
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton* cancelBtn = new QPushButton("取消", this);
    cancelBtn->setObjectName("textBtn");
    cancelBtn->setMinimumHeight(32);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    QPushButton* confirmBtn = new QPushButton("确认", this);
    confirmBtn->setObjectName("primaryBtn");
    confirmBtn->setDefault(true);
    confirmBtn->setMinimumHeight(32);
    confirmBtn->setCursor(Qt::PointingHandCursor);
    connect(confirmBtn, &QPushButton::clicked, this, &TaskDialog::onConfirmClicked);

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(confirmBtn);
    mainLayout->addLayout(btnLayout);
}

// 确认按钮点击 - 校验输入

void TaskDialog::onConfirmClicked() {
    // 校验：任务名称不能为空
    if (m_nameEdit->text().trimmed().isEmpty()) {
        m_errorLabel->setText("⚠ 请输入任务名称");
        m_errorLabel->show();
        m_nameEdit->setFocus();
        return;
    }

    // 校验：开始时间不能为空（QDateTimeEdit 总是有值，无需额外检查）
    // 所有校验通过
    m_errorLabel->hide();
    accept();
}

// 获取对话框结果
Task TaskDialog::getTask() const {
    Task task;

    // 如果是编辑模式，保留原有 ID
    if (m_editMode) {
        task.id = m_editTask.id;
    }

    task.name = m_nameEdit->text().trimmed().toStdString();//去掉空格之后转成字符串

    // 开始时间
    task.startTime = m_startTimeEdit->dateTime().toString("yyyy-MM-dd HH:mm").toStdString();

    // 优先级,Radio是互斥控件
    if (m_highRadio->isChecked()) {
        task.priority = 1;
    } else if (m_mediumRadio->isChecked()) {
        task.priority = 2;
    } else {
        task.priority = 3;
    }

    // 分类
    task.category = m_categoryCombo->currentText().toStdString();

    // 提醒时间
    task.remindTime = m_remindTimeEdit->dateTime().toString("yyyy-MM-dd HH:mm").toStdString();

    return task;
}


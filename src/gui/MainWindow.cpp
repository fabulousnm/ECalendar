//MainWindow.cpp
#include "gui/MainWindow.h"
#include "gui/TaskDialog.h"
#include "gui/ReminderPopup.h"
#include "core/TaskManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QFont>
#include <QApplication>
#include <QDesktopWidget>
#include <QThread>
#include <thread>
#include <future>  //异步任务实现库
#include <unistd.h>
#include <atomic>
#include <vector>
#include <fstream>
#include <algorithm>
//录音相关功能库
#include "audio/Reminder.h"
#include "audio/AudioRecoder.h"
#include "audio/SpeechRecognizer.h"
#include "util/NLProcess.h"

MainWindow::MainWindow(TaskManager* manager, const QString& username, QWidget* parent)
    : QMainWindow(parent)
    , m_manager(manager)
    , m_username(username)
    , m_monthView(false)
    , m_calendarIcon(nullptr)
    , m_dateEdit(nullptr)
    , m_prevDayBtn(nullptr)
    , m_nextDayBtn(nullptr)
    , m_addTaskBtn(nullptr)
    , m_refreshBtn(nullptr)
    , m_dayViewBtn(nullptr)
    , m_monthViewBtn(nullptr)
    , m_table(nullptr)
    , m_userLabel(nullptr)
    , m_taskCountLabel(nullptr)
    , m_nextRemindLabel(nullptr)
    , m_reminderTimer(nullptr)
    , m_recordState(Idle)
    , m_tempWavPath("/tmp/ecalender_speech.wav")
    , m_hasArecord(false)
    , m_stopFeeding(false)
{
    // 初始化数据文件路径
    initDataPaths();

    // 按用户加载，文件名tasks_<username>.json
    m_manager->loadFromUserFile(
        m_dataDir.toStdString(),
        m_userFilePath.toStdString(),
        m_username.toStdString()
    );


    // 初始化提醒
    Reminder::init();

    setupUI();

    // 创建提醒定时器（每10秒检查一次）
    m_reminderTimer = new QTimer(this);
    connect(m_reminderTimer, &QTimer::timeout, this, &MainWindow::onCheckReminders);
    m_reminderTimer->start(10000);

    // 初始加载表格
    refreshTable();
    updateStatusBar();
    // 后台静默预加载语音模型（异步，不阻塞 UI，不输出日志）
    // 首次录音时模型已加载完成，用户零等待
    std::thread([]() {
        int saved_stderr = dup(STDERR_FILENO);
        FILE* null = freopen("/dev/null", "w", stderr);
        SpeechRecognizer preloader;
        preloader.loadModel("model/vosk-model-cn-0.22");
        fflush(stderr);
        dup2(saved_stderr, STDERR_FILENO);
        ::close(saved_stderr);
    }).detach();
}
// 析构函数

MainWindow::~MainWindow() {
    saveData();
}

// 初始化界面

void MainWindow::setupUI() {
    setWindowTitle(QString("Ecalender — %1").arg(m_username));
    resize(900, 600);
    setMinimumSize(700, 450);
    setObjectName("mainWindow");

    // 中央部件
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 顶部工具栏
    // 作为独立的 widget 嵌入
    QWidget* toolbarWidget = new QWidget(this);
    toolbarWidget->setObjectName("toolbarWidget");
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->setContentsMargins(16, 8, 16, 8);
    toolbarLayout->setSpacing(8);


    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");
    m_dateEdit->setMinimumHeight(32);
    m_dateEdit->setMinimumWidth(140);

    // 前一天按钮
    m_prevDayBtn = new QPushButton("◀", this);
    m_prevDayBtn->setFixedSize(32, 32);
    m_prevDayBtn->setObjectName("iconBtn");
    m_prevDayBtn->setToolTip("前一天");
    m_prevDayBtn->setCursor(Qt::PointingHandCursor);

    // 后一天按钮
    m_nextDayBtn = new QPushButton("▶", this);
    m_nextDayBtn->setFixedSize(32, 32);
    m_nextDayBtn->setObjectName("iconBtn");
    m_nextDayBtn->setToolTip("后一天");
    m_nextDayBtn->setCursor(Qt::PointingHandCursor);

    // [+新任务]
    m_addTaskBtn = new QPushButton("+ 新任务", this);
    m_addTaskBtn->setObjectName("primaryBtn");
    m_addTaskBtn->setMinimumHeight(32);
    m_addTaskBtn->setCursor(Qt::PointingHandCursor);
    // 🤖 AI 自然语言输入框
    m_nlInput = new QLineEdit(this);
    m_nlInput->setPlaceholderText("e.g. 明天下午三点去图书馆学习");
    m_nlInput->setMinimumHeight(32);
    m_nlInput->setMinimumWidth(260);
    m_nlInput->setClearButtonEnabled(true);

    // 🤖 AI 添加按钮
    m_aiAddBtn = new QPushButton("🤖 AI 添加", this);
    m_aiAddBtn->setObjectName("primaryBtn");
    m_aiAddBtn->setMinimumHeight(32);
    m_aiAddBtn->setCursor(Qt::PointingHandCursor);
    m_aiAddBtn->setToolTip("用自然语言快速添加任务");

    // 🎤 录音按钮
    m_recordBtn = new QPushButton("🎤 录音", this);
    m_recordBtn->setObjectName("recordBtn");
    m_recordBtn->setMinimumHeight(32);
    m_recordBtn->setCursor(Qt::PointingHandCursor);
    m_recordBtn->setToolTip("点击开始/停止录音");
    // 刷新按钮
    m_refreshBtn = new QPushButton("⟳", this);
    m_refreshBtn->setFixedSize(32, 32);
    m_refreshBtn->setObjectName("iconBtn");
    m_refreshBtn->setToolTip("刷新");
    m_refreshBtn->setCursor(Qt::PointingHandCursor);

    // 视图切换按钮
    m_dayViewBtn = new QPushButton("天视图", this);
    m_dayViewBtn->setObjectName("toggleBtn");
    m_dayViewBtn->setCheckable(true);//可切换状态
    m_dayViewBtn->setChecked(true);
    m_dayViewBtn->setCursor(Qt::PointingHandCursor);

    m_monthViewBtn = new QPushButton("月视图", this);
    m_monthViewBtn->setObjectName("toggleBtn");
    m_monthViewBtn->setCheckable(true);
    m_monthViewBtn->setCursor(Qt::PointingHandCursor);

    // 组装工具栏
    toolbarLayout->addWidget(m_calendarIcon);
    toolbarLayout->addWidget(m_dateEdit);
    toolbarLayout->addWidget(m_prevDayBtn);
    toolbarLayout->addWidget(m_nextDayBtn);
    toolbarLayout->addSpacing(8);
    toolbarLayout->addWidget(m_addTaskBtn);
    toolbarLayout->addWidget(m_nlInput);
    toolbarLayout->addWidget(m_aiAddBtn);
    toolbarLayout->addWidget(m_recordBtn);
    toolbarLayout->addWidget(m_refreshBtn);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_dayViewBtn);
    toolbarLayout->addWidget(m_monthViewBtn);

    mainLayout->addWidget(toolbarWidget);

    // ---- 中央表格 ----
    setupTable();
    mainLayout->addWidget(m_table, 1); // stretch=1 占据剩余空间

    // 底部状态栏
    setupStatusBar();

    // ---- 信号连接 ----
    connect(m_dateEdit, &QDateEdit::dateChanged, this, &MainWindow::onDateChanged);
    connect(m_prevDayBtn, &QPushButton::clicked, this, &MainWindow::onPrevDayClicked);
    connect(m_nextDayBtn, &QPushButton::clicked, this, &MainWindow::onNextDayClicked);
    connect(m_addTaskBtn, &QPushButton::clicked, this, &MainWindow::onAddTaskClicked);
    connect(m_aiAddBtn, &QPushButton::clicked, this, &MainWindow::onAiAddClicked);
    connect(m_nlInput, &QLineEdit::returnPressed, this, &MainWindow::onAiAddClicked);
    connect(m_recordBtn, &QPushButton::clicked, this, &MainWindow::onRecordClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    connect(m_dayViewBtn, &QPushButton::clicked, this, &MainWindow::onDayViewClicked);
    connect(m_monthViewBtn, &QPushButton::clicked, this, &MainWindow::onMonthViewClicked);
    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &MainWindow::onTableDoubleClicked);
}

// 创建中央表格
void MainWindow::setupTable() {
    m_table = new QTableWidget(0, 7, this);
    m_table->setObjectName("taskTable");

    // 设置表头
    QStringList headers;
    headers << "ID" << "任务名称" << "开始时间" << "优先级"
            << "分类" << "提醒时间" << "操作";
    m_table->setHorizontalHeaderLabels(headers);

    // 表格属性
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);//选择整行
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);//隔行变色
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止直接编辑
    m_table->verticalHeader()->setVisible(false);//隐藏行号
    m_table->setMouseTracking(true);//鼠标跟踪

    // 设置列宽
    QHeaderView* header = m_table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Fixed);       // ID
    header->setSectionResizeMode(1, QHeaderView::Stretch);     // 任务名称（自适应）
    header->setSectionResizeMode(2, QHeaderView::Fixed);       // 开始时间
    header->setSectionResizeMode(3, QHeaderView::Fixed);       // 优先级
    header->setSectionResizeMode(4, QHeaderView::Fixed);       // 分类
    header->setSectionResizeMode(5, QHeaderView::Fixed);       // 提醒时间
    header->setSectionResizeMode(6, QHeaderView::Fixed);       // 操作

    m_table->setColumnWidth(0, 50);
    m_table->setColumnWidth(2, 150);
    m_table->setColumnWidth(3, 80);
    m_table->setColumnWidth(4, 80);
    m_table->setColumnWidth(5, 150);
    m_table->setColumnWidth(6, 130);

    // 默认按开始时间排序（第2列）
    m_table->sortByColumn(2, Qt::AscendingOrder);
}

// 创建状态栏
void MainWindow::setupStatusBar() {
    QStatusBar* status = statusBar();
    status->setObjectName("mainStatusBar");

    m_userLabel = new QLabel(this);
    m_userLabel->setObjectName("statusLabel");
    status->addWidget(m_userLabel);

    m_taskCountLabel = new QLabel(this);
    m_taskCountLabel->setObjectName("statusLabel");
    status->addWidget(m_taskCountLabel);
     //永久
    m_nextRemindLabel = new QLabel(this);
    m_nextRemindLabel->setObjectName("statusLabel");
    status->addPermanentWidget(m_nextRemindLabel);
}

// 初始化数据文件路径
   
void MainWindow::initDataPaths() {
    // 使用 ~/.ecalender/ 作为数据目录
    QString homeDir = QString::fromLocal8Bit(qgetenv("HOME"));
    if (homeDir.isEmpty()) homeDir = QDir::homePath();
    m_dataDir = homeDir + "/.ecalender";
    QString dataDir = m_dataDir;

    // 确保数据目录存在
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    m_userFilePath = dataDir + "/users.json";
}

// 保存全部数据

void MainWindow::saveData() {
    m_manager->saveToUserFile(
        m_dataDir.toStdString(),
        m_userFilePath.toStdString()
    );
}

// 获取当前视图的日期前缀,月还是天用于搜索任务
std::string MainWindow::getCurrentDatePrefix() const {
    QDate date = m_dateEdit->date();
    if (m_monthView) {
        // 月视图：匹配 "YYYY-MM"
        return date.toString("yyyy-MM").toStdString();
    } else {
        // 天视图：匹配 "YYYY-MM-DD"
        return date.toString("yyyy-MM-dd").toStdString();
    }
}

// 刷新表格
void MainWindow::refreshTable() {
    // 获取当前视图的任务数据
    std::string prefix = getCurrentDatePrefix();
    std::vector<Task> tasks;
    if (m_monthView) {
        tasks = m_manager->getTasksByMonth(prefix);
    } else {
        tasks = m_manager->getTasksByDate(prefix);
    }

    // 清空表格
    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(tasks.size()));

    for (int i = 0; i < static_cast<int>(tasks.size()); ++i) {
        const Task& task = tasks[i];

        // ID
        QTableWidgetItem* idItem = new QTableWidgetItem(QString::number(task.id));
        idItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 0, idItem);

        // 任务名称
        QTableWidgetItem* nameItem = new QTableWidgetItem(
            QString::fromStdString(task.name));
        m_table->setItem(i, 1, nameItem);

        // 开始时间
        QTableWidgetItem* timeItem = new QTableWidgetItem(
            QString::fromStdString(task.startTime));
        timeItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 2, timeItem);

        // 优先级（带颜色标签）
        QString priorityText;
        QString bgColor, fgColor;
        switch (task.priority) {
            case 1:
                priorityText = "高";
                bgColor = "#FFEBEE";
                fgColor = "#E74C3C";
                break;
            case 2:
                priorityText = "中";
                bgColor = "#FFF8E1";
                fgColor = "#F5A623";
                break;
            default:
                priorityText = "低";
                bgColor = "#F5F5F5";
                fgColor = "#95A5A6";
                break;
        }
        QTableWidgetItem* priorityItem = new QTableWidgetItem(priorityText);
        priorityItem->setTextAlignment(Qt::AlignCenter);
        priorityItem->setBackground(QColor(bgColor));
        priorityItem->setForeground(QColor(fgColor));
        QFont priorityFont = priorityItem->font();
        priorityFont.setBold(true);
        priorityItem->setFont(priorityFont);
        m_table->setItem(i, 3, priorityItem);

        // 分类
        QTableWidgetItem* categoryItem = new QTableWidgetItem(
            QString::fromStdString(task.category));
        categoryItem->setTextAlignment(Qt::AlignCenter);
        // 分类颜色
        QString catColor;
        if (task.category == "学习") catColor = "#4A90D9";
        else if (task.category == "娱乐") catColor = "#9B59B6";
        else if (task.category == "生活") catColor = "#52C41A";
        else catColor = "#888888";
        categoryItem->setForeground(QColor(catColor));
        m_table->setItem(i, 4, categoryItem);

        // 提醒时间
        QString remindStr = task.remindTime.empty()
            ? "—" : QString::fromStdString(task.remindTime);
        QTableWidgetItem* remindItem = new QTableWidgetItem(remindStr);
        remindItem->setTextAlignment(Qt::AlignCenter);
        if (task.remindTime.empty()) {
            remindItem->setForeground(QColor("#C0C4CC"));
        }
        m_table->setItem(i, 5, remindItem);

        // 操作列：编辑 + 删除按钮
        QWidget* actionWidget = new QWidget(this);
        QHBoxLayout* actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(4, 2, 4, 2);
        actionLayout->setSpacing(4);

        QPushButton* editBtn = new QPushButton("编辑", actionWidget);
        editBtn->setObjectName("tableEditBtn");
        editBtn->setFixedSize(50, 24);
        editBtn->setCursor(Qt::PointingHandCursor);
        int taskId = task.id;
        connect(editBtn, &QPushButton::clicked, this, [this, taskId]() {
            onEditTask(taskId);
        });

        QPushButton* deleteBtn = new QPushButton("删除", actionWidget);
        deleteBtn->setObjectName("tableDeleteBtn");
        deleteBtn->setFixedSize(50, 24);
        deleteBtn->setCursor(Qt::PointingHandCursor);
        connect(deleteBtn, &QPushButton::clicked, this, [this, taskId]() {
            onDeleteTask(taskId);
        });

        actionLayout->addWidget(editBtn);
        actionLayout->addWidget(deleteBtn);
        actionLayout->addStretch();

        m_table->setCellWidget(i, 6, actionWidget);//设置自定义控件
    }

    // 按开始时间列排序
    m_table->sortByColumn(2, Qt::AscendingOrder);

    updateStatusBar();
}

// 更新状态栏
void MainWindow::updateStatusBar() {
    int totalTasks = static_cast<int>(m_manager->getTasks().size());
    m_userLabel->setText(QString("当前用户: %1").arg(m_username));
    m_taskCountLabel->setText(QString(" | 任务总数: %1").arg(totalTasks));

    // 查找下次提醒时间
    std::string nextRemind;
    for (const auto& t : m_manager->getTasks()) {
        if (!t.remindTime.empty()) {
            if (nextRemind.empty() || t.remindTime < nextRemind) {
                nextRemind = t.remindTime;
            }
        }
    }
    if (!nextRemind.empty()) {
        m_nextRemindLabel->setText(
            QString("下次提醒: %1").arg(QString::fromStdString(nextRemind)));
    } else {
        m_nextRemindLabel->setText("下次提醒: 无");
    }
}

// ◀ 前一天按钮点击
void MainWindow::onPrevDayClicked() {
    QDate newDate = m_dateEdit->date().addDays(-1);
    m_dateEdit->setDate(newDate);
}

// ▶ 后一天按钮点击
void MainWindow::onNextDayClicked() {
    QDate newDate = m_dateEdit->date().addDays(1);
    m_dateEdit->setDate(newDate);
}
// [+新任务] 按钮点击
void MainWindow::onAddTaskClicked() {
    TaskDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Task task = dialog.getTask();
        int result = m_manager->addTask(task);
        if (result > 0) {
            saveData();
            refreshTable();
        } else if (result == -2) {
            QMessageBox::warning(this, "添加失败",
                "❌ 提醒时间不能晚于开始时间（包括年月日时分），请调整后重试。");
        } else {
            QMessageBox::warning(this, "添加失败",
                "存在同名且同开始时间的任务，请修改后重试。");
        }
    }
}

void MainWindow::onAiAddClicked() {
    QString input = m_nlInput->text().trimmed();
    if (input.isEmpty()) {
        m_nlInput->setFocus();
        return;
    }

    // 显示解析中状态
    if (m_nextRemindLabel)
        m_nextRemindLabel->setText("🤖 AI 解析中...");

    // 使用 NLProcessor 解析（可能在后台线程执行，但这里简单起见直接调）
    // 注：parse() 内部会先尝试 DeepSeek API（网络调用可能慢），然后回退到正

    std::string text = input.toStdString();
    ParsedTask parsed = NLProcessor::parse(text);

    if (!parsed.valid) {
        if (m_nextRemindLabel)
            m_nextRemindLabel->setText(
                QString::fromStdString("❌ 无法解析: " + text));
        return;
    }

    // 显示解析成功
    if (m_nextRemindLabel)
        m_nextRemindLabel->setText(
            QString("✅ 解析成功: %1").arg(QString::fromStdString(parsed.name)));

    // 清空输入框
    m_nlInput->clear();

    // 打开预填 TaskDialog（包含提醒时间）
    confirmAndAddTask(
        QString::fromStdString(parsed.name),
        QString::fromStdString(parsed.startTime),
        parsed.priority,
        QString::fromStdString(parsed.category),
        QString::fromStdString(parsed.remindTime)
    );
}
// 刷新按钮点击
void MainWindow::onRefreshClicked() {
    // 重新从文件加载数据（按用户隔离）
    m_manager->loadFromUserFile(
        m_dataDir.toStdString(),
        m_userFilePath.toStdString(),
        m_username.toStdString()
    );
    refreshTable();
}

// 日期选择变化
void MainWindow::onDateChanged(const QDate& date) {
    Q_UNUSED(date);
    refreshTable();
}

// 切换天视图
void MainWindow::onDayViewClicked() {
    if (m_monthView) {
        m_monthView = false;
        m_dayViewBtn->setChecked(true);
        m_monthViewBtn->setChecked(false);
        refreshTable();
    }
}

// 切换月视图

void MainWindow::onMonthViewClicked() {
    if (!m_monthView) {
        m_monthView = true;
        m_dayViewBtn->setChecked(false);
        m_monthViewBtn->setChecked(true);
        refreshTable();
    }
}

// 编辑任务

void MainWindow::onEditTask(int taskId) {
    // 查找要编辑的任务
    const auto& allTasks = m_manager->getTasks();
    Task taskToEdit;
    bool found = false;
    for (const auto& t : allTasks) {
        if (t.id == taskId) {
            taskToEdit = t;
            found = true;
            break;
        }
    }
    if (!found) {
        QMessageBox::information(this, "提示", "任务已被删除。");
        refreshTable();
        return;
    }

    TaskDialog dialog(true, taskToEdit, this);
    if (dialog.exec() == QDialog::Accepted) {
        Task updatedTask = dialog.getTask();

        // 删除原任务，添加新任务
        m_manager->deleteTask(taskId);
        int newId = m_manager->addTask(updatedTask);
        if (newId > 0) {
            saveData();
            refreshTable();
        } else if (newId == -2) {
            QMessageBox::warning(this, "编辑失败",
                "❌ 提醒时间不能晚于开始时间（包括年月日时分），请调整后重试。");
        } else {
            QMessageBox::warning(this, "编辑失败",
                "存在同名且同开始时间的任务，请修改后重试。");
        }
    }
}

// 删除任务
void MainWindow::onDeleteTask(int taskId) {
    // 查找任务名
    std::string taskName;
    for (const auto& t : m_manager->getTasks()) {
        if (t.id == taskId) {
            taskName = t.name;
            break;
        }
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除",
        QString("确定要删除任务「%1」吗？").arg(QString::fromStdString(taskName)),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (m_manager->deleteTask(taskId)) {
            saveData();
            refreshTable();
        }
    }
}

// 表格双击 - 编辑任务
void MainWindow::onTableDoubleClicked(int row, int column) {
    Q_UNUSED(column);
    QTableWidgetItem* idItem = m_table->item(row, 0);
    if (idItem) {
        int taskId = idItem->text().toInt();
        onEditTask(taskId);
    }
}

// 定时检查提醒
void MainWindow::onCheckReminders() {
    auto reminders = m_manager->getUpcomingReminders();
    for (const auto& task : reminders) {
        // 防重复：检查是否已触发过
        QString key = QString::fromStdString(
            task.name + task.remindTime);
        if (m_recentlyTriggered.contains(key)) {
            continue;
        }

        // 添加到已触发列表
        m_recentlyTriggered.append(key);
        while (m_recentlyTriggered.size() > 100) {
            m_recentlyTriggered.removeFirst();
        }
         // 在独立线程中播放提醒音效，弹窗期间持续播放
        std::thread([]() {
            Reminder::playReminderSound();
        }).detach();

        // 显示提醒弹窗（模态，会阻塞直到用户选择）
        ReminderPopup popup(task, this);
        popup.exec();

        // 用户选择后关闭音频
        Reminder::killAudio();
        // 处理用户选择
        if (popup.action() == ReminderPopup::SNOOZE) {
            // 计算 5 分钟后的时间
            QDateTime now = QDateTime::currentDateTime();
            QDateTime fiveMinLater = now.addSecs(300);
            QString fiveMinStr = fiveMinLater.toString("yyyy-MM-dd HH:mm");

            // 检查是否超过开始时间
            QString startStr = QString::fromStdString(task.startTime);
            if (fiveMinStr >= startStr) {
                QMessageBox::information(this, "无法延后",
                    "⚠ 时间还有五分钟就开始了！不能再延后。");
            } else {
                // 更新提醒时间
                m_manager->updateTaskRemindTime(
                    task.id, fiveMinStr.toStdString());
                saveData();
                refreshTable();
                QMessageBox::information(this, "已延后",
                    QString("✅ 已延后提醒至 %1").arg(fiveMinStr));
            }
        }
   }
}

//录音相关函数
void MainWindow::onRecordClicked() {
    switch (m_recordState) {
        case Idle:
            startRecording();
            break;
        case Loading:
            // 加载中，忽略后续点击
            break;
        case Recording:
            stopRecording();
            break;
    }
}

/**
 * startRecording - 开始录音
 *
 * 1. 按钮变 ⏳ 准备中，不可点击
 * 2. 后台加载模型
 * 3. 加载完成后自动切换到录音状态
 */
void MainWindow::startRecording() {
    m_recordState = Loading;
    m_recordBtn->setText("⏳ 准备中");
    m_recordBtn->setEnabled(false);
    m_recordBtn->setStyleSheet(
        "background-color: #F5A623; color: white; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 4px 12px;"
    );

    if (m_nextRemindLabel)
        m_nextRemindLabel->setText("🎧 模型加载中...");

    // 检查录音工具
    checkRecordTool();

    // 后台加载模型
    loadModelInBackground();
}

/**
 * checkRecordTool - 检查录音工具
 */
bool MainWindow::checkRecordTool() {
    m_hasArecord = (std::system("which arecord 2>/dev/null > /dev/null") == 0);
    bool hasParec = (std::system("which parec 2>/dev/null > /dev/null") == 0);
    if (!m_hasArecord && !hasParec) {
        QMetaObject::invokeMethod(this, [this]() {
            if (m_nextRemindLabel)
                m_nextRemindLabel->setText("❌ 未找到录音工具");
            m_recordState = Idle;
            m_recordBtn->setText("🎤 录音");
            m_recordBtn->setEnabled(true);
            m_recordBtn->setStyleSheet("");
        }, Qt::QueuedConnection);
        return false;
    }
    return true;
}

/*loadModelInBackground - 后台加载模型
 在独立线程加载 Vosk 模型（共享模型，首次 2-3 秒）。
 加载完成后回到主线程调用 onModelLoaded。
 */
void MainWindow::loadModelInBackground() {
    std::thread([this]() {
        SpeechRecognizer* recognizer = new SpeechRecognizer();
        bool success = recognizer->loadModel("model/vosk-model-cn-0.22");

        QMetaObject::invokeMethod(this, [this, recognizer, success]() {
            if (success) {
                // 存储 recognizer 指针，后续喂入线程使用
                // 使用静态共享模型，所以每个 SpeechRecognizer 实例共享同一个模型
                onModelLoaded(success);
            } else {
                if (m_nextRemindLabel)
                    m_nextRemindLabel->setText("❌ 语音模型加载失败");
                m_recordState = Idle;
                m_recordBtn->setText("🎤 录音");
                m_recordBtn->setEnabled(true);
                m_recordBtn->setStyleSheet("");
            }
            delete recognizer;
        }, Qt::QueuedConnection);
    }).detach();
}

/**
 * onModelLoaded - 模型加载完成回调（主线程）
 *
 * 按钮切换为 🔴 录音中，启动录音进程和喂入线程。
 */
void MainWindow::onModelLoaded(bool success) {
    if (!success) return;

    m_recordState = Recording;
    m_stopFeeding = false;
    m_lastPartialText.clear();

    m_recordBtn->setText("🔴 录音中");
    m_recordBtn->setEnabled(true);
    m_recordBtn->setStyleSheet(
        "background-color: #E74C3C; color: white; font-weight: bold;"
        "border: none; border-radius: 4px; padding: 4px 12px;"
    );

    // 清空输入框
    m_nlInput->clear();

    if (m_nextRemindLabel)
        m_nextRemindLabel->setText("🎤 录音中... 点击 🔴 停止");

    // 启动录音
    doStartRecording();
}

/**
 * doStartRecording - 启动录音进程
 *
 * 后台启动 arecord 录制音频到临时文件。
 */
void MainWindow::doStartRecording() {
    m_tempWavPath = QDir::tempPath() + "/ecalender_speech.wav";

    if (m_hasArecord) {
        std::system(("arecord -r 16000 -f S16_LE -c 1 -d 60 \""
            + m_tempWavPath.toStdString() + "\" 2>/dev/null &").c_str());
    } else {
        std::string rawPath = m_tempWavPath.toStdString() + ".raw";
        std::system(("parec --channels=1 --rate=16000 --format=s16le --raw \""
            + rawPath + "\" 2>/dev/null &").c_str());
    }

    // 启动喂入线程
    startFeedThread();
}

/**
 * startFeedThread - 启动音频喂入线程
 *
 * 每 100ms 读取录音文件增量数据，喂入 Vosk，获取实时识别结果。
 * 通过 QMetaObject::invokeMethod 回到主线程更新 m_nlInput。
 */
void MainWindow::startFeedThread() {
    m_feedThread = std::thread([this]() {
        std::streampos lastPos = 0;
        std::string wavPath = m_tempWavPath.toStdString();
        std::string readPath = m_hasArecord ? wavPath : (wavPath + ".raw");

        // 创建本线程专用的 SpeechRecognizer（复用共享模型）
        SpeechRecognizer localRecognizer;
        localRecognizer.loadModel("model/vosk-model-cn-0.22");
        localRecognizer.resetRecognizer();

        while (!m_stopFeeding) {
            std::ifstream audioFile(readPath, std::ios::binary);
            if (audioFile.is_open()) {
                audioFile.seekg(0, std::ios::end);
                std::streampos curPos = audioFile.tellg();

                if (curPos > lastPos) {
                    std::streampos readSize = curPos - lastPos;
                    audioFile.seekg(lastPos);

                    std::vector<char> newData((size_t)readSize);
                    audioFile.read(newData.data(), readSize);

                    // 喂入 Vosk
                    int offset = 0;
                    while (offset < (int)newData.size()) {
                        int chunk = std::min(8000, (int)newData.size() - offset);
                        localRecognizer.acceptWaveform(
                            newData.data() + offset, chunk);
                        offset += chunk;
                    }

                    lastPos = curPos;

                    // 获取部分结果
                    std::string partialJson = localRecognizer.getPartialResult();
                    std::string partialText =
                        localRecognizer.extractPartialText(partialJson);

                    if (!partialText.empty() && partialText != m_lastPartialText) {
                        m_lastPartialText = partialText;
                        QString qtText = QString::fromStdString(partialText);

                        // 回到主线程更新 UI
                        QMetaObject::invokeMethod(this, [this, qtText]() {
                            // 只在录音状态下更新
                            if (m_recordState == Recording) {
                                m_nlInput->setText(qtText);
                            }
                        }, Qt::QueuedConnection);
                    }
                }
                audioFile.close();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 获取最终结果
        std::string finalJson = localRecognizer.getFinalResult();
        std::string finalText = localRecognizer.getText(finalJson);

        // 清洗空格
        std::string cleaned;
        for (size_t i = 0; i < finalText.size(); ) {
            unsigned char c = static_cast<unsigned char>(finalText[i]);
            if (c == ' ') { i++; continue; }
            cleaned += finalText[i];
            i++;
        }
        if (cleaned.empty()) cleaned = finalText;

        // 回到主线程设置最终文本
        QString qtFinalText = QString::fromStdString(cleaned);
        QMetaObject::invokeMethod(this, [this, qtFinalText]() {
            onRecordingFinished(qtFinalText);
        }, Qt::QueuedConnection);
    });
}

/**
 * stopRecording - 停止录音
 *
 * 点击 🔴 录音中按钮时调用：
 * 1. 停止录音进程
 * 2. 停止喂入线程
 * 3. 最终文本由 feedThread 自动回调 onRecordingFinished
 */
void MainWindow::stopRecording() {
    // 停止录音进程
    std::system("killall arecord 2>/dev/null");
    std::system("killall parec 2>/dev/null");
    std::system("killall rec 2>/dev/null");

    if (m_nextRemindLabel)
        m_nextRemindLabel->setText("🔄 识别最终结果...");

    // 停止喂入线程（会触发 finalResult + onRecordingFinished）
    stopFeedThread();
}

/**
 * stopFeedThread - 停止喂入线程
 */
void MainWindow::stopFeedThread() {
    if (m_feedThread.joinable()) {
        m_stopFeeding = true;
        m_feedThread.join();
    }
}

/**
 * updateRecordPartialText - 更新实时识别文本到 AI 输入框（主线程）
 *
 * 由 feedThread 通过 invokeMethod 在主线程调用。
 *
 * @param text 实时识别文本
 */
void MainWindow::updateRecordPartialText(const QString& text) {
    if (m_recordState == Recording) {
        m_nlInput->setText(text);
    }
}

/**
 * onRecordingFinished - 录音结束回调（主线程）
 *
 * 设置最终识别文本到 AI 输入框，恢复按钮待机状态。
 * 用户可在文本框自由编辑后点击 🤖 AI 添加。
 *
 * @param text 最终识别文本
 */
void MainWindow::onRecordingFinished(const QString& text) {
    m_recordState = Idle;
    m_recordBtn->setText("🎤 录音");
    m_recordBtn->setEnabled(true);
    m_recordBtn->setStyleSheet("");

    if (!text.isEmpty()) {
        m_nlInput->setText(text);
        if (m_nextRemindLabel)
            m_nextRemindLabel->setText(
                QString("📝 识别结果: %1").arg(text));
    } else {
        if (m_nextRemindLabel)
            m_nextRemindLabel->setText("❌ 未识别到语音内容");
    }

    // 清理临时文件
    std::string wavStr = m_tempWavPath.toStdString();
    std::remove(wavStr.c_str());
    std::remove((wavStr + ".raw").c_str());
}

/**
 * processRecording - 处理录音结果（兼容旧接口，保留但不再被录音流程使用）
 *
 * 新流程：录音 → 流式识别 → 文本填入 m_nlInput → 用户点击 🤖 AI 添加 → onAiAddClicked()
 *
 * @param wavPath 临时录音 WAV 文件路径
 */
void MainWindow::processRecording(const QString& wavPath) {
    // 新录音流程不再使用此方法
    // 保留以便外部调用（如测试）
    Q_UNUSED(wavPath);
}


//confirmAndAddTask - 打开 TaskDialog 让用户确认并添加任务
void MainWindow::confirmAndAddTask(const QString& taskName, const QString& startTime,
                                    int priority, const QString& category,
                                    const QString& remindTime) {
    // 构建一个临时 Task 对象用于预填 TaskDialog
    Task parsedTask;
    parsedTask.name = taskName.toStdString();
    parsedTask.startTime = startTime.toStdString();
    parsedTask.priority = priority;
    parsedTask.category = category.toStdString();
    if (!remindTime.isEmpty())
        parsedTask.remindTime = remindTime.toStdString();

    // 打开 TaskDialog（编辑模式，预填数据）
    TaskDialog dialog(true, parsedTask, this);
    dialog.setWindowTitle("添加语音识别任务");

    if (dialog.exec() == QDialog::Accepted) {
        Task task = dialog.getTask();
        int result = m_manager->addTask(task);
        if (result > 0) {
            saveData();
            refreshTable();
            if (m_nextRemindLabel) {
                m_nextRemindLabel->setText(QString("✅ 已添加: %1").arg(
                    QString::fromStdString(task.name)));
            }
        } else {
            // 添加失败（同名+同时间）
            if (m_nextRemindLabel) {
                m_nextRemindLabel->setText("❌ 添加失败：任务已存在");
            }
        }
    } else {
        // 用户取消
        if (m_nextRemindLabel) {
            m_nextRemindLabel->setText("已取消");
        }
    }
}




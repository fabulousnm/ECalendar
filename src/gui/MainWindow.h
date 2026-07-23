/*MainWindow.h*/

#ifndef ECALENDER_MAINWINDOW_H
#define ECALENDER_MAINWINDOW_H

#include <QMainWindow>
#include <QDateEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QStatusBar>
#include <QTimer>
#include <QLabel>
#include <QLineEdit>

#include <string>
#include <atomic>
#include <thread>

class TaskManager;
class SpeechRecognizer;

/**
  MainWindow - 主窗口
  日程管理的核心界面。
  通过 TaskManager 操作数据，不直接操作文件。
  每次增删改操作后自动保存。
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    
    explicit MainWindow(TaskManager* manager, const QString& username,
                        QWidget* parent = nullptr);

   
    ~MainWindow() override;

private slots:
    /* ◀ 前一天按钮点击 */
    void onPrevDayClicked();
    /* ▶ 后一天按钮点击 */
    void onNextDayClicked();
    /* [+新任务] 按钮点击 */
    void onAddTaskClicked();
    /* 刷新按钮点击 */
    void onRefreshClicked();
    /* 日期选择变化 */
    void onDateChanged(const QDate& date);
    /* 切换天视图 */
    void onDayViewClicked();
    /* 切换月视图 */
    void onMonthViewClicked();
    /* 编辑任务 */
    void onEditTask(int taskId);
    /* 删除任务 */
    void onDeleteTask(int taskId);
    /* 表格双击 */
    void onTableDoubleClicked(int row, int column);
    /* 定时检查提醒 */
    void onCheckReminders();

    

private:
    TaskManager* m_manager;       // 任务管理器
    QString      m_username;      // 当前登录用户名
    QString      m_taskFilePath;  // 任务数据文件路径
    QString      m_userFilePath;  // 用户数据文件路径
    QString      m_dataDir;       // 数据目录
    bool         m_monthView;     // true=月视图, false=天视图

    // 工具栏控件
    QLabel*      m_calendarIcon;  // 📅 日历图标
    QDateEdit*   m_dateEdit;      // 日期选择器
    QPushButton* m_prevDayBtn;    // 前一天按钮
    QPushButton* m_nextDayBtn;    // 后一天按钮
    QPushButton* m_addTaskBtn;    // 添加任务按钮
    QPushButton* m_refreshBtn;    // 刷新按钮
    QPushButton* m_dayViewBtn;    // 天视图切换按钮
    QPushButton* m_monthViewBtn;  // 月视图切换按钮

    // 表格
    QTableWidget* m_table;        // 任务列表表格

    // 状态栏标签
    QLabel* m_userLabel;          // 当前用户标签
    QLabel* m_taskCountLabel;     // 任务总数标签
    QLabel* m_nextRemindLabel;    // 下次提醒标签

    // 提醒定时器
    QTimer* m_reminderTimer;      // 提醒检查定时器（每10秒）
    QStringList m_recentlyTriggered; // 已触发的提醒列表（防重复）

    /** 初始化用户界面 */
    void setupUI();
    /** 创建顶部工具栏 */
    QWidget* setupToolBar();
    /** 创建中央表格 */
    void setupTable();
    /** 创建状态栏 */
    void setupStatusBar();

    /** 刷新表格数据 */
    void refreshTable();
    /** 更新状态栏信息 */
    void updateStatusBar();
    /** 获取数据文件路径 */
    void initDataPaths();
    /* 保存全部数据 */
    void saveData();
    /* 获取当前显示的日期/月份前缀 */
    std::string getCurrentDatePrefix() const;

};

#endif // ECALENDER_MAINWINDOW_H


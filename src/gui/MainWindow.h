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
    /* 自然语言输入*/
    void onAiAddClicked();
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
  void onRecordClicked();

    //自然语言解析接口
    void processRecording(const QString& wavPath);

    /**
     * confirmAndAddTask - 打开 TaskDialog 确认并添加任务（主线程）
     * @param taskName  解析出的任务名称
     * @param startTime 解析出的开始时间
     * @param priority  解析出的优先级
     * @param category  解析出的分类
     */
    void confirmAndAddTask(const QString& taskName, const QString& startTime,
                           int priority, const QString& category,
                           const QString& remindTime = QString());

    /**
     * updateRecordPartialText - 更新实时识别文本到输入框（主线程）
     * @param text 实时识别文本
     */
    void updateRecordPartialText(const QString& text);

    /**
     * onModelLoaded - 模型加载完成回调（主线程）
     * @param success 是否加载成功
     */
    void onModelLoaded(bool success);

    /**
     * onRecordingFinished - 录音结束回调（主线程）
     * @param text 最终识别文本
     */
    void onRecordingFinished(const QString& text);
    

private:
    TaskManager* m_manager;       // 任务管理器
    QString      m_username;      // 当前登录用户名
    QString      m_taskFilePath;  // 任务数据文件路径
    QString      m_userFilePath;  // 用户数据文件路径
    QString      m_dataDir;       // 数据目录
    bool         m_monthView;     // true=月视图, false=天视图
    // ---- 录音相关状态 ----
    enum RecordState { Idle, Loading, Recording };
    RecordState  m_recordState;      // 录音三态：待机/准备中/录音中
    QString      m_tempWavPath;      // 临时录音文件路径
    bool         m_hasArecord;       // 是否有 arecord 录音工具
    std::atomic<bool> m_stopFeeding; // 停止喂入线程标志
    std::thread  m_feedThread;       // 音频喂入线程
    std::string  m_lastPartialText;  // 上次实时文本（去重）
    // 工具栏控件
    QLabel*      m_calendarIcon;  // 📅 日历图标
    QDateEdit*   m_dateEdit;      // 日期选择器
    QPushButton* m_prevDayBtn;    // 前一天按钮
    QPushButton* m_nextDayBtn;    // 后一天按钮
    QPushButton* m_addTaskBtn;    // 添加任务按钮
    QPushButton* m_refreshBtn;    // 刷新按钮
    QPushButton* m_dayViewBtn;    // 天视图切换按钮
    QPushButton* m_monthViewBtn;  // 月视图
    QLineEdit*   m_nlInput;       // 🤖 AI 自然语言输入
    QPushButton* m_aiAddBtn;      // 🤖 AI 自然语言添加按钮
    QPushButton* m_recordBtn;     // 🎤 录音按钮

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
    // ---- 录音私有方法 ----
    /** 开始录音（后台加载模型 → 自动切换到录音） */
    void startRecording();
    /** 停止录音 */
    void stopRecording();
    /** 后台加载模型线程 */
    void loadModelInBackground();
    /** 启动录音 + 喂入线程 */
    void doStartRecording();
    /** 启动喂入线程 */
    void startFeedThread();
    /** 停止喂入线程 */
    void stopFeedThread();
    /** 检查录音工具 */
    bool checkRecordTool();
};

#endif // ECALENDER_MAINWINDOW_H


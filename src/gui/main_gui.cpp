//gui入口
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QFont>
#include <QFontDatabase>
#include <QMessageBox>
#include <QDebug>
#include <cstdlib>
#include <fstream>

#include <QByteArray>
#include <csignal>
#include "audio/SpeechRecognizer.h"
#include "gui/LoginDialog.h"
#include "gui/MainWindow.h"
#include "core/TaskManager.h"
/**
 * signalHandler - 信号处理函数
 *
 * 在收到 SIGINT 或 SIGTERM 时，先释放 Vosk 共享模型（~2GB），
 * 然后恢复默认信号处理并重新触发信号，让进程正常终止。
 */
static void signalHandler(int sig) {
    SpeechRecognizer::unloadModel();
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

/*
  main - Qt GUI 应用程序入口
*/
int main(int argc, char *argv[]) {
    // ---- 自动配置中文输入法 ----
    // 先设置环境变量，再启动输入法（顺序重要）
    // 检测并启动 ibus（优先，WSLg 兼容性更好)
    bool hasIbus = (std::system("which ibus-daemon 2>/dev/null > /dev/null") == 0);
    bool hasFcitx = (std::system("which fcitx-autostart 2>/dev/null > /dev/null") == 0);

    if (hasIbus) {
        if (std::system("pgrep -x ibus-daemon > /dev/null 2>&1") != 0) {
            qDebug() << "正在启动 ibus 中文输入法...";
            std::system("ibus-daemon -d -x 2>/dev/null");
            // 等待启动完成
            for (int i = 0; i < 10; i++) {
                if (std::system("pgrep -x ibus-daemon > /dev/null 2>&1") == 0) break;
                std::system("sleep 0.5");
            }
        }
        // 激活拼音引擎（让输入框可以直接输入中文，无需用户按 Ctrl+Space）
        std::system("ibus engine libpinyin 2>/dev/null");

        // 设置输入法环境变量
        qputenv("QT_IM_MODULE", QByteArray("ibus"));
        qputenv("GTK_IM_MODULE", QByteArray("ibus"));
        qputenv("XMODIFIERS", QByteArray("@im=ibus"));

  
        qputenv("IBUS_NO_PANEL", QByteArray("1"));

        qDebug() << "ibus 中文输入法已就绪";
    } else if (hasFcitx) {
        qputenv("QT_IM_MODULE", QByteArray("ibus"));
        qputenv("GTK_IM_MODULE", QByteArray("ibus"));
        qputenv("XMODIFIERS", QByteArray("@im=ibus"));

        if (std::system("pgrep -x ibus-daemon > /dev/null 2>&1") != 0) {
            qDebug() << "正在启动 ibus 中文输入法...";
            std::system("ibus-daemon -d 2>/dev/null");
        }
    } else {
        // 无输入法，保留默认设置
        qputenv("QT_IM_MODULE", QByteArray("ibus"));
        qputenv("GTK_IM_MODULE", QByteArray("ibus"));
        qputenv("XMODIFIERS", QByteArray("@im=ibus"));
    }
     // 注册信号处理器：退出时释放 Vosk 模型内存
    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    QApplication app(argc, argv);
    app.setApplicationName("Ecalender");
    app.setApplicationVersion("0.1");
    // ---- 设置中文字体 ----
    // 优先使用 SimSun（宋体），如果不可用则用 Noto Sans CJK SC
    QFont appFont;
    QStringList chineseFonts = {"SimSun", "Noto Sans CJK SC", "Noto Sans SC",
                                "Microsoft YaHei", "WenQuanYi Micro Hei",
                                "AR PL UMing CN", "AR PL UKai CN"};
    for (const QString& fontName : chineseFonts) {
        QFont testFont(fontName, 10);
        QFontDatabase fontDb;
        if (fontDb.hasFamily(fontName)) {
            appFont = testFont;
            qDebug() << "使用字体:" << fontName;
            break;
        }
    }
    if (appFont.family().isEmpty()) {
        appFont = QFont("Noto Sans CJK SC", 10);
        qDebug() << "使用默认字体";
    }
    app.setFont(appFont);

    // ---- 加载QSS样式表 ----
    // 优先搜索可执行文件同级目录，再搜索当前工作目录
    QStringList stylePaths;
    stylePaths << QApplication::applicationDirPath() + "/style.qss"
               << QDir::currentPath() + "/style.qss"
               << "src/gui/style.qss"
               << "../src/gui/style.qss";

    bool styleLoaded = false;
    for (const QString& path : stylePaths) {
        QFile styleFile(path);
        if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
            app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
            styleFile.close();
            qDebug() << "样式表已加载:" << path;
            styleLoaded = true;
            break;
        }
    }
    if (!styleLoaded) {
        qDebug() << "警告: 未找到 style.qss 样式表文件";
    }

    // ---- 加载 .env 环境变量 ----
    {
        // 收集更多可能路径：当前目录、上级、可执行文件同级、项目根目录
        QStringList envCandidates;
        envCandidates << ".env"
                      << "../.env"
                      << "../../.env"
                      << QApplication::applicationDirPath() + "/.env"
                      << QDir::currentPath() + "/.env";
        // 也尝试项目根目录 ECalendar/.env（构建目录的父目录）
        QDir curDir(QDir::currentPath());
        curDir.cdUp();
        envCandidates << curDir.absolutePath() + "/.env";

        for (const QString& envPath : envCandidates) {
            std::ifstream file(envPath.toStdString());
            if (!file) continue;
            qDebug() << "加载 .env:" << envPath;
            std::string line;
            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') continue;
                size_t eq = line.find('=');
                if (eq == std::string::npos) continue;
                std::string key = line.substr(0, eq);
                std::string value = line.substr(eq + 1);
                auto trim = [](std::string& s) {
                    size_t start = s.find_first_not_of(" \t\r\n");
                    size_t end = s.find_last_not_of(" \t\r\n");
                    if (start == std::string::npos) s.clear();
                    else s = s.substr(start, end - start + 1);
                };
                trim(key); trim(value);
                if (!key.empty() && !value.empty()) {
                    setenv(key.c_str(), value.c_str(), 0);
                    if (key == "DEEPSEEK_API_KEY") {
                        qDebug() << "DEEPSEEK_API_KEY 已加载";
                    }
                }
            }
            break;
        }
    }

    // ---- 创建 TaskManager ----
    TaskManager manager;

    // ---- 加载已有用户数据（确保登录时能验证）----

    QString homeDir = QString::fromLocal8Bit(qgetenv("HOME"));
    if (homeDir.isEmpty()) homeDir = QDir::homePath();
    QString dataDir = homeDir + "/.ecalender";
    QString userFile = dataDir + "/users.json";
    // 创建数据目录（如果不存在）
    QDir dir(dataDir);
    if (!dir.exists()) dir.mkpath(".");
    // 加载用户数据（任务数据在登录后按用户加载）
    manager.loadFromFile("", userFile.toStdString());

    // ---- 显示登录对话框 ----
    LoginDialog loginDlg(&manager);
    if (loginDlg.exec() == QDialog::Accepted) {
        // 登录成功，进入主窗口
        MainWindow mainWin(&manager, loginDlg.getUsername());
        mainWin.show();
        int ret = app.exec();
	// 用户关闭主窗口后，释放 Vosk 共享模型
        SpeechRecognizer::unloadModel();

        return ret;
    }
     // 释放 Vosk 共享模型（~2GB），防止内存泄漏
    SpeechRecognizer::unloadModel();


    // 用户取消登录，退出程序
    return 0;
}




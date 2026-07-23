//登录框实现

#include "gui/LoginDialog.h"
#include "core/TaskManager.h"
#include "core/Storage.h"
#include <QDir>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QApplication>
#include <QScreen>
#include <QFont>


LoginDialog::LoginDialog(TaskManager* manager, QWidget* parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_usernameEdit(nullptr)
    , m_passwordEdit(nullptr)
    , m_loginBtn(nullptr)
    , m_registerBtn(nullptr)
    , m_errorLabel(nullptr)
{
    setupUI();
}

// 初始化
void LoginDialog::setupUI() {
    // 窗口属性设置
    setWindowTitle("Ecalender - 登录");
    setFixedSize(400, 380);
    setObjectName("loginDialog");//与qss相匹配

    //  中央垂直布局 
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(16);

    // Logo/标题区域
    QLabel* titleLabel = new QLabel("Ecalender", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(25);
    titleFont.setBold(true);//加粗
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #4A90D9;");//蓝色
    mainLayout->addWidget(titleLabel);//添加到主布局

    QLabel* subtitleLabel = new QLabel("日程管理", this);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    QFont subtitleFont = subtitleLabel->font();
    subtitleFont.setPointSize(11);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setStyleSheet("color: #888888; margin-bottom: 8px;");
    mainLayout->addWidget(subtitleLabel);

    mainLayout->addSpacing(8);

    // 用户名输入
    QLabel* userLabel = new QLabel("用户名", this);
    userLabel->setStyleSheet("font-size: 10pt; color: #333333;");
    mainLayout->addWidget(userLabel);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("请输入用户名");//提示文字
    m_usernameEdit->setMaxLength(32);
    m_usernameEdit->setMinimumHeight(32);
    mainLayout->addWidget(m_usernameEdit);

    // 密码输入
    QLabel* passLabel = new QLabel("密  码", this);
    passLabel->setStyleSheet("font-size: 10pt; color: #333333;");
    mainLayout->addWidget(passLabel);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("请输入密码");
    m_passwordEdit->setEchoMode(QLineEdit::Password);//密码模式！显示原点不显示明文
    m_passwordEdit->setMaxLength(64);
    m_passwordEdit->setMinimumHeight(32);
    mainLayout->addWidget(m_passwordEdit);

    mainLayout->addSpacing(8);

    // ---- 按钮区域 ----
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(16);

    m_loginBtn = new QPushButton("登  录", this);
    m_loginBtn->setObjectName("primaryBtn");
    m_loginBtn->setDefault(true);//回车自动触发
    m_loginBtn->setMinimumHeight(36);
    m_loginBtn->setCursor(Qt::PointingHandCursor);//悬停提示

    m_registerBtn = new QPushButton("注  册", this);
    m_registerBtn->setObjectName("textBtn");
    m_registerBtn->setMinimumHeight(36);
    m_registerBtn->setCursor(Qt::PointingHandCursor);

    btnLayout->addStretch();//水平中央嵌套布局
    btnLayout->addWidget(m_loginBtn);
    btnLayout->addWidget(m_registerBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    // ---- 错误提示标签 ----
    m_errorLabel = new QLabel("", this);
    m_errorLabel->setObjectName("errorLabel");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setWordWrap(true);//自动换行
    m_errorLabel->hide();//默认隐藏
    mainLayout->addWidget(m_errorLabel);

    mainLayout->addStretch();//控件中央布局

    // 按钮与函数信号连接
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_registerBtn, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);

    // 按下回车键触发登录
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    connect(m_usernameEdit, &QLineEdit::returnPressed, this, [this]() {
        m_passwordEdit->setFocus();
    });
}


// 点击登录按钮
void LoginDialog::onLoginClicked() {
    std::string username = m_usernameEdit->text().trimmed().toStdString();
    std::string password = m_passwordEdit->text().toStdString();

    // 输入非空校验
    if (username.empty()) {
        m_errorLabel->setText("⚠ 请输入用户名");
        m_errorLabel->show();
        m_usernameEdit->setFocus();
        return;
    }
    if (password.empty()) {
        m_errorLabel->setText("⚠ 请输入密码");
        m_errorLabel->show();
        m_passwordEdit->setFocus();
        return;
    }

    // 调用 TaskManager 验证
    if (m_manager->login(username, password)) {
        m_username = QString::fromStdString(username);
        accept();  // 关闭对话框，返回 Accepted
    } else {
        m_errorLabel->setText("⚠ 用户名或密码错误");
        m_errorLabel->show();
        m_passwordEdit->selectAll();
        m_passwordEdit->setFocus();
    }
}

// 注册按钮点击
void LoginDialog::onRegisterClicked() {
    std::string username = m_usernameEdit->text().trimmed().toStdString();
    std::string password = m_passwordEdit->text().toStdString();

    // 输入非空校验
    if (username.empty()) {
        m_errorLabel->setText("⚠ 请输入用户名");
        m_errorLabel->show();
        m_usernameEdit->setFocus();
        return;
    }
    if (password.empty()) {
        m_errorLabel->setText("⚠ 请输入密码");
        m_errorLabel->show();
        m_passwordEdit->setFocus();
        return;
    }
    if (password.length() < 3) {
        m_errorLabel->setText("⚠ 密码长度至少3位");
        m_errorLabel->show();
        m_passwordEdit->setFocus();
        return;
    }

    // 调用 TaskManager 注册
    if (m_manager->registerUser(username, password)) {
        QString homeDir = QString::fromLocal8Bit(qgetenv("HOME"));
        if (homeDir.isEmpty()) homeDir = QDir::homePath();
        std::string userFile = homeDir.toStdString() + "/.ecalender/users.json";
   
        m_username = QString::fromStdString(username);
        accept();
    } else {
        m_errorLabel->setText("⚠ 用户名已存在");
        m_errorLabel->show();
        m_usernameEdit->selectAll();
        m_usernameEdit->setFocus();
    }
}






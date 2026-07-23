//登录对话框

#ifndef ECALENDER_LOGINDIALOG_H
#define ECALENDER_LOGINDIALOG_H
//从qss中加载
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class TaskManager;


//对话框
class LoginDialog : public QDialog {
    Q_OBJECT //槽

public:
    
    explicit LoginDialog(TaskManager* manager, QWidget* parent = nullptr);

    //Qt的字符串类型
    QString getUsername() const { return m_username; }

private slots://私有槽
    /** 点击登录按钮 - 验证用户名和密码 */
    void onLoginClicked();
    /** 点击注册按钮 - 注册新用户并自动登录 */
    void onRegisterClicked();

private:
    TaskManager* m_manager;     // 任务管理器
    QLineEdit*   m_usernameEdit; // 用户名输入框
    QLineEdit*   m_passwordEdit; // 密码输入框
    QPushButton* m_loginBtn;     // 登录按钮
    QPushButton* m_registerBtn;  // 注册按钮
    QLabel*      m_errorLabel;   // 错误提示标签
    QString      m_username;     // 登录成功的用户名

    /** 初始化用户界面布局 */
    void setupUI();
};

#endif // ECALENDER_LOGINDIALOG_H


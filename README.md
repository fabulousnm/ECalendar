# ECalendar
计算机编程实践大作业
### Ecalender初步功能需求：
1. 命令行界面
2. Linux Qt图形界面
3. 账户管理（口令Hash加密）
4. 任务录入（名称/时间/优先级/分类/提醒时间）
5. 任务保存（JSON文件，非数据库）
6. 任务加载（登录后从文件加载到内存）
7. 任务删除（按ID）
8. 任务显示（按天/月排序）
9. 任务提醒（屏幕打印）
10. 多线程（后台周期性检查）
11. 播放音乐/录音提醒
12. 语音对话录入

### 项目初步架构
分层实现: core存放核心功能代码，cli存放命令行有关代码，gui存放Qt图形界面代码，util存放工具函数如hash加密密码


依赖：
项目使用 `vosk-model-cn-0.22`（约 2.0 GB，推荐），也兼容 `vosk-model-small-cn-0.22`（42 MB，准确率较低）：

```bash
# 进入项目根目录
cd <你的Ecalender项目路径>/model

# 下载推荐模型（2.0 GB）
wget https://alphacephei.com/vosk/models/vosk-model-cn-0.22.zip
unzip vosk-model-cn-0.22.zip
rm vosk-model-cn-0.22.zip
必须下载到model里，否则代码路径不匹配

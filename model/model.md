`

项目使用 `vosk-model-cn-0.22`（约 2.0 GB，推荐），也兼容 `vosk-model-small-cn-0.22`（42 MB，准确率较低）：

```bash
# 进入项目根目录
cd <你的Ecalender项目路径>

# 下载推荐模型（2.0 GB）
wget https://alphacephei.com/vosk/models/vosk-model-cn-0.22.zip
unzip vosk-model-cn-0.22.zip
rm vosk-model-cn-0.22.zip

---
title: 更新日志
---

- [更新日志](#更新日志)

# 更新日志

**Features**

- 功能：新增`GstStreamOutput`类，支持通过 GStreamer 进行 RTP 推流 #1
- 功能：新增`GstSnoopOutput`类，支持通过 GStreamer 负责输入与输出，并在中间部分使用回调函数对视频流数据进行处理 #2

**Fixes**

- 修复：使得`GstRtspStreamOutput`类能够正常工作，支持通过 GStreamer 进行 RTSP 推流 #1

**Building**

- 构建：启用对`examples`文件夹内的示例程序的编译 #1

**Documentation**

- 文档：更新`README.md`，补充对于各类场景的支持矩阵 #1

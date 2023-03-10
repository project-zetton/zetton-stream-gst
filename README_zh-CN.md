# zetton-stream-gst

[English](README.md) | 中文

## 目录

- [zetton-stream-gst](#zetton-stream-gst)
  - [目录](#目录)
  - [简介](#简介)
  - [最新进展](#最新进展)
  - [安装](#安装)
  - [教程](#教程)
  - [支持的数据输入与输出](#支持的数据输入与输出)
  - [常见问题](#常见问题)
  - [贡献指南](#贡献指南)
  - [致谢](#致谢)
  - [许可证](#许可证)
  - [相关项目](#相关项目)

## 简介

zetton-stream-gst 是 [zetton-stream](https://github.com/project-zetton/zetton-stream) 软件包的一个开源扩展，它使用 GStreamer 框架实现视频流的处理。它是 [Project Zetton](https://github.com/project-zetton) 的一部分。

## 最新进展

有关详细信息和发布历史，请参阅 [changelog.md](docs/zh_CN/changelog.md) 。

有关 zetton-stream-gst 不同版本之间的兼容性更改，请参阅 [compatibility.md](docs/zh_CN/compatibility.md) 。

## 安装

请参考 [快速入门指南](docs/zh_CN/get_started.md) 获取安装说明。

## 教程

请查看 [快速入门指南](docs/zh_CN/get_started.md) 以了解 zetton-stream-gst 的基本用法。

## 支持的数据输入与输出

|  Task  | Protocol | Format | Encoding |  CPU  | Intel GPU | NVIDIA GPU | NVIDIA Jetson | Rockchip |
| :----: | :------: | :----: | :------: | :---: | :-------: | :--------: | :-----------: | :------: |
| Source |   V4L2   | MJPEG  |   JPEG   |   ✅   |     ❓     |     ❓      |       ❓       |    ❓     |
| Source |   V4L2   |  Raw   |    /     |   ✅   |     /     |     /      |       /       |    /     |
| Source |   RTSP   |   /    |  H.264   |   ✅   |     ❓     |     ❓      |       ❓       |    ❓     |
| Source |   RTMP   |   /    |  H.264   |   ❌   |     ❌     |     ❌      |       ❌       |    ❌     |
| Source |   RTP    |   /    |  H.264   |   ✅   |     ❓     |     ❓      |       ❓       |    ❓     |
|  Sink  |   RTSP   |   /    |  H.264   |   ✅   |     ❓     |     ❓      |       ❓       |    ❓     |
|  Sink  |   RTMP   |   /    |  H.264   |   ❌   |     ❌     |     ❌      |       ❌       |    ❌     |
|  Sink  |   RTP    |   /    |  H.264   |   ✅   |     ❓     |     ❓      |       ❓       |    ❓     |
|  Sink  |    /     |  MP4   |  H.264   |   ❌   |     ❌     |     ❌      |       ❌       |    ❌     |

- ✅: 支持且已测试
- ❓: 支持但未测试
- ❌: 暂时不支持

关于各个数据输入与输出的延迟和吞吐量，请参阅 [benchmark.md](docs/zh_CN/benchmark.md) 。

## 常见问题

请参考 [FAQ](docs/zh_CN/faq.md) 获取常见问题的答案。

## 贡献指南

我们感谢所有为改进 zetton-stream-gst 做出贡献的人。请参考 [贡献指南](.github/CONTRIBUTING.md) 了解参与项目贡献的相关指引。

## 致谢

我们感谢所有实现自己方法或添加新功能的贡献者，以及给出有价值反馈的用户。

我们希望这个软件包和基准能够通过提供灵活的工具包来部署模型，为不断发展的研究和生产社区服务。

## 许可证

- 对于学术用途，该项目在 2 条款 BSD 许可证下授权，请参阅 [LICENSE 文件](LICENSE) 以获取详细信息。

- 对于商业用途，请联系 [Yusu Pan](mailto:xxdsox@gmail.com)。

## 相关项目

- [zetton-stream](https://github.com/project-zetton/zetton-stream)：适用于视频流处理的主要软件包

- [zetton-ros-vendor](https://github.com/project-zetton/zetton-ros-vendor): ROS 相关示例

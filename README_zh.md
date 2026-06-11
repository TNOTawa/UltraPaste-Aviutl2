# UltraPaste for AviUtl2

[English](README.md) | [中文](README_zh.md)

将 REAPER 剪贴板数据导入 AviUtl2 时间轴的 `.aux2` 泛用插件。

> **状态**: 早期测试阶段，仅支持 REAPER 剪贴板。问题较多。

## 功能

- REAPER 中 `Ctrl+C` 复制 Item → AviUtl2 右键空白处 → **"[UltraPaste] 导入 REAPER 剪贴板"**
- 自动按轨道分层；同一轨道内重叠物件自动分配独立图层（贪婪首次适配）
- 同步 REAPER Item 的播放位置、播放速度、循环播放三项参数

## 安装

1. 在 [Releases](https://github.com/TNOTawa/UltraPaste-Aviutl2/releases) 下载 `UltraPaste.aux2`
2. 选择以下方式之一：
   - 放入 AviUtl2 的 `aviutl2\Plugin` 目录，重启 AviUtl2
   - 将 `UltraPaste.aux2` 直接拖拽到 AviUtl2 预览窗口中

## 使用方法

1. 在 REAPER 中框选 Item（可多轨），`Ctrl+C` 复制
2. 切换到 AviUtl2，在时间轴空白处右键 → **[UltraPaste] 导入 REAPER 剪贴板**
3. 物件按轨道分层，从当前光标层起始放置

## 构建

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
cmake --build build
# 产物: build/UltraPaste.aux2
```

依赖：MinGW-w64 (g++ 15+), CMake 3.20+

## 已知限制

- 仅支持 REAPER 剪贴板（`REAPERMedia` 格式），不支持 `.rpp` 文件导入
- 不导入 Envelope、Take、Stretch Marker、MIDI Item
- 无配置 UI，无持久化设置
- 物件创建使用 `create_object_from_media_file`，仅同步位置/速度/循环三项参数

## 许可

GNU Lesser General Public License v3.0 — 详见 `LICENSE` 文件

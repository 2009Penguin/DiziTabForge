# DiziTabForge

笛子简谱图片 → 带六孔指法手势图的 A4 PDF。

把简谱图片拖进窗口，程序调用 **GLM-4.6V-Flash** 视觉模型把图上的简谱识别成文本记法，
再用内置的 PDF 写入器渲染成竖版 A4 的指法手势谱 PDF：每个音符画出六个音孔（实心=按住、空心=松开），
音符上方是简谱记号，下方是时值延音线，最下面是该音的音名。

## 1. 依赖与构建

| 依赖 | 来源 | 版本 |
| --- | --- | --- |
| CMake | 自行安装 | ≥ 3.16 |
| C++ | 自行安装 | C++17 |
| Qt | 自行安装 | ≥ 6.2（Core / Gui / Widgets / Network） |
| libcurl | git 子模块 `deps/curl` | 随子模块固定 |
| nlohmann/json | git 子模块 `deps/json` | 3.12.0 |

**只有 Qt 需要自己装**。libcurl 与 nlohmann/json 都用 git 子模块引入，随项目一起编译，
不需要 vcpkg、conan 或任何系统预装库；libcurl 编译成静态库，Windows 上直接用系统自带的
Schannel 做 TLS，因此不会多出 `libcurl.dll` 这类运行时依赖。

PoDoFo 没有使用：PDF 由 `core/render/PdfWriter.cpp` 自己写（PDF 1.4 子集，A4 竖版、
内置 Helvetica、圆形用四段贝塞尔曲线、文字用 `Tj`），整个工程因此没有任何三方 C 库。

### 首次拉取子模块

```powershell
git submodule update --init --recursive
```

如果直连 GitHub 太慢，可以让 git 走国内镜像，而 `.gitmodules` 里仍保持上游地址：

```powershell
git config --global url."https://gitee.com/mirrors/curl.git".insteadOf "https://github.com/curl/curl.git"
git config --global url."https://gitee.com/mirrors/nlohmann-json.git".insteadOf "https://github.com/nlohmann/json.git"
git submodule update --init --recursive
```

（本机添加子模块时已经用了上面的镜像，所以克隆走的是 Gitee；这两条只是给其它机器/清空后重新拉取用。）

### 构建

Windows（MSVC + Ninja）：

```powershell
cmake -S . -B cmake-build-release -G Ninja ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_PREFIX_PATH="D:/Qt/6.8.3/msvc2022_64"
cmake --build cmake-build-release
```

Linux（Debian/Ubuntu，只需要 Qt6）：

```bash
sudo apt install qt6-base-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Qt 的 DLL 与 `platforms/qwindows.dll` 由 CMake 的 POST_BUILD 步骤自动复制到 exe 旁边，
libcurl 是静态库不需要复制 DLL。

## 2. 使用

1. 启动程序，点击右上角「设置」，填入智谱开放平台的 **API Key**，保存。
   配置写入 exe 同级的 `config.json`。
2. 把一张或多张简谱图片拖入窗口（也可以点「导入图片」）。
3. 双击缩略图可以裁剪：拖动鼠标框选要保留的区域，裁剪结果保存为同目录下的新 PNG
   并替换列表中的原图，源文件不会被修改。
4. 点右下角「生成 PDF」，识别完成后 PDF 与源图放在同一目录，文件名形如
   `dizi-tab-20250101-120000.pdf`，并自动用系统默认程序打开。

`config.json` 结构：

```json
{
    "apiKey": "你的智谱 API Key",
    "endpoint": "https://open.bigmodel.cn/api/paas/v4/chat/completions",
    "model": "glm-4.6v-flash",
    "timeoutSeconds": 90
}
```

## 3. 识别记法

模型被要求只输出下列记法，解析器严格按此规则解析：

| 记法 | 含义 |
| --- | --- |
| `1` `3` `6` | 中音区 |
| `1'` `3'` | 高音区（数字后一个单引号） |
| `6,` `7,` | 低音区（数字后一个逗号） |
| `-` | 延音线，延长前一个音一拍，不新建音符 |
| `0` | 休止符 |
| `#` `b` | 升号 / 降号，写在数字前面，作用于其后的音 |

音符用空格或换行分隔。无法解析的片段不会中断整首曲子：能解析的部分照常写入 PDF，
跳过的片段会在界面上提示。

## 4. 指法表（筒音作 5）

`core/render/FingerTable.cpp` 中是一张静态表，行 = `音符序号 + 升降号 + 音区掩码`，
列 = 六个音孔的按/开状态。表的音域覆盖筒音作 5 的常用音域：

| 音区掩码 | 对应记法 | 音域 |
| --- | --- | --- |
| `0b001` | `6,` `7,` `1` `2` `3` `4` `5` | 低音区（筒音 G4 起） |
| `0b010` | `1` … `7` | 中音区（B4 – C5） |
| `0b100` | `1'` `2'` `3'` `5'` `6'` `7'` | 高音区（C6 – B6） |

表里没有的音（例如超出三个八度的极端音、筒音作 5 之外调性体系需要的其它音）
会在 PDF 上标成 `?`，不会让渲染失败。

## 5. 目录结构

```
project_root/
├── CMakeLists.txt              唯一的构建文件：依赖 + 全部库与可执行目标 + Windows 部署
├── .gitmodules                 curl 与 json 两个子模块（地址指向上游）
├── main.cpp                    只创建 QApplication 与 MainWindow
├── deps/                       第三方依赖，全部以 git 子模块引入
│   ├── CMakeLists.txt          构建选项、curl 目标名适配，并导出 dizi_deps 接口目标
│   ├── curl/                   git 子模块，静态编译，Windows 用 Schannel
│   └── json/                   git 子模块，nlohmann/json
├── core/                       全部核心逻辑，只依赖 QtCore / QtGui
│   ├── model/                  Note、FingerPattern（纯数据体）
│   ├── util/                   StringUtil / ImageUtil / PathUtil（命名空间自由函数）
│   ├── config/                 ConfigRepository（读写 config.json）
│   ├── parse/                  记法 → Score 的四段解析
│   │   ├── ScoreToken.h            词法单元与记号类型
│   │   ├── ScoreTokenizer          词法：字符流 → 记号流
│   │   ├── ScoreParser             语法：把升降号/八度记号归并到音符
│   │   ├── ScoreInterpreter        语义：记号流 → Note 序列（延音线、休止符）
│   │   └── ScorePipeline           串起上面三段，对外只暴露 parse()
│   ├── network/                ImageEncoder（文件 → data URI）、GmlClient（libcurl POST）
│   └── render/                 FingerTable（指法表）、NoteFormatter（记法文字）
│       ├── PdfWriter           零依赖 PDF 1.4 写入器（页面、文字、圆形、xref）
│       └── PdfRenderer         把 Score 排成 A4 版面并调用 PdfWriter
└── ui/                         全部界面代码，依赖 core，反向不成立
    ├── MainWindow(.h/.cpp/.ui)     装配与信号连接
    ├── ImageDropWidget(.h/.cpp)    拖拽区控件
    ├── CropDialog(.h/.cpp/.ui)     裁剪对话框
    └── SettingsDialog(.h/.cpp/.ui) API Key 设置对话框
```

依赖方向：`ui → core`，`core` 内部 `parse/network/render → model/util`，任何一层都不反向依赖。

构建目标全部定义在根目录的 `CMakeLists.txt` 里，按层拆成 6 个静态库 + 1 个接口库 + 1 个可执行目标：

| 目标 | 类型 | 内容 | 链接 |
| --- | --- | --- | --- |
| `dizi_model` | INTERFACE | `core/model` 纯头文件 | Qt6::Core |
| `dizi_util` | STATIC | `core/util` | Qt6::Core / Gui |
| `dizi_config` | STATIC | `core/config` | dizi_util |
| `dizi_parse` | STATIC | `core/parse` | dizi_model / dizi_util |
| `dizi_network` | STATIC | `core/network` | dizi_model / dizi_config / dizi_util + `dizi_deps` |
| `dizi_render` | STATIC | `core/render` | dizi_model / dizi_util / dizi_parse |
| `dizi_ui` | STATIC | `ui/`（含三个 `.ui`） | 上述全部 + Qt6::Widgets / Network |
| `DiziTabForge` | EXECUTABLE | `main.cpp` | dizi_ui |

`deps/CMakeLists.txt` 通过 `add_subdirectory` 接入两个子模块，并按实际情况把 curl 的目标名
解析成 `libcurl_static`（找不到时退回 `libcurl`），最后用 `dizi_deps` 这个 INTERFACE 目标
同时导出 curl 与 nlohmann_json，根 CMakeLists 因此不必关心 curl 内部的目标命名。
这两个依赖只出现在 `dizi_network` 的 `PRIVATE` 链接项里，头文件路径不会传播到 UI 层。

## 6. 设计约束

- UI 的布局、控件树、控件属性与视觉样式全部写在 `.ui` 文件里，C++ 中没有 `setStyleSheet`，
  工程里也没有 `.qss` 文件。
- 拖拽区的悬停高亮用「动态属性 `dropActive` + `.ui` 中的属性选择器 + `style()->polish()`」实现。
- 与对象状态无关的辅助函数一律是命名空间里的自由函数，没有只装静态方法的工具类。
- 指法映射与绘制参数都是静态表，没有长 if-else 链或按音高逐个判断的 switch。
- libcurl 与 nlohmann/json 的头文件只出现在 `core/network/GmlClient.cpp` 里，头文件中没有它们。

## 7. 已知限制

- 网络请求在后台线程执行，界面上的进度表示「已上传 / 总字节」，进度回调频率取决于 libcurl。
- 若识别结果里出现了超出三八度的音，或者指法表里没有的音，PDF 上用 `?` 占位。
- `ConfigRepository::load` 在 `config.json` 不存在时会直接回退到默认配置，不视为错误。
- 内置 PDF 写入器只实现渲染用到的子集（未压缩内容流、Standard 14 字体的 Helvetica、
  无书签/超链接）；PDF 里的文字目前是 ASCII 记法，中文标注需要后续把字体换成嵌入的 CJK 字体。
- 子模块当前固定在 curl `master` 与 json `develop` 的浅克隆上；要锁版本就在子模块里
  `git checkout <tag>` 再提交，父仓库会记录新的 submodule commit。

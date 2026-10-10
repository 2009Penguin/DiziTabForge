# DiziTabForge

笛子简谱图片 → 带六孔指法手势图的 A4 PDF。

把简谱图片导入窗口，程序调用 **Qwen/Qwen3-VL-8B-Instruct** 视觉模型把图上的简谱识别成文本记法，
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

curl 只需要 HTTP/HTTPS，其余协议（FTP、SMTP、SSH/SFTP、IPFS、WebSocket、dict、imap、pop3、
telnet、tftp、rtsp、mqtt、gopher、ldap、file 等）在 `CMakeLists.txt` 里全部编译掉，
因此它不会再去探测 libssh2 / libpsl / GSSAPI / zlib / brotli / zstd 这些系统库 ——
这一步是必需的：机器上装了 Anaconda 时，curl 会从 Anaconda 里翻出旧版 libssh2 导致
CMake 生成阶段报 `libssh2::libssh2` 目标不存在。CMakeLists 里另外把常见 Anaconda
安装目录加进了 `CMAKE_IGNORE_PREFIX_PATH`，避免 `find_package` 误取其中的库。

PDF 也不依赖任何三方库：`core/render/` 下用**策略模式**把「版面」和「PDF 引擎」分开，
`PdfRenderer` 只负责排版，落盘交给 `PageCanvas` 接口的两个实现：

| 策略 | 实现 | 说明 |
| --- | --- | --- |
| `CanvasKind::QtPdf`（默认） | `QtPdfCanvas` | 用 Qt 自带的 PDF 引擎（`QPdfWriter` + `QPainter`，属 Qt6::Gui），字体子集、xref、压缩全部由 Qt 负责 |
| `CanvasKind::Handwritten` | `VectorPdfCanvas` | 纯手写的 PDF 1.4 写入器（`PdfWriter` + `TrueTypeFace` 自建 CJK 子集），不依赖 QtGui 之外的任何东西 |

换引擎只要给 `PdfRenderer` 构造函数传另一个 `CanvasKind`，版面代码一行都不用改。

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

Qt 的 DLL、`platforms/qwindows.dll` 以及图片格式插件（`imageformats/qjpeg`、`qgif`、`qwebp`、
`qico`、`qtiff`、`qsvg`）由 CMake 的 POST_BUILD 步骤自动复制到 exe 旁边的 `plugins/` 目录里；
少了 imageformats 就只能读 Qt 内置的 PNG / BMP，JPEG、GIF、WebP 一律报
`Unsupported image format`。
libcurl 是静态库不需要复制 DLL。

## 2. 使用

**前置条件（魔搭）**：魔搭的 API-Inference 要求账号**已绑定阿里云账号并完成实名认证**，
绑定后才会给免费调用额度。否则无论填什么模型都会返回
`401 Please bind your Alibaba Cloud account before use.`。绑定入口在魔搭官网右上角头像 →
账号设置里的"阿里云账号绑定"。

1. 启动程序，点击右上角「设置」，填入 **API Key** 保存。配置写入 exe 同级的 `config.json`。
   默认接的是**魔搭 ModelScope** 的 `Qwen/Qwen3-VL-8B-Instruct`，所以这里填魔搭的 Key（`ms-` 开头）。
2. 点中间的「导入图片」，选择一张或多张简谱图片（已有图片时列表右上角是「继续导入」）。
3. 双击缩略图可以裁剪：拖动鼠标框选要保留的区域，裁剪结果保存为同目录下的新 PNG
   并替换列表中的原图，源文件不会被修改。
4. 点右下角「生成 PDF」，先弹出**另存为**对话框选择保存位置（默认文件名
   `dizi-tab-<时间戳>.pdf`，默认目录是第一张图片所在目录），识别完成后写入该 PDF
   并用系统默认程序打开。程序不会往图片目录写任何东西。

`config.json` 结构（默认值）：

```json
{
    "apiKey": "你的魔搭 API Key",
    "endpoint": "https://api-inference.modelscope.cn/v1",
    "model": "Qwen/Qwen3-VL-8B-Instruct",
    "timeoutSeconds": 90
}
```

`endpoint` 填 **base URL 或完整的 `chat/completions` 地址都可以**：程序会在缺少
`/chat/completions` 时自动补全，末尾多余的 `/` 也会被去掉。所以下面三种写法等价：

```
https://api-inference.modelscope.cn/v1
https://api-inference.modelscope.cn/v1/
https://api-inference.modelscope.cn/v1/chat/completions
```

换用阿里云百炼（Model Studio）也可以，只要改这两项：

```json
{
    "endpoint": "https://dashscope.aliyuncs.com/compatible-mode/v1",
    "model": "qwen3-vl-8b-instruct"
}
```

两边都是 OpenAI 兼容的 `chat/completions` 接口，请求体（`messages[].content` 里的
`text` 与 `image_url`）完全一致，因此换服务商不需要改代码，只改这两项即可。
注意各地域的百炼 Key 与地址不通用；模型名大小写按各平台的写法填（魔搭是
`Qwen/Qwen3-VL-8B-Instruct`，百炼是 `qwen3-vl-8b-instruct`）。

## 3. 识别记法

提示词要求模型**只认简谱、其余内容一律屏蔽**：歌词、曲名标题、作者/编曲/演唱者、调号与拍号的
说明文字、页码、水印、二维码、印章、logo、指法与演奏提示文字、五线谱及其音符谱号、和弦记号、
速度/力度/表情记号（♩=90、D.C.、ff 之类）、乐器与段落说明、背景图案、页边批注都要忽略；
简谱行内部的小节线、反复记号、跳房子记号、连音线、换气记号、装饰音标记也不输出。
真正写进 PDF 的只有下面这套记法，解析器严格按此规则解析：

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

`core/render/FingerTable.cpp` 中是一张静态表，每行是 `音符序号 + 升降号 + 音名 + 六孔按/开状态`，
六个位置自上而下对应笛子的第一孔到第六孔，`1` 表示按住、`0` 表示松开：

| 记法 | 音名 | 六孔状态（第一孔 → 第六孔） |
| --- | --- | --- |
| `1` | C5 | ● ○ ○ ○ ○ ○ |
| `2` | D5 | ○ ○ ○ ○ ○ ○ |
| `3` | E5 | ○ ○ ○ ○ ○ ● |
| `4` | F5 | ● ● ● ● ● ● |
| `5` | G5 | ● ● ● ● ● ● |
| `6` | A5 | ● ● ● ● ● ○ |
| `7` | B5 | ● ● ● ● ○ ○ |

查询时按 `音符序号 + 升降号` 查表（同一序号在低、中、高八度共用同一套孔位），
命中不到的音会返回音名 `?` 并标记为超吹音，**不会让渲染失败**。当前表覆盖的是
筒音作 5 的一个八度（B4 – B5，即记法 `1` 到 `7`）：

- 带 `,` 的低音区落在 B4 – G4，其中 `1` `2` `3` 有对应孔位，`4` `5` `6` `7` 会显示为 `?`；
- 带 `'` 的高音区目前一律显示为 `?`，需要时按下表格式补行即可（例如高音 `1'` 是
  `● ● ● ○ ○ ○`，高音 `2'` 是 `● ● ○ ○ ○ ○`）。

休止符（`0`）没有指法，因此只画记号与时值线，不画六孔图、也不标音名；查表失败的音同理，
只会画记号而不会画出误导性的孔位图。

想扩表就在 `patternTable()` 的 `std::array` 里加行，不需要改任何别的代码。

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
│   ├── model/                  Note、FingerPattern（纯数据体，FingerTable 直接用 Fingering 指代它）
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
│       ├── PageCanvas          策略接口：addPage / drawText / drawHole / finish
│       ├── QtPdfCanvas         策略一：Qt 自带 PDF 引擎（QPdfWriter + QPainter）
│       ├── VectorPdfCanvas     策略二：手写 PDF 写入器（含 CJK 子集嵌入）
│       ├── TrueTypeFace        手写 TrueType 解析与按需字形子集（cmap / glyf / loca / hmtx …）
│       ├── CjkFontFile         打开中文字体并按用到的字抽出子集（不依赖 Qt）
│       ├── PdfWriter           零依赖 PDF 1.4 写入器（页面、文字、圆形、嵌入字体、xref）
│       └── PdfRenderer         只做版面：把 Score 排成 A4 网格，画到 PageCanvas 上
└── ui/                         全部界面代码，依赖 core，反向不成立
    ├── MainWindow(.h/.cpp/.ui)     装配与信号连接
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
- 图片只通过「导入图片」按钮选择，没有拖拽逻辑，因此也不需要向控件写入拖拽相关的样式或状态。
- 与对象状态无关的辅助函数一律是命名空间里的自由函数，没有只装静态方法的工具类。
- 指法映射与绘制参数都是静态表，没有长 if-else 链或按音高逐个判断的 switch。
- libcurl 与 nlohmann/json 的头文件只出现在 `core/network/GmlClient.cpp` 里，头文件中没有它们。

## 7. 已知限制

- 网络请求在后台线程执行，界面上的进度表示「已上传 / 总字节」，进度回调频率取决于 libcurl。
- 图片**按内容判断真实格式，不看扩展名**：`.png` 里其实是 JPEG 的图也能正常导入，发给模型时
  `data:` 的 MIME 也是按真实格式写的。反过来，如果格式真的不受支持（缺插件），报错会直接写明
  「文件是 XXX 格式，但程序缺少该格式的解码插件」。
- 若识别结果里出现了超出三八度的音，或者指法表里没有的音，PDF 上用 `?` 占位。
- `ConfigRepository::load` 在 `config.json` 不存在时会直接回退到默认配置，不视为错误。
- 手写那套 PDF 写入器（`VectorPdfCanvas` + `PdfWriter`）只实现渲染用到的子集（未压缩内容流、
  Standard 14 字体的 Helvetica、嵌入的 TrueType CJK 子集字体；无书签 / 超链接 / 压缩）。
  默认走的是 Qt 引擎，不受这些限制影响。
- 子模块当前固定在 curl `master` 与 json `develop` 的浅克隆上；要锁版本就在子模块里
  `git checkout <tag>` 再提交，父仓库会记录新的 submodule commit。

## 8. PDF 版面与字体

- 每个音符占一个固定格子（7 列 x 7 行 = 49 个/页），格子内自上而下是：**简谱符号**、
  **时值延音线**，然后是**六孔手势图**，**音名**紧贴孔位左侧、与第一个孔同一行。
  这样「数字—本格的孔位图」一眼能对上，不会误读成上一格的内容。
- **页面上不写来源图片的文件名**（也不再是 "Part N"），只有页脚：左侧 `page i/n`、
  右侧调性提示 `筒音作5`。
- 多张图片的识别结果会**合并进同一个 PDF**，每张图片各自成页；超过 49 个音符会自动续页。
- 中文与数字都用**随程序打包的中文字体**渲染：`res/fonts/WenYuanSerifSC-Regular.ttf` 经
  `res.qrc` 编进 exe。Qt 引擎把它注册进 QFontDatabase 后由 Qt 做子集嵌入；手写引擎优先从
  Qt 资源读取，读不到再依次退回 exe 同级的 `fonts/`、`res/fonts/`，最后才找系统的
  `simhei.ttf`、`Deng.ttf`、`simkai.ttf`。两条路径都能在任何机器上正确显示中文。
- 手写引擎的字形子集固定包含 ASCII `0x20`–`0x7E`、`'` `,` 以及本页用到的全部文字
  （页脚的调性提示）；万一出现字体里没有的字符会退化成 `?`，不会让整页变成乱码。
- 休止符与超出指法表的音不画六孔图，只画简谱记号与时值线。

## 9. 保存位置

点「生成 PDF」后弹出**另存为对话框**，默认文件名是 `dizi-tab-<时间戳>.pdf`，默认目录为第一张图片所在目录，
可另存到任意路径；取消则中止生成。程序不会自动往图片目录写入任何文件。
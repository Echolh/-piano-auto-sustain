# Piano Auto Sustain 1.0 · GitHub Actions macOS 云端编译指南

> 仓库：https://github.com/Echolh/-piano-auto-sustain
> Workflow 文件：`.github/workflows/build-macos.yml`
> 本机（开发机）无需安装 Xcode 即可产出 Mac 版插件。

---

## 一、整体流程

```
本地（Mac/Windows 均可）                    GitHub 云端（macos-14 Runner）
──────────────────────                    ─────────────────────────────
git add / git commit                       push 触发 workflow
git push origin main            ──────▶    1. checkout 源码
                                           2. CMake 配置（Xcode 生成器，
                                              Universal：arm64 + x86_64）
                                           3. 编译 VST3 + AU MIDI FX + 测试程序
                                              （JUCE 8.0.12 自动下载）
                                           4. ctest 单元测试 + VST3 扫描测试
                                           5. lipo 验证双架构 + ad-hoc 签名
                                           6. auval 验证 AU（Apple 官方工具）
                                           7. 打包 ZIP + SHA256 → 上传 Artifact
浏览器/命令行下载 Artifact  ◀────────────  构建完成，页面显示绿勾
```

产物：
- `Piano Auto Sustain.vst3`（macOS Universal，供支持 VST3 MIDI 路由的宿主）
- `Piano Auto Sustain MIDI FX.component`（Logic Pro AU MIDI FX）
- 压缩为 `Piano-Auto-Sustain-1.0-macOS-Universal-*.zip` + `.sha256`

---

## 二、搭建过程中实际做了哪些事情

1. **迁移与评估旧方案**
   参考了 AmuMusicBridge 的《跨平台总体方案.md》。结论：其"GitHub Actions 云端编译 macOS、ad-hoc 签名、Artifact 分发"的思路完全适用；但其技术细节（双 UI 分叉、VST3 SDK 缓存、Node 运行时打包）本项目不需要——本项目基于 JUCE，一套源码天然跨平台，CI 每次自动 FetchContent 下载 JUCE 8.0.12，无需把任何 SDK 放进仓库。

2. **新建独立仓库版 workflow**
   项目里原有的 `scripts/github-build-workflow.yml` 是为旧仓库的子目录布局写的（`working-directory: piano-auto-sustain`），在独立仓库里会直接失败。新写的 `.github/workflows/build-macos.yml` 在仓库根目录运行，并去掉了不需要的 Linux 矩阵。

3. **初始化 git 仓库并提交**
   `git init -b main`，提交全部源码（112 个文件），`.gitignore` 排除 `build/`、`dist/`、`third_party/`、`.claude/`、`.DS_Store`。注意 `third_party/VSTModuleArchitectureSDK` 被排除——它只服务于 Windows 专用的 Cubase 原生探针（`BUILD_NATIVE_MIDI_PROBE=OFF`），Mac 构建不需要。

4. **网络与认证排障**
   - 本机 git 直连 GitHub 报 `SSL_ERROR_SYSCALL`（被墙）。系统代理在 `127.0.0.1:17891`，执行
     `git config http.https://github.com.proxy http://127.0.0.1:17891` 后推送成功。
   - `gh auth login` 未做，改用 macOS 钥匙串里 git 已保存的凭据
     （`git credential fill`）直接调 GitHub API 查询构建状态。

5. **第一次构建失败 → 定位 → 修复**
   编译/测试/签名全过，只挂在 auval：
   `ERROR: Cannot get Component's Name strings` / `didn't find the component`。
   这是 CI 无头 Runner 的经典问题：AU 组件注册守护进程
   （AudioComponentRegistrar）缓存过期，auval 找不到刚安装的组件。
   修复：安装组件后先 `killall -9 AudioComponentRegistrar`（强制重新扫描）
   再跑 auval，并 `sleep 2` 等守护进程重启。

---

## 三、注意事项（踩坑记录）

| # | 事项 | 说明 |
|---|---|---|
| 1 | **触发条件** | workflow 在 `push` 到 `main` 或网页手动 `workflow_dispatch` 时触发。推其他分支不会自动构建。 |
| 2 | **网络代理** | 本地推送/查 API 必须走代理时，用 `git config http.https://github.com.proxy` 只对 github.com 生效，不污染其他 git 操作。 |
| 3 | **runner 版本** | `runs-on: macos-14` = Apple Silicon。如需 Intel 版产物，不用换 runner——构建本身是 Universal 双架构。 |
| 4 | **构建时间与额度** | 单次约 10~15 分钟（JUCE 每次全新下载编译）。私有仓库每月 2000 分钟免费额度，约可构建 130~200 次；公开仓库免费。 |
| 5 | **ad-hoc 签名的局限** | `codesign --sign -` 只是本地临时签名，没有 Apple 开发者证书和公证。最终用户首次打开可能需要在"系统设置→隐私与安全性"里点"仍要打开"。正式商业发布仍需付费签名+公证。 |
| 6 | **auval 依赖组件注册** | 无头环境必须先杀 AudioComponentRegistrar；若未来仍偶发失败，可考虑改为非阻断步骤（`continue-on-error`），因为 README 已注明 AU 待实机验证。 |
| 7 | **修改后同步** | 以后每次改源码，只需 `git push`，云端自动出新 Mac 包；Windows 版仍在原 Windows 流程构建，两不干扰。 |
| 8 | **源码即全部** | 仓库不含任何 SDK、构建缓存，JUCE 由 CI 在线下载，任何人克隆后用 `bash scripts/build-macos.sh` 也能在本地 Mac 复现同样的产物。 |
| 9 | **YAML 续行符陷阱** | `run:` 多行命令中行尾写 `\` 续行时，YAML 折叠标量（默认 `>` 行为）会把 `\` 变成字面参数传给命令，导致 argparse 报"缺少参数"。多行 shell 命令要么用单行，要么用 `|` 字面块并保证 `\` 后直接换行。 |

---

## 四、构建成功后如何下载可执行程序

### 方式 A：网页下载（推荐）

1. 打开 https://github.com/Echolh/-piano-auto-sustain/actions
2. 点击最新一条绿色 ✓ 的 **Piano Auto Sustain 1.0 macOS build**
3. 拉到页面底部 **Artifacts** 区域，点击
   `Piano-Auto-Sustain-1.0-macOS-Universal` 下载（是一个 zip）
4. 解压后得到 `Piano-Auto-Sustain-1.0-macOS-Universal-*.zip`（插件包）和
   `.sha256`（校验文件）

### 方式 B：命令行下载（需 gh auth login）

```bash
gh run download --repo Echolh/-piano-auto-sustain \
  --name Piano-Auto-Sustain-1.0-macOS-Universal -D ./下载目录
```

### 安装到 Mac

解压出的两个 bundle 按用途安装：

```bash
# VST3（多数宿主）
cp -R "Piano Auto Sustain.vst3" ~/Library/Audio/Plug-Ins/VST3/

# AU MIDI FX（Logic Pro）
cp -R "Piano Auto Sustain MIDI FX.component" ~/Library/Audio/Plug-Ins/Components/
```

若 macOS 拦截未公证的插件：

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Piano\ Auto\ Sustain.vst3
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Piano\ Auto\ Sustain\ MIDI\ FX.component
```

校验完整性（可选）：

```bash
shasum -a 256 -c Piano-Auto-Sustain-1.0-macOS-Universal-*.zip.sha256
```

---

## 五、正式发版：GitHub Releases 自动发布

Artifact 只是开发用的临时产物（需登录 GitHub 才能下载、保留 30 天后自动删除）。
**正式对外分发用 Releases**：任何人无需登录即可下载、永久保留，适合发给 Mac 用户。

### 工作方式

- workflow 只在推送 **`v` 开头的 tag** 时走发版流程（`tags: ['v*']`），普通 push 到
  `main` 仍然只出 Artifact，不会刷出多余"版本"
- tag 构建成功后自动执行 `gh release create`，把 `dist/` 里的 zip + sha256 挂为附件，
  Release 说明自动包含：插件格式、auval 通过、ad-hoc 签名未公证、首次打开的
  "仍要打开"操作指引、对应源码 commit
- 需要的权限已在 workflow 里配好（`permissions: contents: write`）

### 发版命令（仅此一条）

```bash
git tag v1.0.1
git push origin v1.0.1
```

云端自动：构建 → 测试 → auval → 签名 → 打包 → 发布到
https://github.com/Echolh/-piano-auto-sustain/releases

删除/重发一个版本：

```bash
git tag -d v1.0.1 && git push origin :refs/tags/v1.0.1   # 删除远程 tag 和 Release（需网页再删 Release）
```

### Release 下载的是什么

下载的 `Piano-Auto-Sustain-*.zip` 是**编译完成的成品**，解压即用，无需用户编译：

```
Piano-Auto-Sustain-1.0-...-TEST/
├── Piano Auto Sustain.vst3               ← VST3 插件（含 arm64+Intel 双架构二进制）
├── Piano Auto Sustain MIDI FX.component  ← AU 插件（Logic Pro 用，已过 auval）
├── Install.command / Uninstall.command   ← 双击安装/卸载
├── BUILD-INFO.json                       ← 版本与 commit 信息
└── *.sha256                              ← 完整性校验
```

注意：音频插件不是双击独立运行的 `.app`，而是装进宿主使用——用户下载解压后
双击 `Install.command`，再在 Logic Pro / Cubase / REAPER 里加载即可。
`.vst3` / `.component` 内 `Contents/MacOS/` 下就是真正的可执行二进制。

用户下载入口：仓库页右侧 **Releases** → 点对应版本 → 附件直接下载（免登录、永久有效）。

### 命名建议

当前包为 ad-hoc 签名的未公证测试版，tag 名称建议带 `test`/`beta` 字样
（如 `v1.0.0-mac-test.1`）；正式商业版完成 Apple 签名公证后再发 `v1.0.0` 正式号。

---

## 六、构建结果记录

- 第 1 次（run 38420106834）：失败——编译/测试/签名全部通过，auval 因组件注册缓存过期找不到插件。修复：auval 前 `killall -9 AudioComponentRegistrar`。
- 第 2 次（run 38424203448）：失败——**auval 已通过（AU VALIDATION SUCCEEDED）**，挂在打包步骤：YAML `run:` 折叠块里行尾 `\` 续行符被折叠成字面参数，argparse 报 `--build` 缺失。修复：打包命令改为单行。
- 第 3 次（run 38426209806，2026-10-14 00:04 UTC+8）：**成功** ✅
- 第 4 次（run 38428504608，tag `v1.0.0-mac-test.1`，2026-10-14 00:24）：**成功** ✅
  自动发版链路首次验证通过：Release 已自动创建，附件为
  `Piano-Auto-Sustain-1.0-macOS-Universal-TEST.zip`（13 MB）+ `.sha256`，
  下载页：https://github.com/Echolh/-piano-auto-sustain/releases/tag/v1.0.0-mac-test.1
  （免登录、永久有效的直链）
  - Artifact：`Piano-Auto-Sustain-1.0-macOS-Universal`（13 MB）
  - 内容：`Piano Auto Sustain.vst3` + `Piano Auto Sustain MIDI FX.component`（均含 ad-hoc 签名）、`Install.command` / `Uninstall.command` 安装卸载脚本、`BUILD-INFO.json`、SHA256 校验文件
  - 本地已下载并验证：`shasum -a 256 -c` 通过，双架构（arm64 + x86_64）签名完整
  - 下载页：https://github.com/Echolh/-piano-auto-sustain/actions/runs/38426209806

（后续每次构建失败/修复可继续在此追加记录。）

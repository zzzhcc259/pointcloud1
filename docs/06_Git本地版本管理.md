# 06 Git 本地版本管理

> 这份文档只讲一件事：**让你敢改代码。**
> 改坏了能退回去，你才敢动手；不敢动手，看一百份文档也学不会。

---

## 一、Git 到底是什么（和 GitHub 无关）

### 最常见的误解

> "Git 是要把代码传到网上给别人看的。"

**错。** Git 分两半：

| | 是什么 | 需要联网吗 |
|---|---|---|
| **Git** | 一个**本地**的版本管理工具，装在你自己电脑上 | **不需要** |
| **GitHub / Gitee** | 一个**网站**，用来存放 Git 仓库的副本 | 需要 |

打比方：

- **Git** = 你笔记本上的"存档/读档"功能
- **GitHub** = 云存档

**你完全可以只用本地存档，永远不放云上。** 这是本项目推荐的做法。

### Git 能帮你做什么（就三件事）

1. **留档**：把代码在某个时刻的样子存下来，写一句话说明改了什么
2. **对比**：看看现在和第 3 次存档之间改了什么
3. **读档**：改坏了，回到第 3 次存档的状态

**就这三件事。** 学会了就能覆盖 90% 的日常使用。

---

## 二、先确认 Git 装了没有

```powershell
git --version
```

本机实测输出：

```
git version 2.55.0.windows.1
```

装在哪里（本机探查结果）：

```powershell
(Get-Command git).Source
# E:\TOOL\Git\cmd\git.exe
```

> 如果 `git --version` 报"不是内部或外部命令"，说明 Git 没在 PATH 里。
> 重开一个 PowerShell 窗口；还不行就重新打开终端或者检查安装。

### 第一次使用必须先设身份

```powershell
git config --global user.name "你的名字"
git config --global user.email "you@example.com"
```

**这不是注册账号，不会联网，也不会上传任何东西。** 它只是给每次存档贴个名字，
方便以后 `git log` 时知道是谁存的。

检查是否设好：

```powershell
git config --global --list
```

**注意**：这些信息存在 `C:\Users\<你>\.gitconfig` 里（一个小文本文件，几行），
不占空间。

---

## 三、初始化仓库：`git init`

```powershell
cd E:\dianyunxulian
git init
```

预期输出：

```
Initialized empty Git repository in E:/dianyunxulian/.git/
```

**这一步做了什么**：在当前目录创建一个隐藏文件夹 `.git\`，
里面就是你的"存档柜"。**没有这个文件夹，Git 完全不认识这个目录。**

确认它存在：

```powershell
Get-ChildItem E:\dianyunxulian -Force -Filter .git
```

**重要**：`.git` 里存的是**完整的版本历史**，所以它会越来越大。
但你不用管它，也不用看它。

### 三个"工作区"

这是理解 Git 最关键的一张图：

```
   你的文件夹          暂存区            仓库
  (工作区 /          (staging          (.git 里的
   Working Tree)      area / Index)     每次提交)
       │                  │                │
       │  git add ────────►│                │
       │                  │  git commit ──►│
       │◄──── git checkout / git restore ──│
       └──────────────────┴────────────────┘
```

- **工作区** = 你眼睛看到的文件，你用编辑器改的就是它
- **暂存区** = 一个"待存档清单"，`git add` 把文件放上去
- **仓库** = 已经存好的历史，`git commit` 才真正存

**新手最容易困惑的点**：改了文件不等于存了档。
**改了 → `git add` → `git commit`，三步都做完才算存档。**

---

## 四、`git status`：最常用的命令

```powershell
git status
```

刚 `git init` 之后，在本项目里跑，会看到类似：

```
On branch master

No commits yet

Untracked files:
  (use "git add <file>..." to include in what will be committed)
        .gitignore
        CMakeLists.txt
        README.md
        data/
        docs/
        practice/
        src/

nothing added to commit but untracked files present (use "git add" to track)
```

**怎么读**：

| 英文 | 中文含义 |
|---|---|
| `Untracked files` | **未跟踪**：Git 不认识这些文件，你也没说要管 |
| `Changes not staged for commit` | **已改动但未加入暂存区**：改过了，还没 `add` |
| `Changes to be committed` | **已加入暂存区**：`add` 过了，等 `commit` |
| `nothing to commit, working tree clean` | **干净**：没有未存档的改动 |

**习惯**：**每做一步都先 `git status`。** 这是 Git 里最值得养成的习惯。

---

## 五、`git add`：把文件放进暂存区

### 加一个特定文件

```powershell
git add CMakeLists.txt
```

### 加整个目录

```powershell
git add src/
```

### 加所有改动（最常用）

```powershell
git add .
```

**注意 `.` 前面的空格不能省**，`.` 表示"当前目录及以下"。

### 加完之后

```powershell
git status
```

```
Changes to be committed:
  (use "git restore --staged <file>..." to unstage)
        new file:   .gitignore
        new file:   CMakeLists.txt
        new file:   README.md
        ...
```

**这时候还没存档！** 只是"准备好了"。

### 如果 add 错了

```powershell
git restore --staged CMakeLists.txt
```

`--staged` 的意思是"只从暂存区里拿出来"，
**文件本身不受影响**，只是回到"未暂存"状态。

> 老教程里的 `git reset HEAD 文件名` 效果一样，`restore --staged` 是新写法。

---

## 六、`git commit`：真正存一次档

### 标准写法

```powershell
git commit -m "初始化项目：CMake 骨架 + 数据层 + 7 个算法模块"
```

**`-m` 后面那句是"这次改了什么"**，必须要写，而且要写人话。

### 好的提交信息 vs 坏的

| 坏的 | 好的 |
|---|---|
| `update` | `修改体素降采样默认 leaf 从 1.0 到 2.0` |
| `fix` | `修正 RANSAC 默认迭代次数 1000→5000，避免抽不到平板` |
| `123` | `新增 PassThrough 裁剪模块，含 4 个参数` |
| `asdf` | `补 docs/08 体素降采样文档的实测数据` |

**判断标准：半年后你自己看到这句话，能想起来当时为什么改。**

### 这个仓库里就有一条写错的提交信息（真实案例）

**你运行 `git log --oneline` 会看到**：

```
339ef75 补齐 docs/01/07/08/09/10 五份文档，文档共 16 份齐全
cacc079 加入 16 份中文教学文档          ← 这条写错了
b4ad66a 加入合成场景原始点云 scene_original.pcd（30251 点）
ef82936 加入程序源码：数据层/预处理/搜索/裁剪/分割/菜单 + 练习区
8f4b5d7 初始化项目：CMake 构建骨架与 README
```

**`cacc079` 说"16 份"，但那次实际只提交了 11 份** ——
写提交信息的时候**凭印象写**，没有先 `git status` 数一数。

**这正好是一个反面教材。** 三个教训：

1. **提交前看一眼 `git status --short`**，确认要提交的到底是哪些文件
2. **提交信息里的数字要能对上** —— "16 份"就要真的 16 个文件
3. **写错了要不要改历史？** 见下一节

### 提交信息写错了怎么办

**三条路，代价递增：**

| 情况 | 做法 | 风险 |
|---|---|---|
| **还没提交**（刚 `git add`） | 直接重新 `git commit -m "对的信息"` | 无 |
| **刚提交，是最后一个** | `git commit --amend -m "对的信息"` | 无（但会改 commit hash） |
| **已经提交了几次** | `git rebase -i <之前的提交>` 改那一条 | **中** —— 会重写之后所有提交的 hash |
| **已经推到远程了** | 别改 | **高** —— 别人手里的历史和你不一样 |

**本仓库选择的是"不改历史"** ——
因为 cacc079 之后已经有了新提交，`rebase -i` 会重写它，
对一个教学仓库来说不值得。

**正确做法是像本仓库这样：留着它，然后在后面的提交里补齐，并在文档里记一笔。**
**这比假装历史完美更有教学价值。**

**记住一个原则**：

> **提交信息可以写得不完美，但不能写"假的"。**
> 写"补文档"比写"补 16 份文档"然后实际只补 5 份要诚实得多。

### 中文提交信息会不会乱码

Windows 上 `git log` 显示中文时可能变成 `\344\277\256\346\224\271` 这种八进制。

修一下（只影响显示，不改文件）：

```powershell
git config --global core.quotepath false
```

再设一下终端编码（本项目的 `setupConsole()` 已经帮程序做了，但 Git 本身要单独设）：

```powershell
$env:LANG = "zh_CN.UTF-8"
```

### 本项目建议的提交节奏

**不要攒一大堆再提交。** 一次提交 = 一件说得清的事。

按本项目的开发顺序，理想的提交历史大概长这样：

```
1. 初始化项目：CMake 骨架 + core 层（console / cloud_io / types）
2. 新增数据层：场景生成器 + 5 个预设 + 点云检查
3. 新增菜单框架与 3 个数据项
4. 新增体素降采样与 RANSAC 平面分割
5. 补齐 SOR / ROR / KD-Tree / PassThrough / 欧氏聚类
6. 修正中文表格对齐与算法输出重复打印
7. 新增写保护验证与回归脚本
8. 补 README、.gitignore 与 docs 00-05
9. 补 docs 01/07-10
10. 补 docs 06/11-15
```

**每次提交都是一个"能编译通过"的状态。** 这是好习惯：
任何时候 `git checkout` 到某个提交，代码都是能跑的。

---

## 七、`git log`：看历史

```powershell
git log
```

```
commit 3f2a1b9c8e7d6f5a4b3c2d1e0f9a8b7c6d5e4f3a (HEAD -> master)
Author: 你的名字 <you@example.com>
Date:   Fri Oct 24 15:30:12 2026 +0800

    补齐 SOR / ROR / KD-Tree / PassThrough / 欧氏聚类

commit 8a7b6c5d4e3f2a1b0c9d8e7f6a5b4c3d2e1f0a9b
Author: 你的名字 <you@example.com>
Date:   Fri Oct 24 14:10:05 2026 +0800

    新增体素降采样与 RANSAC 平面分割
```

### 常用的三种简化

```powershell
# 一行一条，看得快
git log --oneline

# 带分支图（以后有用）
git log --oneline --graph

# 只看最近 5 条
git log --oneline -5
```

`git log --oneline` 的输出长这样：

```
3f2a1b9 补齐 SOR / ROR / KD-Tree / PassThrough / 欧氏聚类
8a7b6c5 新增体素降采样与 RANSAC 平面分割
1d4e7f2 新增菜单框架与 3 个数据项
9c0b3a1 新增数据层：场景生成器 + 5 个预设 + 点云检查
4e8d2c6 初始化项目：CMake 骨架 + core 层
```

**每行开头那 7 个字符是"提交编号"（commit hash，哈希）。**
后面所有"回到某次提交"都靠它。

---

## 八、`git diff`：看看到底改了什么

这是**最被低估的命令**。当程序"昨天还能跑，今天不行了"，`git diff` 能直接告诉你改了什么。

### 看还没 `add` 的改动

```powershell
git diff
```

输出大概长这样：

```diff
diff --git a/src/preprocessing/voxel_filter.cpp b/src/preprocessing/voxel_filter.cpp
index 1a2b3c4..5d6e7f8 100644
--- a/src/preprocessing/voxel_filter.cpp
+++ b/src/preprocessing/voxel_filter.cpp
@@ -18,7 +18,7 @@ AlgoResult runVoxelGrid(const CloudPtr &cloud, const VoxelOptions &opt)
     VoxelOptions safeOpt = opt;
-    if (safeOpt.leaf <= 0.0)
+    if (safeOpt.leaf < 0.0)
     {
         safeOpt.leaf = 1.0;
     }
```

**怎么读**：

| 符号 | 含义 |
|---|---|
| `---` 开头 | 旧版本 |
| `+++` 开头 | 新版本 |
| `-` 开头的行 | **被删掉/改掉的行**（旧内容） |
| `+` 开头的行 | **新增/改成这样的行**（新内容） |
| `@@ -18,7 +18,7 @@` | 旧文件从第 18 行起 7 行，新文件从第 18 行起 7 行 |

上面这个 diff 说的是：**有人把 `<= 0` 改成了 `< 0`**。
这个改动会让 `leaf = 0` 不再被拦截（体素边长 0 会导致除零）——
**一行 `git diff` 就找出了 bug。**

### 看已 `add` 但还没提交的改动

```powershell
git diff --staged
```

### 看某两个版本之间改了什么

```powershell
# 比较 commit 8a7b6c5 和它的上一个版本
git diff 8a7b6c5^ 8a7b6c5
```

`^` 表示"上一个"。这一句等价于：

```powershell
git show 8a7b6c5
```

### 只看某个文件的历史改动

```powershell
git diff HEAD -- src/segmentation/ransac_plane.cpp
```

`HEAD` = 当前最后那次提交。

**实用场景**：你改了一堆文件，怀疑是 `ransac_plane.cpp` 出问题：

```powershell
git diff HEAD -- src/segmentation/ransac_plane.cpp
```

只看这一个，眼睛就不会被别的改动干扰。

---

## 九、`.gitignore`：哪些文件不存档

### 为什么需要它

本项目里有些文件**不该进版本库**：

| 文件 | 为什么不存 |
|---|---|
| `build/` 下所有东西 | 由 CMake 生成，随时能再生；`.obj`/`.exe` 都是二进制、体积大 |
| `.vs/` | Visual Studio 的个人配置（窗口布局、断点位置），和代码无关 |
| `output/*.pcd` | 算法每次跑都重新生成，存了会不断膨胀 |
| `data/*.bak` | 临时备份 |
| `*.pdb` / `*.ilk` | 调试用的中间文件，几百 MB |

### 本项目的 `.gitignore`（已创建）

打开 `E:\dianyunxulian\.gitignore` 看实际内容。核心部分是：

```gitignore
# CMake 生成目录
build/
cmake-build-*/

# Visual Studio
.vs/
*.sln
*.vcxproj*

# 编译产物
*.obj
*.exe
*.dll
*.lib
*.pdb
*.ilk

# 输出结果（每次运行都会重新生成）
output/*.pcd

# 临时文件
_tmp*/
_env_probe/
```

### 语法说明

| 写法 | 含义 |
|---|---|
| `build/` | 匹配**任何层级**下叫 `build` 的目录 |
| `*.obj` | 匹配所有 `.obj` 文件 |
| `output/*.pcd` | 匹配 `output/` 下所有 pcd |
| `!important.pcd` | **例外**，不忽略（前面必须有被忽略的规则） |
| `#` 开头 | 注释 |

**顺序很重要**：`!` 例外规则必须写在**忽略规则之后**：

```gitignore
output/*.pcd
!output/.gitkeep     # 但保留这个占位文件，让 output/ 目录本身能被提交
```

### 一个容易忽略的点：目录本身

Git **不追踪空目录**。`output/` 里如果没有任何被跟踪的文件，这个目录不会出现在仓库里。
所以别人 clone 下来会少一个目录，程序第一次运行可能失败。

**解决办法**：在空目录里放一个空文件（比如 `.gitkeep`），并在 `.gitignore` 里
用 `!` 放行它。

---

## 十、`data/scene_original.pcd` 要不要进版本库

这是一个很实际的问题。

### 情况 A：合成数据（本项目的情况）

| 事实 | 数字 |
|---|---|
| 文件大小 | 约 355 KB（binary PCD，30251 点 × 16 字节 + 头部） |
| 能否重新生成 | **能**，菜单 1 一键生成 |
| 生成是否确定 | 固定种子，**同一台机器上完全一致** |

**结论：可以进版本库。** 理由：

1. 355 KB 不算大（一张手机照片都 2-5 MB）
2. 它是**全项目唯一的参照物** —— 所有文档里的数字（17847、196、459、8323、2213、4131）
   都是在这份数据上测出来的。如果它被换了，文档就对不上了
3. 新机器上 clone 下来直接就能跑菜单 2-11，不用先"生成"

### 情况 B：真实扫描数据（以后的情况）

如果换成线激光扫的真实 PCD：

| 事实 | 数字 |
|---|---|
| 单帧大小 | 几十 MB 到几百 MB 常见 |
| 能否重新生成 | **不能**，扫一次要钱要时间 |
| 是否需要备份 | **需要**，但要备份到别处 |

**结论：不进 Git，用别的方式管理。**

原因：Git 存的是**每一次改动的完整快照**，不是增量。
一个 100 MB 的文件改 10 次，仓库就 1 GB 了（Git 会做压缩，但依然很大）。

**推荐的替代做法**：

| 做法 | 适合 |
|---|---|
| 网盘（OneDrive / 百度网盘） | 个人数据，简单 |
| 移动硬盘 + 定期拷贝 | 数据量大、不想上网 |
| Git LFS | 需要和代码一起版本管理时（本项目用不上） |
| 只提交**一个极小的样本** | 比如从大文件里抽 1 万点存成 `data/sample_small.pcd` |

**判断标准（经验值）**：

- **< 10 MB**：进 Git，省事
- **10 – 100 MB**：犹豫。能再生就不进，不能再生就考虑 LFS
- **> 100 MB**：不进 Git，用网盘/硬盘

---

## 十一、恢复误改的文件

这是 Git 最有价值的功能。

### 场景 1：改了文件还没 add，想撤销

```powershell
# 先看看改了什么（很重要！别不看就撤销）
git diff src/segmentation/ransac_plane.cpp

# 确认要撤销，再执行
git restore src/segmentation/ransac_plane.cpp
```

**警告**：`git restore` **会丢掉你的修改，而且无法恢复**。
**永远先 `git diff` 看一眼。**

### 场景 2：add 了还没 commit，想撤销

```powershell
# 只从暂存区退出来，文件内容不变
git restore --staged src/segmentation/ransac_plane.cpp

# 再撤销文件内容（如果需要）
git restore src/segmentation/ransac_plane.cpp
```

### 场景 3：commit 了，想把文件恢复成上一次提交的样子

```powershell
git restore --source=HEAD -- src/segmentation/ransac_plane.cpp
```

`--source=HEAD` 明确说"从最后一次提交里取"。

### 场景 4：误删了文件

```powershell
git restore src/preprocessing/voxel_filter.cpp
```

**只要它曾经被 commit 过，就永远能找回来。** 这就是"存档"的意义。

### 场景 5：整个项目搞乱了，想回到某次提交

```powershell
# 第一步：看历史，找到想回去的那个哈希
git log --oneline

# 第二步：先看一眼那个版本长什么样（不改动任何东西）
git show 8a7b6c5:src/main.cpp

# 第三步：确认无误，整体回到那个版本
git reset --hard 8a7b6c5
```

**`--hard` 是危险命令**：它会把工作区**所有未提交的改动全部丢弃**。

**安全用法**：

```powershell
# 先存一份当前状态（万一回错了还能回来）
git stash

# 或者先提交一次
git commit -am "临时保存：准备回退"

# 然后再 reset
git reset --hard 8a7b6c5
```

### `reset` 和 `revert` 的区别

| 命令 | 效果 | 会用在哪 |
|---|---|---|
| `git reset --hard <hash>` | **删掉** hash 之后的提交，历史被改写 | 只有你自己、还没上传时 |
| `git revert <hash>` | **新增一次提交**，内容是"撤销 hash 那次改动" | 已经共享给别人时 |

**本项目只用本地，`reset` 够用。** 但要知道 `reset` 会真正抹掉历史，
如果抹错了，得用 `git reflog` 抢救（见下一节）。

---

## 十二、救命命令：`git reflog`

如果你 `reset --hard` 回错了，或者误删了分支，**Git 还留着记录**。

```powershell
git reflog
```

```
3f2a1b9 HEAD@{0}: reset: moving to 8a7b6c5
8a7b6c5 HEAD@{1}: commit: 新增体素降采样与 RANSAC 平面分割
1d4e7f2 HEAD@{2}: commit: 新增菜单框架与 3 个数据项
```

**每一行都是一次"HEAD 移动"的记录**，包括 reset 这种"破坏性"操作。
所以你能看到 `3f2a1b9` 就是 reset 之前的状态。

救回来：

```powershell
git reset --hard 3f2a1b9
```

**`reflog` 默认保留 90 天。** 所以"我误删了"这件事，
在三个月内基本都能救回来。

---

## 十三、只用本地，不上传

### 你完全不需要 GitHub

本项目的使用方式：

```powershell
git init
git add .
git commit -m "初始化"
# 以后每次改完
git add .
git commit -m "改了什么"
```

**从头到尾没有一条联网命令。** `git push` 是本项目**禁止**执行的操作。

### 确认仓库没配远程地址

```powershell
git remote -v
```

**输出为空** = 没有配置任何远程仓库 = 不可能误传。

如果你看到有输出（说明配过了），删掉：

```powershell
git remote remove origin
```

### Git 会偷偷联网吗

**不会**（除非你执行 `push` / `pull` / `fetch` / `clone`）。
`init` / `add` / `commit` / `log` / `diff` / `status` / `restore` / `reset` 全是本地操作。

---

## 十四、本项目的建议操作流程

把上面的内容压成一张卡片，**贴在显示器边上**：

```
开始干活前：
    git status                    ← 确认工作区干净

改代码时：
    （正常改）

改完一段、能编译通过时：
    git status                    ← 看看改了哪些文件
    git diff                      ← 看看具体改了什么
    git add src/xxx.cpp           ← 只加相关的文件
    git commit -m "一句话说清改了什么"

改坏了想退回：
    git diff src/xxx.cpp          ← 先看，别急
    git restore src/xxx.cpp       ← 撤销（会丢失修改！）

完全搞乱了：
    git reflog                    ← 找到"之前正常"的哈希
    git reset --hard <哈希>        ← 整体退回
```

---

## 十五、练习

### 练习 1（10 分钟）：完成第一次提交

```powershell
cd E:\dianyunxulian
git add .
git status
git commit -m "初始化项目：CMake 骨架 + 数据层 + 7 个算法模块 + 文档"
git log --oneline
```

**验收**：`git log --oneline` 能看到一条记录；`git status` 显示 `working tree clean`。

**顺便检查**：`git status` 里**不应该**出现 `build/` 下的任何文件。
如果出现了，说明 `.gitignore` 没生效，检查文件名拼写（是 `.gitignore`，不是 `.gitignore.txt`）。

### 练习 2（10 分钟）：制造一个改动，然后用 diff 找出来

1. 打开 `src\preprocessing\voxel_filter.h`
2. 把 `double leaf = 2.0;` 改成 `double leaf = 3.0;`
3. 保存
4. 回到 PowerShell：

```powershell
git status
git diff
```

**验收**：你能在 `git diff` 的输出里找到那一行 `-` 和 `+`。

### 练习 3（5 分钟）：撤销刚才的改动

```powershell
git restore src/preprocessing/voxel_filter.h
git status
```

**验收**：`git status` 又变回 `working tree clean`；
打开文件看，`leaf` 又变回 `2.0`。

**这一步很重要**：你刚刚亲手体验了"改坏了能退回去"。
**这就是 Git 给你的自由。**

### 练习 4（15 分钟）：故意回退到上一个版本

1. 随便改一个文件（比如在 `README.md` 里加一行 `测试回退`）
2. `git add .` `git commit -m "测试提交"`
3. `git log --oneline` 记下现在的哈希，和上一个的哈希
4. 回退到上一个：

```powershell
git reset --hard <上一个哈希>
```

5. 打开 `README.md` 看，那行"测试回退"不见了

**然后体验 `reflog` 抢救**：

```powershell
git reflog
git reset --hard <刚才那个"测试提交"的哈希>
```

**验收**：`README.md` 里那行又回来了。

**这个练习的价值**：你会亲眼看到**没有什么是真正丢掉的**。
所以以后改代码不用怕。

### 练习 5（20 分钟）：看 `.gitignore` 到底拦住了什么

```powershell
# 看看 Git 眼里的所有文件（包括被忽略的）
git status --ignored
```

输出里会出现：

```
Ignored files:
        build/
        output/
```

再试试：

```powershell
# 问 Git："这个文件为什么被忽略了"
git check-ignore -v build/
git check-ignore -v output/voxel_leaf2.000.pcd
```

**验收**：能看到是哪一行规则拦住了它。

### 练习 6（15 分钟）：改掉 `.gitignore` 再恢复

1. 把 `.gitignore` 里的 `output/*.pcd` 这一行**删掉**
2. `git status` → 看 `output/` 下的 pcd 文件是不是冒出来了
3. `git add .` → **别 commit！**
4. 把 `.gitignore` 改回来
5. `git restore --staged .` → 把它们从暂存区退出来
6. `git status` → 又干净了

**验收**：你理解了 `.gitignore` 只影响"Git 要不要看"，
不影响文件本身，也不影响已经 `add` 过的文件（所以才要 `restore --staged`）。

---

## 十六、常见报错

| 报错 | 原因 | 解决 |
|---|---|---|
| `fatal: not a git repository` | 当前目录不是 Git 仓库 | `cd` 到项目根目录，或先 `git init` |
| `fatal: pathspec 'xxx' did not match any files` | `git add` 的文件路径写错了 | 用 `git status` 看真实文件名 |
| `Please tell me who you are` | 没设 `user.name` / `user.email` | 执行第二节那两条 `git config` |
| `nothing added to commit but untracked files present` | 忘了 `git add` | 先 `git add .` |
| `error: failed to push some refs` | 你执行了 `push`（本项目不该发生） | **本项目禁止 push**，删远程地址 |
| `warning: LF will be replaced by CRLF` | Windows 换行符差异 | 无害，忽略；或 `git config --global core.autocrlf true` |
| 中文显示成 `\344\277\256` | `core.quotepath` | `git config --global core.quotepath false` |

---

## 下一步

去 [07_PCL点云数据结构.md](07_PCL点云数据结构.md)。

到这里，"项目怎么搭、怎么编、怎么调、怎么存档"这条线走完了。
再往下就进入算法本身。

<p align="right">
  <strong>简体中文</strong> · <a href="CHANGELOG.md">English</a>
</p>

# Changelog

## Unreleased

- 将用户提供的封面加入 240 x 320 开机画面，按任意键退出，不误触后续菜单。
  原人物、玩法及声音默认设置不变，存档加载期间的按键不会在加载后重放。

- 按单行/双行文字的实际高度居中菜单标签，使其与图标对齐。男女头像统一高度、
  宽度及五官定位，保留男性偏分短发、方下巴和女性马尾的区别。

- 以更宽的原生像素网格重绘马尾角色，加入头发层次、饱满脸颊、椭圆眼睛、
  与眼睛分离的小微笑及更清楚的西装领口。首页取景保留高马尾顶部，
  生产素材主机测试可生成 240 x 320 的站立形象预览。

- 精修牛马歌子视觉：图标导航首页、整数倍像素人物特写、电池格、音量条、需求进度条
  与六格行动。菜单使用原创像素图标、中等字重和统一间距；新增八种状态驱动表情，
  保留参考图的人物轮廓与马尾造型。

- 牛马歌子所有页面新增 16 像素半径的黑色像素圆角遮罩，顶部状态栏与底部操作提示
  向内留出安全边距。半径为初始视觉设定，仍需实机确认与外壳的贴合程度。

- 区分饭碗/面杯及白水/咖啡/奶茶杯型，调整高马尾角色为短圆脸，
  茶水摸鱼改为显示喝茶/忙碌状态，不再显示无关的目标/位置提示。

- 修正办公坐姿，桌下可见双腿；补上倦怠点阵，并将餐盒移离面部。
  新增按键队列驱动的完整职业旅程，不直接填入进度即可到达全部路线、物品和纪念。

- 新增早晨概览，显示周次/星期、恢复后状态和周末提示。
  八种纪念各有故事、解锁条件、日期和画面；纪念册标识收藏状态，生活里程碑后仍可继续。

- 金币不足时可进入免费餐或休养确认；行动耗尽时可直接结算/睡觉或查看资料。
  失败操作不扣资源。长按手势统一为两秒，并通过编译检查防止旧配置生效。

- 加入听音乐休息的确认与原创旋律播放，不改动全局背景音乐设置；新增实时学习笔记。
  加班确认现在默认选中取消。

- 加入六种风格图鉴，标识当前风格、说明形成方式，支持免费循环浏览。
  离职后展示出发画面，可回家休息或查看公司；重新入职清除过期确认页面的返回历史。

- 牛马歌子的长菜单现在在标题旁显示页码，选择项支持跨页和首尾循环。

- 加入开发中的牛马歌子应用，包含像素角色、照顾行动、六个小游戏、背包、
  事件、声音和双语界面。工资结算后进入居家晚间，睡觉才推进日期。
  新增五种有持续效果和首次进入日期的职业去向、员工工牌，
  以及全部 12 件物品的价格和不改变装备的试装预览。
  音量/亮度预览支持取消且不影响已保存设置；存档状态实时更新，提供失败重试提示。
  小游戏新增开始前说明、不扣行动的取消和仅做基础行动选项；摸鱼危险前有预警，
  行动预览反映实际数值上限、咖啡和职业修正。
  存档初始化重试会重新读取有效槽和版本号，避免保存成功后重启又恢复旧进度；
  实际存档任务的故障注入测试已纳入静态验证。
  开机读取失败提供只读重试和确认重置；创建角色前不会自动保存默认角色，
  读取期间的按键不会在恢复后重放。
  电量移至右上角，15% 及以下突出提醒，读取失败保持未知；设备状态页显示
  音频就绪状态并提供测试音，不会因外设异常中断游戏。
  小游戏确定键区分短按操作和长按暂停，长按不再消耗尝试次数；新增全选项布局/
  字形检查，以及中英文牛马歌子玩家指南。
  日常事件显示实际状态、技能与关系变化；事件提升关系也能解锁带日期的朋友成就。
  已完成事件不再重复提供选项，加班/合作未满足条件时显示具体原因。
  会议小游戏新增六类可阅读议题、三种回应、八秒回合、不同成长效果、建议回应和
  单次奖励结算，替换原有的标记目标反应玩法。
  下班冲刺改为通过限时电梯或楼梯下三层，包含可绕开的临时会议、按下楼进度
  计币和提前出门奖励；会议与路线提交后不再改变选中项。
  新增擦桌整理、治疗、独立隔间/洗手、饮水和做饭动画；咖啡奶茶改用杯子，
  不再显示便当。散步交替迈步，行李仅保留在离职场景。
  静态验证覆盖所有行动画面映射、四色输出、绘图边界与确定性、角色变体和动画变化。
  基础餐和便当不再压过其他养成风格，主动做饭仍计入生活派。新增不直接修改进度
  数值的自然行动通关测试，覆盖五种职业去向、12 件商品、八项带日期成就、倦怠恢复
  和六种可逆风格，全程验证存档往返。
  修复已复现的存档状态竞态：新请求进入队列前，旧写入完成不会再清掉待保存标记。
  请求编号覆盖重复版本、计数回绕、失败和写入期间提交；仍有待保存请求时禁止只读重载。
  睡觉后进入倦怠或恢复时显示温和提示，可直接转去休养或社交；继续倦怠存档也会
  显示提醒。打开提示不消耗行动，实际控制器自然游玩测试覆盖两种状态变化。
  行动预览和结果显示实际技能、经验、行动消耗与关系变化，包括数值上限处的增益。
  首页显示全部五项需求；周末首项改为散步，待业角色可以选择职业去向。
  谈薪显示等待天数、准备条件和实际涨薪前后日薪/职级；薪资到顶不再报告虚假的
  涨薪成功，弹性公司的收入在职级到顶后仍随日薪提升。
  开发版存档更新为 NMA4（244 字节），不迁移旧开发版存档。
  完整玩法验收和设备实测仍待完成。

- 加入厂家为优特利 520mAh 电芯生成的 80 字节 CW2017 profile，并实现内容与更新标志检查、写入后校验、规定的重启时序以及有上限的 SOC 就绪等待。

- 按功能域整理文档并采用双入口：根目录 `AGENTS.md` 变为薄路由（只保留硬约束与任务路由），详细的 AI 开发工作流下沉到 `docs/development/ai-guide.md`，`agent-guide.md` 并入其中。为 `docs/development/` 增加二级分区（`engineering/`、`ci/`、`release/`），把 `plays/` 应用档案与 `experiences/` 移入带专属 README 的 `docs/reference/` 参考区；删除 `docs/software-design/`（空脚手架）；把 `assets/{fonts,images,music}/README` 三个叶子 README 并入 `assets/` README；把 `project-completion` 的六个子文档压平为单文件；并把每个目录统一为单一 README，消除所有 `INDEX` 文件与一处重复经验索引。所有交叉引用与文献链接已更新；未丢弃任何内容。

- 保留固定的 `cardid`/Recovery 保护分区，以及 CI 对合并镜像结构、
  分区表 MD5/范围、3 MB 应用上限和保护分区数据不入包的校验；
  移除功能键持续 5 秒进入 Recovery 的 bootloader hook。
- 规定多应用发布的 Release 标题约定：tag 按 `v<版本>-<应用名>`（如 `v0.1.0-voice-keychain`）命名，让 Release 标题同时带版本与应用名；发布成功后核对标题，保证一眼扫 Release 列表就能区分是哪个应用。
- 新增发布后收尾流程：`issue-suggestions` skill 用于把用户反馈作为 issue 提交到上游项目；`experience-pr` skill 用于把可复用的开发经验作为文档 PR 提交；新增 `docs/experiences/` 目录保存单条经验文件；并配套 `project-completion`、`file-issues` 与经验索引文档。
- 精简仓库根目录：将 GitHub 可识别的社区治理文档迁入 `.github/`，将变更记录迁入 `docs/`，同步全部引用，并在仓库检查中加入根目录文档白名单。
- 全仓库文档语言规范：所有维护中的 Markdown 默认 `.md` 文件使用英文，简体中文使用配对的 `.zh_CN.md`，双方提供语言切换；静态检查会阻止缺失配对、缺失切换链接或英文默认页混入中文正文。
- AI 开发流程一期：精简按任务加载的上下文入口，统一本地/CI 验证脚本，新增 PR 自动构建与模板，并提交依赖锁文件以提高构建可复现性。
- PR 审查修复：GitHub Actions 固定到完整 commit SHA，构建与发布 job 按最小权限拆分，同步 checkout 关闭凭证持久化；补充 Feature Request / Usage Question issue 表单；启用并修正私密安全报告兜底说明；清理 README 路径、CI 触发条件与历史分支描述漂移。
- 语言规范变更：commit 标题、PR 标题与 body 由"默认中文"改为**使用英文**（`docs/contribution/commit-and-pr.md` 更新）；中文写作规范（全角标点）适用范围剔除 PR/MR 描述（`doc-conventions.md` 更新）。
- CI 构建改造：`build-firmware.yml` 显式传入 `SDKCONFIG_DEFAULTS=sdkconfig.defaults` 再 `idf.py build`，由 defaults 启用自定义分区表（`CONFIG_PARTITION_TABLE_CUSTOM=y`，文件名为 `partitions.csv`）；`CONFIG_ESPTOOLPY_HEADER_FLASHSIZE_UPDATE` 改为 `n`，再用 `idf.py merge-bin -o build/FoloToy-AI-Passport-full.bin` 合并可直刷完整固件；产物精简为仅 full.bin；`actions/cache` 升级到 v5 以消除 GitHub Actions Node.js 20 弃用警告；CI 文档同步更新。
- 合并上游 PR #6（wireless-low-power-demos）以解决 PR #4 冲突：引入无线/低功耗 demo（`main/demo_wifi.c`、`demo_ble.c`、`demo_radio.c`、`demo_low_power.c`）、`partitions.csv`（NVS/PHY/3 MB factory-app 分区）、`main/CMakeLists.txt`/`main.c`/`demo.h`/`sdkconfig.defaults` 更新；同步硬件指南的 Wi-Fi/BLE/低功耗章节；README 能力契约表补充 Wi-Fi/Bluetooth LE/Low power 三项（中英双语）。
- 提交规范补充：`docs/contribution/commit-and-pr.md` 明确 PR 标题与 commit 标题使用相同的 Conventional Commit 格式和英文祈使句，不用名词短语当标题。
- CI 与文档清理：`sync-main.yml` 移除 `test_mode` 残留模板注释；`docs/development/coding-conventions.md` 将「Redis TTL」条目泛化为「缓存组件」条目（当前固件无 TTL 约束需求，消除从模板带入的无关约定）。
- 补充通用规范（借鉴 Shinku）：`docs/contribution/doc-conventions.md` 新增中文全角标点规范（正文 `，`；`（`）`，代码/命令/路径保留英文原样）、凭证不入仓规范（token/密钥/私钥绝不入仓，提交前 git diff 扫描敏感前缀）、文件删除安全规范（删除走系统回收站，不用 rm -rf/git clean -fd）。
- 代码注释规范强化：`docs/development/coding-conventions.md` 补充完善注释要求——函数说明（用途/参数/返回值/副作用/线程上下文/内存所有权/初始化顺序）、变量说明（语义/取值范围/生命周期/同步要求）、逻辑注释（状态机/时序/寄存器/魔数依据），覆盖范围宁多勿少，中文注释保留英文技术术语。
- 文档去 AI 化：`docs/README.md` / `docs/README.zh_CN.md` 移除 AI 专属章节（Entry point、Source-of-truth、提需求格式、BSP 边界、Runtime invariants、验收交付格式、构建命令），README 只保留给人看的项目介绍、硬件能力契约、demo 案例与项目结构；构建命令章节删除（与 `docs/development/build-and-test.md` 重复）。
- 新增 `docs/development/agent-guide.md`：集中承载"AI 如何在本仓库工作"（上下文建立顺序、事实来源优先级、提需求格式、BSP 边界、运行时规则、交付格式），并链接 build-and-test 与硬件指南，不重复构建命令与验收矩阵。
- 同步更新索引：`AGENTS.md` 规则索引新增 agent-guide 条目；`docs/INDEX.md` 与 `docs/development/README.md` 新增 agent-guide 索引行。
- 文档补充：`docs/fork-guide.md` 说明「为什么根目录不放置 README」——根目录 README 预留给 fork 开发者自行放置（上游留空），fork 后可将自己的内容写入根目录 `README.md` 介绍 fork 后的项目；GitHub 显示优先级（根 README > docs/README.md）契合该预留意图。
- 分支合并：创建 `main-update` 分支（基于与上游一致的 main），将 `feature/repo-structure`、`ci/build-firmware`、`ci/sync-main` 三个分支合并进来，统一 docs 结构（CI 文档归入 `docs/development/`，workflow 文件随 ci 分支引入 `.github/workflows/`）；解决 development/software-design README 的 add/add 冲突。
- 合并后审查修复：`docs/INDEX.md` 补充 CI 文档索引；`docs/fork-guide.md` 修正 workflow 引用为 `.github/workflows/sync-main.yml`；`docs/README` 双语项目结构块补充 `.github/workflows/` 与 CI 文档说明。
- ci 分支 CI 文档路径调整：`ci/build-firmware` 的 `docs/software-design/CI-build-and-release.md` 与 `ci/sync-main` 的 `docs/software-design/CI-sync-main.md` 均移入各分支的 `docs/development/`（CI 属工程规范）；`docs/software-design/README.md` 保留为软件设计索引；feature 分支的 software-design 索引同步更新引用。
- fork 补充文档目录迁移：`assets/docs/` 移至 `docs/assets/`（文档素材归入 docs/ 更合理），新增 `docs/assets/.gitkeep` 空目录占位；同步更新 AGENTS.md / INDEX / doc-conventions / fork-guide 的路径引用。
- 文档结构调整：根目录不再放 README——上游英文 README 移入 `docs/README.md`、中文移入 `docs/README.zh_CN.md`（GitHub 从 docs/ 识别主 README）；原 `docs/README.md` 根总索引更名为 `docs/INDEX.md`；同步更新 AGENTS.md / CONTRIBUTING / SUPPORT / fork-guide / doc-conventions 的路径引用。
- 初始化项目文档：新增 `AGENTS.md`、`CLAUDE.md` 和 `CHANGELOG.md`。
- 仓库结构规整：上游英文 `README.md` 更名为 `README.en_US.md`，保留 `README.zh_CN.md`。
- 新增目录骨架：`docs/`（software-design / hardware-design）、`assets/`（fonts / images / music，各含 `README.md`）、`skills/`。
- 将上游硬件开发指南归位到 `docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.md`。
- 文档规范：子目录 readme 统一为大写 `README.md`；补充 fork 用户约定（main 只动根 README）。
- 扩展 fork 用户约定：`main` 分支允许修改根目录 `README.md` 和 `assets/docs/`（README 不足以说明项目时存放补充文档与素材）。
- 新增 `assets/docs/` 目录约定：上游 main 只保留空目录 `.gitkeep`，内容文件仅存在于 fork；使用方法规范写入 AGENTS.md「给 fork 用户」约定。
- CI 文档迁移：`docs/software-design/CI.md` 从本分支移除，迁至 `ci/build-firmware` 分支并改名为 `docs/software-design/CI-build-and-release.md`。
- 补充 `main` 分支策略说明：解释 `main` 保持干净的两大原因（与上游同步无冲突 + 多小项目按分支整理）；例外——执意 main 开发需停用 CI 自动同步；提醒 fork 用户默认 action 关闭需手动启用（此条为整个 CI 的通用要求，统一写入 AGENTS.md）。
- 文档拆分：将 `AGENTS.md` 按主题拆为公共文档——新增 `docs/contribution/`（doc-conventions.md、commit-and-pr.md）与 `docs/development/`（build-and-test.md、coding-conventions.md），新增 `docs/fork-guide.md`；`AGENTS.md` 精简为简介 + 项目概述 + 必读文档索引。
- 同步更新索引：`docs/software-design/README.md`、`README.en_US.md` / `README.zh_CN.md` 的 `docs/` 目录说明。
- 参考 cindy 仓库文档组织完善索引：新增 `docs/README.md` 根总索引；AGENTS.md 规则索引按触发场景改写（附触发条件）；`docs/contribution/` 与 `docs/development/` 的 README 补充收录标准。
- 引入社区治理文档（参照 cindy 改写，放仓库根目录）：新增 `CONTRIBUTING.md` / `.zh_CN.md`（贡献指南，针对 ESP-IDF/AI agent/fork 场景改写）、`CODE_OF_CONDUCT.md` / `.zh_CN.md`（贡献者公约）、`SECURITY.md` / `.zh_CN.md`（安全报告流程）、`SUPPORT.md` / `.zh_CN.md`（支持渠道）；AGENTS.md 与 docs/README.md 同步引用。

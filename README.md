## HotPatcher
**重要通知：作者没有任何平台和渠道录制收费课程，也不对任何第三方的商业行为背书。请擦亮双眼，谨防上当受骗。**

**本软件的开源协议：允许在商业项目中免费使用功能，但不允许任何第三方基于该插件进行任何形式的二次收费，包括但不限于录制收费课程、对插件及代码的二次分发等。**

English Document:[README_EN.md](https://github.com/hxhb/HotPatcher/blob/master/README_EN.md)

[HotPatcher](https://github.com/hxhb/HotPatcher)是虚幻引擎中涵盖了热更新、构建提升、包体优化、资源审计等全方面流程的资源管理框架。

在热更新领域，HotPatcher可以管理热更版本和资源打包，能够追踪工程版本的原始资源变动。支持资源版本管理、版本间的差异对比和打包、支持导出多平台的基础包信息、方便地Cook和打包多平台的Patch，并提供了数种开箱即用的包体优化方案，支持迭代打包、丰富的配置化选项，全功能的commandlet支持，可以非常方便地与ci/cd平台相集成。全平台、跨引擎版本（UE4.21 ~ UE5）支持。

>目前支持的引擎版本为UE4.21-UE5最新版本(5.3.2)，我创建了个群来讨论UE热更新和HotPatcher插件的问题(QQ群958363331)。

插件文档：[UE资源热更打包工具HotPatcher](https://imzlp.com/posts/17590/)

视频教程：[UE4热更新：HotPatcher插件使用教程](https://www.bilibili.com/video/BV1Tz4y197tR/)

我的UOD热更新主题演讲：
- [Unreal Open Day2022 UE热更新的原理与实现 | 查利鹏](https://www.bilibili.com/video/BV1d841187Pt/?zw)
- [Unreal Open Day2020 虚幻引擎4全平台热更新方案 | 查利鹏](https://www.bilibili.com/video/BV1ir4y1c76g)

我写的UE热更新和资源管理的系列文章，可以作为工程实践的参考：

- [UE热更新：需求分析与方案设计](https://imzlp.com/posts/17371)
- [UE资源热更打包工具HotPatcher](https://imzlp.com/posts/17590/)
- [UE热更新：基于HotPatcher的自动化流程](https://imzlp.com/posts/10938/)
- [2020 Unreal Open Day](https://imzlp.com/posts/11043/)
- [UE热更新：拆分基础包](https://imzlp.com/posts/13765/)
- [UE热更新：资产管理与审计工具](https://imzlp.com/posts/3675)
- [UE热更新：Create Shader Patch](https://imzlp.com/posts/5867/)
- [UE热更新：Questions & Answers](https://imzlp.com/posts/16895/)
- [UE热更新：资源的二进制补丁方案](https://imzlp.com/posts/25136/)
- [UE热更新：Shader更新策略](https://imzlp.com/posts/15810/)
- [UE 资源合规检查工具 ResScannerUE](https://imzlp.com/posts/11750/)
- [UE热更新：Config的重载与应用](https://imzlp.com/posts/9028/)
- [基于ResScannerUE的资源检查自动化实践](https://imzlp.com/posts/20376/)
- [UE5：Game Feature预研](https://imzlp.com/posts/17658/)
- [UE 资源管理：引擎打包资源分析](https://imzlp.com/posts/22570/)
- [基于ZSTD字典的Shader压缩方案](https://imzlp.com/posts/24725/)
- [虚幻引擎中 Pak 的运行时重组方案](https://imzlp.com/posts/12188/)
- [一种灵活与非侵入式的基础包拆分方案](https://imzlp.com/posts/24350/)
- [HotPatcher 的模块化改造和开发规划](https://imzlp.com/posts/30178/)
- [资源管理：重塑 UE 的包拆分方案](https://imzlp.com/posts/37036/)

**基于HotPatcher的资源管理框架**

![](https://img.imzlp.com/imgs/zlp/picgo/2021/20220526194731.png)

**应用项目**

|                                               元梦之星                                                |                                                  QQ                                                   |                                          Apex Legends Mobile                                          |                                            WitiSports                                             |                                              MOSSAI                                               | 二之国：交错世界|                                                                                       
| :-----------------------------------------------------------------------------------------------: | :---------------------------------------------------------------------------------------------------: | :---------------------------------------------------------------------------------------------------: | :-----------------------------------------------------------------------------------------------: | :-----------------------------------------------------------------------------------------------: | :-----------------------------------------------------------------------------------------------: |
| <img src="https://img.imzlp.com/imgs/zlp/picgo/2023/12/15/103025.png" height="100" width="100" /> | <img src="https://img.imzlp.com/imgs/zlp/picgo/2022/202207280953994.webp" height="100" width="100" /> | <img src="https://img.imzlp.com/imgs/zlp/picgo/2022/202207280956642.webp" height="100" width="100" /> | <img src="https://img.imzlp.com/imgs/zlp/picgo/2023/12/18/122654.png" height="100" width="100" /> | <img src="https://img.imzlp.com/imgs/zlp/picgo/2023/12/18/124705.png" height="100" width="100" /> | <img src="https://img.imzlp.com/imgs/zlp/picgo/2024/02/28/103324.png" height="100" width="100" /> |

HotPatcher在大量的UE项目中使用，是目前UE社区中最流行的热更新工具。


## 捐赠
如果HotPatcher项目有帮助到你，并希望HotPatcher以后能够做的更好，可以扫描下方的捐赠二维码支持作者。

|                              微信支付                              |                     支付宝                      |
| :----------------------------------------------------------: | :----------------------------------------------------------: |
| <img src="https://img.imzlp.com/imgs/zlp/picgo/2022/wechatpay.webp" height="160" width="160" /> | <img src="https://img.imzlp.com/imgs/zlp/picgo/2022/alipay.webp" height="160" width="160" /> |

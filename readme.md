# Real-Time Rendering Engine Demo (OpenGL)

## 项目简介

本项目为独立设计与实现的实时渲染引擎 Demo，基于 OpenGL + GLSL 构建完整现代实时渲染管线，涵盖：

- 物理一致 PBR 直接光照
- 面光源高光积分（LTC）
- 改进版 PCSS 软阴影
- 屏幕空间反射（SSR）
- 基于光线追踪探针的 DDGI（静态场景）

核心算法参考工业级实现（如 Unreal Engine 4 / Unreal Engine 5 的公开资料），并使用 GLSL 重写与结构化实现，同时在阴影与半影稳定性方面进行了自主优化。

### 一、直接光照系统（物理一致 PBR + 面光源）

#### 1.1 BRDF 实现

- 基于**金属度工作流**完整实现 Cook–Torrance BRDF
- 使用 GGX 法线分布函数
- Schlick 近似菲涅尔项

构建物理一致的能量分配流程，确保漫反射与镜面反射之间的能量守恒。

#### 1.2 漫反射积分

- 使用 **Irradiance Vector（辐照度矢量）** 精确计算 Lambert 项积分
- 避免传统球面积分近似误差

#### 1.3 面光源高光（LTC）

- 使用 **LTC（Linearly Transformed Cosines）** 实现矩形面光源镜面反射积分
- 完整实现：
  - 查表数据读取
  - 矩阵重建
  - 参数映射
  - 几何项校正

该模块等价于工业级面光源高光实现方式。

#### 1.4 IBL 预计算流程

完整实现 Image-Based Lighting：

- GGX 重要性采样 Prefiltered Specular
- BRDF Integration LUT 预积分
- 环境辐照度卷积

### 二、改进版 PCSS 软阴影系统

#### 2.1 平面斜率深度偏移

- 基于光空间微分构建**平面斜率深度偏移**
- 替代传统常量偏移或偏移锥方法
- 在大核 PCF 采样下显著降低阴影痤疮与闪烁

#### 2.2 半影统计建模

Blocker Search 阶段：

- 统计遮挡样本深度的均值与方差
- 基于遮挡样本构建协方差矩阵
- 进行特征值分解

构造**各向异性椭圆 PCF 采样核**：

- 提升半影边界抗走样能力
- 提高时间稳定性
- 减少旋转采样带来的闪烁

> 本部分为自主改进设计。

### 三、屏幕空间反射（SSR）

实现基于屏幕空间的实时反射路径追踪：

- 屏幕空间光线步近
- 深度厚度检测
- Horizon 数据辅助减少无效步进

保证实时性前提下的反射质量与稳定性。

### 四、基于光线追踪的 DDGI（静态场景）

实现静态场景下的探针式全局光照系统。

#### 4.1 加速结构

- 使用 SAH 构建 BVH
- 支持三角形场景结构

#### 4.2 探针采样

- GPU 光线追踪采样场景辐射能量
- 多方向 Monte Carlo 采样
- 八面体贴图编码存储探针辐照度

#### 4.3 漏光控制

- 使用 VSM（Variance Shadow Map）减少探针漏光
- 可见性概率估计过滤错误光照

#### 4.4 当前版本

- 面向静态场景
- 离线构建一次
- 实时运行时插值使用

### 五、时空滤波系统

实现统一的时序稳定框架，用于：

- PCSS 软阴影
- SSR
- DDGI 探针插值结果

**解决问题：**

- 减少 Monte Carlo 噪声
- 抑制半影闪烁
- 降低 SSR 抖动
- 提升整体画面时间稳定性

## 运行截图

![]([./imgs/output.png](https://github.com/Furina-de-cyber/OpenGL-Renderer/blob/master/imgs/output.png))

该场景在RTX 3060 Laptop上，可以稳定166fps运行

## 构建与运行

### 环境要求

- Windows 10 / 11
- Visual Studio 2022/2026（支持 CMake）
- OpenGL 4.5+（需支持Bindless Texture扩展）
- 支持独立显卡（推荐）

### 构建步骤

1. 克隆项目：

```bash
git clone ...
```

1. 使用 Visual Studio 打开项目根目录下的 `CMakeLists.txt`
2. 等待 CMake 自动生成缓存
3. 选择 `Release` 或 `Debug` 模式
4. 直接运行主程序目标

### 资源说明

- 模型与 HDR 贴图位于 `assets/` 目录

## TODO（后续优化方向）

### 动态 DDGI

- 引入时序更新策略
- 支持动态场景能量刷新

### Clustered / Tiled Lighting

- 构建基于屏幕分块的光源裁剪结构
- 提升多光源场景性能

###  Render Graph 重构

- 构建基于依赖图的 Pass 调度系统
- 优化资源生命周期管理

### GPU 加速结构构建

- 将 BVH 构建迁移至 GPU
- 支持更大规模场景

# SSH远程开发计划 - 直接访问Linux后端项目

## 目标
直接在Trae中通过SSH连接到远程Linux服务器(10.186.49.122)，浏览和编辑~/campus_social后端项目，实现前后端代码的统一管理。

## 当前环境
- **前端项目**: 本地Windows - campus-social-frontend
- **后端项目**: 远程Linux - ~/campus_social
- **服务器**: Ubuntu虚拟机 @ 10.186.49.122
- **用户名**: lzy

## 实施方案

### 方法1：Trae内置SSH功能（推荐尝试）
检查Trae是否支持以下功能：
1. SSH远程资源管理器
2. Remote-SSH扩展支持
3. 直接浏览远程文件系统

### 方法2：终端SSH会话
通过Trae的集成终端建立SSH连接：
```bash
ssh lzy@10.186.49.122
cd ~/campus_social
```

### 方法3：文件系统挂载
使用SSHFS将远程目录挂载为本地驱动器：
```bash
# Windows下可使用WinFsp + SSHFS-Win
# 或通过网络驱动器映射
```

## 操作步骤

### 步骤1：建立SSH连接
1. 打开Trae集成终端
2. 输入SSH连接命令
3. 提供认证信息（密码/密钥）

### 步骤2：浏览后端项目
连接成功后执行：
```bash
ls -la ~/campus_social        # 查看项目结构
cat ~/campus_social/README.md # 查看项目说明
find ~/campus_social -name "*.cpp" -o -name "*.h" # 查找C++源文件
```

### 步骤3：前后端协同开发
- 前端：本地编辑campus-social-frontend
- 后端：远程编辑~/campus_social
- API配置：确保前端指向正确的后端地址

### 步骤4：验证集成
1. 检查后端服务状态
2. 测试API连接
3. 验证前后端数据交互

## 技术栈分析
根据前端API配置，后端应该是：
- **语言**: C++
- **服务端口**: 8080
- **API规范**: RESTful
- **数据格式**: JSON

## 文件结构预览
连接后需要关注的关键文件：
```
~/campus_social/
├── src/              # C++源代码
├── include/          # 头文件
├── build/            # 构建文件
├── CMakeLists.txt    # CMake配置
├── README.md         # 项目说明
├── config/           # 配置文件
└── docs/             # 文档
```

## 开发工作流
1. **前端开发**: 本地修改前端代码
2. **后端开发**: SSH连接修改后端代码
3. **构建部署**: 远程构建和重启后端服务
4. **测试验证**: 前端调用后端API测试

## 注意事项
- 保持网络连接稳定
- 定期保存远程文件修改
- 注意文件权限和安全性
- 建立代码版本控制
- 监控后端服务状态
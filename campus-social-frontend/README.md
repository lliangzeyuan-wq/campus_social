# 校园多元社交匹配系统 - 前端

基于 C++ 服务端 + 响应式网页前端的校园社交匹配平台，支持学习搭子 / 考研搭子 / 兴趣交友 / 异性恋爱四大场景。

## 项目结构

```
campus-social-frontend/
├── index.html              # 登录/注册页面（入口）
├── css/
│   └── style.css           # 全局样式文件（移动端优先）
├── js/
│   ├── api.js              # API 接口封装（含模拟数据）
│   └── utils.js            # 工具函数库
├── pages/
│   ├── purpose.html        # 交友目的选择页
│   ├── home.html           # 首页
│   ├── match.html          # 智能匹配页
│   ├── profile-edit.html   # 资料完善/编辑页
│   ├── messages.html       # 消息列表页
│   ├── chat.html           # 实时聊天页
│   ├── invites.html        # 邀请管理页
│   └── profile.html        # 个人中心页
└── assets/
    └── images/             # 图片资源目录
```

## 功能特性

### 1. 用户认证
- 学号注册/登录
- 表单验证
- 本地存储登录状态

### 2. 交友目的选择
- 学习搭子
- 考研搭子
- 同性交友
- 异性恋爱

### 3. 智能匹配
- 多维度匹配算法展示
- 匹配度显示
- 筛选功能（性别、地点、时间）

### 4. 社交功能
- 发送/接收匹配邀请
- 同意/拒绝邀请
- 好友管理

### 5. 实时聊天
- 一对一文字聊天
- 消息历史记录
- 实时消息推送（轮询方式）

### 6. 个人中心
- 查看/编辑个人资料
- 查看统计数据
- 管理社交关系
- 退出登录

### 7. 移动端适配
- 响应式布局
- 触摸优化
- 安全区域适配（刘海屏）
- 底部导航栏

## 技术栈

- **HTML5**: 页面结构
- **CSS3**: 样式美化、动画、Flexbox/Grid 布局
- **JavaScript (ES6+)**: 交互逻辑、API 通信
- **LocalStorage**: 本地数据存储

## 运行方式

### 方式一：直接打开
由于使用纯前端技术，可以直接在浏览器中打开 `index.html` 文件。

### 方式二：本地服务器（推荐）
```bash
# 使用 Python
python -m http.server 8080

# 或使用 Node.js
npx serve .

# 或使用 VS Code Live Server 插件
```

然后访问 `http://localhost:8080`

### 方式三：部署到服务器
1. 将项目文件上传到 Web 服务器（Nginx/Apache）
2. 配置 Nginx 托管静态文件
3. 配置反向代理到 C++ 后端服务

## 后端接口配置

修改 `js/api.js` 中的 `API_CONFIG.baseUrl` 为实际的后端服务器地址：

```javascript
const API_CONFIG = {
    baseUrl: 'http://your-server-address:8080',  // 修改为实际地址
    timeout: 10000,
    retryTimes: 3
};
```

## API 接口列表

| 接口 | 方法 | 说明 |
|------|------|------|
| /api/login | POST | 用户登录 |
| /api/register | POST | 用户注册 |
| /api/profile | POST | 更新资料 |
| /api/matches | GET | 获取匹配列表 |
| /api/invites | POST | 发送邀请 |
| /api/invites/received | GET | 收到的邀请 |
| /api/invites/sent | GET | 发出的邀请 |
| /api/chats | GET | 聊天列表 |
| /api/messages/:id | GET | 消息记录 |
| /api/messages | POST | 发送消息 |
| /api/stats | GET | 统计数据 |

## 开发说明

### 模拟数据
当前版本包含完整的模拟数据，无需后端即可预览所有功能。
在 `js/api.js` 的 `mockResponse` 方法中可以修改模拟数据。

### 响应式设计
- 移动端优先（< 480px）
- 适配平板和桌面端
- 横竖屏适配

### 浏览器兼容性
- Chrome 60+
- Safari 12+
- Firefox 60+
- Edge 79+
- iOS Safari 12+
- Android Chrome 60+

## 部署方案

### 云服务器部署

1. **准备服务器**
   - 购买阿里云/腾讯云轻量应用服务器
   - 安装 Linux 系统（Ubuntu/CentOS）

2. **安装 Nginx**
   ```bash
   sudo apt update
   sudo apt install nginx
   ```

3. **上传代码**
   ```bash
   # 使用 scp 或 ftp 上传
   scp -r campus-social-frontend/* user@server:/var/www/html/
   ```

4. **配置 Nginx**
   ```nginx
   server {
       listen 80;
       server_name your-domain.com;
       
       root /var/www/html;
       index index.html;
       
       location / {
           try_files $uri $uri/ /index.html;
       }
       
       # 反向代理到后端
       location /api/ {
           proxy_pass http://localhost:8080;
           proxy_set_header Host $host;
           proxy_set_header X-Real-IP $remote_addr;
       }
   }
   ```

5. **重启 Nginx**
   ```bash
   sudo nginx -s reload
   ```

## 后续优化建议

1. **WebSocket**: 替换轮询为 WebSocket 实现真正的实时通信
2. **PWA**: 添加 Service Worker 支持离线访问
3. **图片上传**: 实现头像上传功能
4. **位置服务**: 集成地图 API 实现位置共享
5. **消息推送**: 添加浏览器通知功能

## 团队分工

- **前端开发**: 响应式网页、移动端适配、界面交互
- **后端开发**: C++ 服务端、匹配算法、数据库设计
- **产品设计**: 需求分析、UI 设计、用户体验优化
- **测试部署**: 功能测试、服务器配置、上线维护

## 许可证

MIT License

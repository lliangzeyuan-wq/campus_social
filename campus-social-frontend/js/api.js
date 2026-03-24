/**
 * 校园社交匹配系统 - API 接口层
 * 用于与后端 C++ 服务端通信
 */

// API 基础配置
const API_CONFIG = {
    // 开发环境使用本地服务器
    // 生产环境替换为实际的服务器地址
    baseUrl: 'http://localhost:8080',
    timeout: 10000,
    retryTimes: 3
};

// API 请求封装
const api = {
    /**
     * 基础请求方法
     */
    async request(url, options = {}) {
        // 演示模式：直接使用模拟数据
        console.log('API 请求:', url, options);
        return this.mockResponse(url, options);
    },
    
    /**
     * 模拟响应（开发阶段使用）
     */
    mockResponse(url, options) {
        console.log('使用模拟数据:', url);
        
        // 登录
        if (url === '/api/login') {
            let studentId = '2021001';
            if (options.body) {
                const body = typeof options.body === 'string' ? JSON.parse(options.body) : options.body;
                studentId = body.studentId || studentId;
            }
            return {
                success: true,
                data: {
                    id: '10001',
                    studentId: studentId,
                    name: '张三',
                    gender: 'male',
                    token: 'mock_token_' + Date.now(),
                    profileCompleted: true
                }
            };
        }
        
        // 注册
        if (url === '/api/register') {
            return {
                success: true,
                message: '注册成功'
            };
        }
        
        // 获取匹配列表
        if (url.startsWith('/api/matches')) {
            const mockUsers = [
                {
                    id: '10002',
                    name: '李四',
                    gender: 'female',
                    college: '计算机学院',
                    major: '软件工程',
                    grade: '大三',
                    matchScore: 92,
                    tags: ['图书馆', 'Java', 'Python'],
                    location: '图书馆三楼',
                    studyTime: '晚上 7-10点',
                    interests: '编程、阅读、电影'
                },
                {
                    id: '10003',
                    name: '王五',
                    gender: 'male',
                    college: '计算机学院',
                    major: '计算机科学',
                    grade: '大四',
                    matchScore: 85,
                    tags: ['教学楼', '考研', '数学'],
                    location: '教学楼A区',
                    studyTime: '下午 2-6点',
                    targetSchool: '清华大学',
                    interests: '算法、篮球、游戏'
                },
                {
                    id: '10004',
                    name: '赵六',
                    gender: 'female',
                    college: '外国语学院',
                    major: '英语',
                    grade: '大二',
                    matchScore: 78,
                    tags: ['咖啡厅', '英语', '阅读'],
                    location: '星巴克',
                    studyTime: '上午 9-12点',
                    interests: '英语、旅行、摄影'
                }
            ];
            return {
                success: true,
                data: mockUsers
            };
        }
        
        // 获取聊天列表
        if (url === '/api/chats') {
            return {
                success: true,
                data: [
                    {
                        userId: '10002',
                        name: '李四',
                        type: 'study',
                        lastMessage: '明天一起去图书馆吗？',
                        time: new Date(Date.now() - 3600000).toISOString(),
                        unread: 2
                    },
                    {
                        userId: '10003',
                        name: '王五',
                        type: 'kaoyan',
                        lastMessage: '高数题解出来了！',
                        time: new Date(Date.now() - 86400000).toISOString(),
                        unread: 0
                    }
                ]
            };
        }
        
        // 获取消息
        if (url.startsWith('/api/messages/')) {
            const userId = url.split('/').pop();
            return {
                success: true,
                data: [
                    {
                        fromId: userId,
                        content: '你好呀！看到你的资料，感觉我们很合适做学习搭子～',
                        time: new Date(Date.now() - 7200000).toISOString()
                    },
                    {
                        fromId: '10001',
                        content: '你好！我也觉得，你一般什么时候去图书馆？',
                        time: new Date(Date.now() - 7000000).toISOString()
                    },
                    {
                        fromId: userId,
                        content: '我通常晚上7点到10点，你呢？',
                        time: new Date(Date.now() - 3600000).toISOString()
                    },
                    {
                        fromId: userId,
                        content: '明天一起去图书馆吗？',
                        time: new Date(Date.now() - 3500000).toISOString()
                    }
                ]
            };
        }
        
        // 获取收到的邀请
        if (url === '/api/invites/received') {
            return {
                success: true,
                data: [
                    {
                        id: 'inv1',
                        fromId: '10005',
                        fromName: '小明',
                        fromCollege: '经济学院',
                        type: 'study',
                        message: '你好，我也在准备英语六级，可以一起复习吗？',
                        status: 'pending',
                        time: new Date(Date.now() - 172800000).toISOString()
                    }
                ]
            };
        }
        
        // 获取发出的邀请
        if (url === '/api/invites/sent') {
            return {
                success: true,
                data: [
                    {
                        id: 'inv2',
                        fromId: '10001',
                        fromName: '张三',
                        type: 'kaoyan',
                        status: 'pending',
                        time: new Date(Date.now() - 259200000).toISOString()
                    }
                ]
            };
        }
        
        // 获取待处理邀请数量
        if (url === '/api/invites/pending') {
            return {
                success: true,
                data: [
                    { id: 'inv1' }
                ]
            };
        }
        
        // 获取统计数据
        if (url === '/api/stats') {
            return {
                success: true,
                data: {
                    friends: 5,
                    matches: 12,
                    chats: 8,
                    activeDays: 15
                }
            };
        }
        
        // 获取系统消息
        if (url === '/api/system-messages') {
            return {
                success: true,
                data: [
                    {
                        id: 'sys1',
                        title: '系统维护通知',
                        content: '系统将于今晚12点进行维护，预计持续1小时。',
                        time: new Date(Date.now() - 604800000).toISOString()
                    },
                    {
                        id: 'sys2',
                        title: '新功能上线',
                        content: '新增"附近的人"功能，快去发现身边的伙伴吧！',
                        time: new Date(Date.now() - 1209600000).toISOString()
                    }
                ]
            };
        }
        
        // 获取最近动态
        if (url === '/api/activities') {
            return {
                success: true,
                data: [
                    {
                        icon: '🎉',
                        title: '与李四成为学习搭子',
                        time: '2小时前'
                    },
                    {
                        icon: '💬',
                        title: '收到一条新消息',
                        time: '5小时前'
                    }
                ]
            };
        }
        
        // 默认成功响应
        return {
            success: true,
            message: '操作成功'
        };
    },
    
    // ========== 用户认证 ==========
    
    /**
     * 用户登录
     */
    login(studentId, password) {
        return this.request('/api/login', {
            method: 'POST',
            body: { studentId, password }
        });
    },
    
    /**
     * 用户注册
     */
    register(data) {
        return this.request('/api/register', {
            method: 'POST',
            body: data
        });
    },
    
    // ========== 用户资料 ==========
    
    /**
     * 更新用户资料
     */
    updateProfile(data) {
        return this.request('/api/profile', {
            method: 'POST',
            body: data
        });
    },
    
    /**
     * 获取用户资料
     */
    getProfile(userId) {
        return this.request('/api/profile/' + userId);
    },
    
    // ========== 匹配功能 ==========
    
    /**
     * 获取匹配列表
     */
    getMatches(type, filters = {}) {
        const params = new URLSearchParams({ type, ...filters });
        return this.request('/api/matches?' + params.toString());
    },
    
    // ========== 邀请功能 ==========
    
    /**
     * 发送邀请
     */
    sendInvite(userId, type) {
        return this.request('/api/invites', {
            method: 'POST',
            body: { userId, type }
        });
    },
    
    /**
     * 处理邀请（接受/拒绝）
     */
    handleInvite(inviteId, action) {
        return this.request('/api/invites/' + inviteId, {
            method: 'PUT',
            body: { action }
        });
    },
    
    /**
     * 获取收到的邀请
     */
    getReceivedInvites() {
        return this.request('/api/invites/received');
    },
    
    /**
     * 获取发出的邀请
     */
    getSentInvites() {
        return this.request('/api/invites/sent');
    },
    
    /**
     * 获取待处理邀请
     */
    getPendingInvites() {
        return this.request('/api/invites/pending');
    },
    
    // ========== 聊天功能 ==========
    
    /**
     * 获取聊天列表
     */
    getChatList() {
        return this.request('/api/chats');
    },
    
    /**
     * 获取与某用户的消息
     */
    getMessages(userId) {
        return this.request('/api/messages/' + userId);
    },
    
    /**
     * 发送消息
     */
    sendMessage(userId, content) {
        return this.request('/api/messages', {
            method: 'POST',
            body: { userId, content }
        });
    },
    
    // ========== 统计数据 ==========
    
    /**
     * 获取统计数据
     */
    getStats() {
        return this.request('/api/stats');
    },
    
    /**
     * 获取系统消息
     */
    getSystemMessages() {
        return this.request('/api/system-messages');
    },
    
    /**
     * 获取最近动态
     */
    getActivities() {
        return this.request('/api/activities');
    }
};

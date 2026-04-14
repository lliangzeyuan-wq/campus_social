/**
 * 校园社交匹配系统 - API 接口层
 * 用于与后端 C++ 服务端通信
 */

// API 基础配置 - 核心修改：使用虚拟机IP代替127.0.0.1
const API_CONFIG = {
    baseUrl: 'http://39.108.165.213:8080',
    timeout: 10000,                        // 10秒超时，避免无限Pending
    retryTimes: 3,
    useMock: false                         // 关闭模拟数据，强制走真实后端
};

// API 请求封装 - 核心修复跨域/超时/错误捕获
const api = {
    async request(url, options = {}) {
        // 强制关闭模拟数据，确保请求走真实后端
        if (API_CONFIG.useMock) {
            console.warn('⚠️ 已强制关闭模拟数据，使用真实后端！');
            API_CONFIG.useMock = false;
        }

        const fullUrl = API_CONFIG.baseUrl + url;
        const { method = 'GET', body, headers = {} } = options;

        // 从本地存储获取登录 Token
        const token = localStorage.getItem('token');

        // 🔥 核心修复：删除 credentials: 'include'，解决CORS跨域冲突
        const fetchOptions = {
            method,
            mode: 'cors',                // 显式开启跨域
            // 已删除：credentials: 'include'（冲突根源）
            headers: {
                'Content-Type': 'application/json',
                'Accept': 'application/json',
                // 自动携带 Token 到请求头
                ...(token ? { 'Authorization': `Bearer ${token}` } : {}),
                ...headers
            }
        };

        // 处理POST/PUT请求体
        if (body && (method === 'POST' || method === 'PUT')) {
            fetchOptions.body = typeof body === 'string' ? body : JSON.stringify(body);
        }

        console.log('📡 发起后端请求:', method, fullUrl, body);

        // 重试机制 + 超时控制
        let retryCount = 0;
        while (retryCount < API_CONFIG.retryTimes) {
            try {
                const controller = new AbortController();
                const timeoutId = setTimeout(() => controller.abort(), API_CONFIG.timeout);
                fetchOptions.signal = controller.signal;

                // 发起请求
                const response = await fetch(fullUrl, fetchOptions);
                clearTimeout(timeoutId);

                // 处理非200状态码
                if (!response.ok) {
                    throw new Error(`后端响应异常: ${response.status} ${response.statusText}`);
                }

                // 解析响应数据
                const result = await response.json();
                console.log('✅ 后端响应结果:', result);
                return result;

            } catch (error) {
                retryCount++;
                console.error(`❌ 请求失败（第${retryCount}次重试）:`, error.message);

                // 超时/网络错误，重试
                if (error.name === 'AbortError' || error.message.includes('Failed to fetch')) {
                    if (retryCount >= API_CONFIG.retryTimes) {
                        alert(`❌ 请求后端失败！\n原因：${error.message}\n请检查：\n1. 后端是否在 ${API_CONFIG.baseUrl} 运行\n2. 虚拟机防火墙是否拦截8080端口\n3. 宿主机能否ping通 39.108.165.213:8080`);

                        throw new Error(`请求超时/网络错误: ${error.message}`);
                    }
                    // 重试前等待500ms
                    await new Promise(resolve => setTimeout(resolve, 500));
                } else {
                    // 非网络错误，直接抛出
                    alert(`❌ 后端接口错误：${error.message}`);
                    throw error;
                }
            }
        }
    },

    // 模拟数据（保留，但默认禁用）
    mockResponse(url, options) {
        console.log('使用模拟数据:', url);

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

        if (url === '/api/register') {
            return { success: true, message: '注册成功' };
        }

        if (url.startsWith('/api/matches')) {
            const mockUsers = [
                { id: '10002', name: '李四', gender: 'female', college: '计算机学院', major: '软件工程', grade: '大三', matchScore: 92, tags: ['图书馆', 'Java', 'Python'], location: '图书馆三楼', studyTime: '晚上 7-10点', interests: '编程、阅读、电影' },
                { id: '10003', name: '王五', gender: 'male', college: '计算机学院', major: '计算机科学', grade: '大四', matchScore: 85, tags: ['教学楼', '考研', '数学'], location: '教学楼A区', studyTime: '下午 2-6点', targetSchool: '清华大学', interests: '算法、篮球、游戏' }
            ];
            return { success: true, data: mockUsers };
        }

        if (url === '/api/chats') {
            return {
                success: true,
                data: [
                    { userId: '10002', name: '李四', type: 'study', lastMessage: '明天一起去图书馆吗？', time: new Date().toISOString(), unread: 2 },
                    { userId: '10003', name: '王五', type: 'kaoyan', lastMessage: '高数题解出来了！', time: new Date().toISOString(), unread: 0 }
                ]
            };
        }

        if (url.startsWith('/api/messages/')) {
            const userId = url.split('/').pop();
            return {
                success: true,
                data: [
                    { fromId: userId, content: '你好呀！', time: new Date().toISOString() },
                    { fromId: '10001', content: '你好！', time: new Date().toISOString() }
                ]
            };
        }

        if (url === '/api/invites/received') {
            return {
                success: true,
                data: [{ id: 'inv1', fromId: '10005', fromName: '小明', type: 'study', status: 'pending', time: new Date().toISOString() }]
            };
        }

        if (url === '/api/invites/sent') {
            return { success: true, data: [] };
        }

        if (url === '/api/stats') {
            return { success: true, data: { friends: 5, matches: 12, chats: 8 } };
        }

        return { success: true, message: '操作成功' };
    },

    // ========== 用户认证 ==========
    // 登录接口 - 增加错误提示
    async login(studentId, password) {
        try {
            return await this.request('/api/login', { method: 'POST', body: { studentId, password } });
        } catch (error) {
            console.error('登录失败:', error);
            alert('登录失败：' + error.message);
            return { success: false };
        }
    },

    // 注册接口 - 核心修复：完善参数校验+错误提示
    async register(data) {
        // 前端参数校验，避免空值请求
        const { studentId, name, gender, password } = data;
        if (!studentId || !name || !gender || !password) {
            alert('❌ 请填写完整注册信息！');
            return { success: false };
        }
        try {
            const result = await this.request('/api/register', { method: 'POST', body: data });
            // 注册成功提示
            if (result.success) {
                alert('✅ 注册成功！请登录');
            } else {
                alert('❌ 注册失败：' + result.message);
            }
            return result;
        } catch (error) {
            console.error('注册失败:', error);
            return { success: false };
        }
    },

    // ========== 用户资料 ==========
    async updateProfile(data) {
        try {
            return await this.request('/api/profile/update', { method: 'POST', body: data });
        } catch (error) {
            alert('更新资料失败：' + error.message);
            return { success: false };
        }
    },

    async getProfile(userId) {
        try {
            return await this.request('/api/profile/' + userId);
        } catch (error) {
            alert('获取资料失败：' + error.message);
            return { success: false };
        }
    },

    // ========== 匹配功能 ==========
    async getMatches(type, filters = {}) {
        try {
            const params = new URLSearchParams({ type, ...filters });
            return await this.request('/api/matches?' + params.toString());
        } catch (error) {
            alert('获取匹配列表失败：' + error.message);
            return { success: false, data: [] };
        }
    },

    // ========== 邀请功能 ==========
    async sendInvite(userId, type) {
        try {
            return await this.request('/api/invites', { method: 'POST', body: { userId, type } });
        } catch (error) {
            alert('发送邀请失败：' + error.message);
            return { success: false };
        }
    },

    async handleInvite(inviteId, action) {
        try {
            return await this.request('/api/invites/handle', {
                method: 'POST',
                body: { inviteId, action }
            });
        } catch (error) {
            alert('处理邀请失败：' + error.message);
            return { success: false };
        }
    },

    async getReceivedInvites() {
        try {
            return await this.request('/api/invites/received');
        } catch (error) {
            alert('获取收到的邀请失败：' + error.message);
            return { success: false, data: [] };
        }
    },

    async getSentInvites() {
        try {
            return await this.request('/api/invites/sent');
        } catch (error) {
            alert('获取发出的邀请失败：' + error.message);
            return { success: false, data: [] };
        }
    },

    // ========== 聊天功能 ==========
    async getChatList() {
        try {
            return await this.request('/api/chats');
        } catch (error) {
            alert('获取聊天列表失败：' + error.message);
            return { success: false, data: [] };
        }
    },

    async getMessages(userId) {
        try {
            return await this.request('/api/messages/' + userId);
        } catch (error) {
            alert('获取聊天记录失败：' + error.message);
            return { success: false, data: [] };
        }
    },

    async sendMessage(userId, content) {
        try {
            return await this.request('/api/messages', { method: 'POST', body: { userId, content } });
        } catch (error) {
            alert('发送消息失败：' + error.message);
            return { success: false };
        }
    },

    // ========== 统计数据 ==========
    async getStats() {
        try {
            return await this.request('/api/stats');
        } catch (error) {
            alert('获取统计数据失败：' + error.message);
            return { success: false, data: {} };
        }
    }
};

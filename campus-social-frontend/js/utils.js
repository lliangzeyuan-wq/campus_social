/**
 * 校园社交匹配系统 - 工具函数
 */

/**
 * 显示 Toast 提示
 */
function showToast(message, duration = 2000) {
    const toast = document.getElementById('toast');
    if (!toast) return;

    toast.textContent = message;
    toast.classList.add('show');

    setTimeout(() => {
        toast.classList.remove('show');
    }, duration);
}

/**
 * 检查登录状态
 */
function checkLogin() {
    const userInfo = localStorage.getItem('userInfo');
    if (!userInfo) {
        // 兼容相对路径/绝对路径，避免跳转失败
        const basePath = window.location.pathname.includes('pages/') ? '../index.html' : 'index.html';
        window.location.href = basePath;
        return false;
    }
    return true;
}

/**
 * 格式化时间
 */
function formatTime(time) {
    if (!time) return '';

    // 兼容时间戳/字符串格式
    let date;
    try {
        date = new Date(time);
        // 检测无效日期
        if (isNaN(date.getTime())) return '未知时间';
    } catch (e) {
        return '未知时间';
    }

    const now = new Date();
    const diff = now - date;

    // 小于1分钟
    if (diff < 60000) {
        return '刚刚';
    }

    // 小于1小时
    if (diff < 3600000) {
        return Math.floor(diff / 60000) + '分钟前';
    }

    // 小于24小时
    if (diff < 86400000) {
        return Math.floor(diff / 3600000) + '小时前';
    }

    // 小于7天
    if (diff < 604800000) {
        return Math.floor(diff / 86400000) + '天前';
    }

    // 超过7天显示日期（兼容不同浏览器格式）
    return date.toLocaleDateString('zh-CN', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit'
    });
}

/**
 * 格式化日期时间
 */
function formatDateTime(time) {
    if (!time) return '';

    // 兼容时间戳/字符串格式
    let date;
    try {
        date = new Date(time);
        if (isNaN(date.getTime())) return '未知时间';
    } catch (e) {
        return '未知时间';
    }

    return date.toLocaleString('zh-CN', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit',
        hour12: false // 强制24小时制
    });
}

/**
 * 转义 HTML 特殊字符
 */
function escapeHtml(text) {
    if (!text || typeof text !== 'string') return '';

    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

/**
 * 防抖函数
 * @param {Function} func - 执行函数
 * @param {Number} wait - 等待时间(ms)
 * @param {Boolean} immediate - 是否立即执行
 */
function debounce(func, wait = 300, immediate = false) {
    let timeout;
    return function executedFunction(...args) {
        const context = this;
        const later = () => {
            timeout = null;
            if (!immediate) func.apply(context, args);
        };
        const callNow = immediate && !timeout;
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
        if (callNow) func.apply(context, args);
    };
}

/**
 * 节流函数
 * @param {Function} func - 执行函数
 * @param {Number} limit - 节流间隔(ms)
 */
function throttle(func, limit = 200) {
    let inThrottle;
    return function executedFunction(...args) {
        const context = this;
        if (!inThrottle) {
            func.apply(context, args);
            inThrottle = true;
            setTimeout(() => inThrottle = false, limit);
        }
    };
}

/**
 * 本地存储封装
 */
const storage = {
    set(key, value) {
        try {
            // 兼容非JSON格式数据
            if (typeof value === 'undefined') return false;
            localStorage.setItem(key, JSON.stringify(value));
            return true;
        } catch (e) {
            console.error('Storage set error:', e);
            return false;
        }
    },

    get(key, defaultValue = null) {
        try {
            const item = localStorage.getItem(key);
            if (item === null) return defaultValue;
            return JSON.parse(item);
        } catch (e) {
            console.error('Storage get error:', e);
            return defaultValue;
        }
    },

    remove(key) {
        try {
            localStorage.removeItem(key);
            return true;
        } catch (e) {
            console.error('Storage remove error:', e);
            return false;
        }
    },

    clear() {
        try {
            localStorage.clear();
            return true;
        } catch (e) {
            console.error('Storage clear error:', e);
            return false;
        }
    }
};

/**
 * 表单验证
 */
const validator = {
    // 验证学号（6-20位数字）
    studentId(value) {
        if (!this.required(value)) return false;
        const pattern = /^\d{6,20}$/;
        return pattern.test(value.trim());
    },

    // 验证密码（6-20位）
    password(value) {
        if (!this.required(value)) return false;
        const val = value.trim();
        return val.length >= 6 && val.length <= 20;
    },

    // 验证确认密码
    confirmPassword(value, original) {
        return this.password(value) && value === original;
    },

    // 验证手机号
    phone(value) {
        if (!this.required(value)) return false;
        const pattern = /^1[3-9]\d{9}$/;
        return pattern.test(value.trim());
    },

    // 验证邮箱
    email(value) {
        if (!this.required(value)) return false;
        const pattern = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
        return pattern.test(value.trim());
    },

    // 验证非空
    required(value) {
        return value !== null && value !== undefined && value.toString().trim() !== '';
    },

    // 验证长度
    length(value, min, max) {
        if (!this.required(value)) return false;
        const len = value.toString().trim().length;
        return len >= min && len <= max;
    }
};

/**
 * 网络状态检测
 */
const network = {
    isOnline() {
        return navigator.onLine;
    },

    watch(callback) {
        if (typeof callback !== 'function') return;
        window.addEventListener('online', () => callback(true));
        window.addEventListener('offline', () => callback(false));
    }
};

/**
 * 设备信息检测
 */
const device = {
    // 是否为移动端
    isMobile() {
        return /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
    },

    // 是否为微信浏览器
    isWechat() {
        return /MicroMessenger/i.test(navigator.userAgent);
    },

    // 是否为iOS
    isIOS() {
        return /iPad|iPhone|iPod/.test(navigator.userAgent) && !window.MSStream;
    },

    // 是否为Android
    isAndroid() {
        return /Android/.test(navigator.userAgent) && !window.MSStream;
    },

    // 是否为PC端
    isPC() {
        return !this.isMobile();
    }
};

/**
 * 页面可见性检测
 */
const visibility = {
    isVisible() {
        return document.visibilityState === 'visible';
    },

    watch(callback) {
        if (typeof callback !== 'function') return;
        document.addEventListener('visibilitychange', () => {
            callback(document.visibilityState === 'visible');
        });
    }
};

/**
 * 图片懒加载
 */
function lazyLoadImages() {
    // 兼容不支持IntersectionObserver的浏览器
    if (!('IntersectionObserver' in window)) {
        // 降级方案：直接加载所有图片
        document.querySelectorAll('img[data-src]').forEach(img => {
            img.src = img.dataset.src;
            img.removeAttribute('data-src');
        });
        return;
    }

    const images = document.querySelectorAll('img[data-src]');

    const imageObserver = new IntersectionObserver((entries, observer) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                const img = entry.target;
                img.src = img.dataset.src;
                img.removeAttribute('data-src');
                observer.unobserve(img);
            }
        });
    }, {
        rootMargin: '100px 0px' // 提前100px加载
    });

    images.forEach(img => imageObserver.observe(img));
}

/**
 * 滚动加载更多
 */
function scrollLoadMore(callback, threshold = 100) {
    if (typeof callback !== 'function') return () => { };

    const handleScroll = throttle(() => {
        const scrollTop = window.pageYOffset || document.documentElement.scrollTop;
        const windowHeight = window.innerHeight;
        const documentHeight = document.documentElement.scrollHeight;

        if (documentHeight - (scrollTop + windowHeight) < threshold) {
            callback();
        }
    }, 200);

    window.addEventListener('scroll', handleScroll);

    // 返回取消监听的函数
    return () => {
        window.removeEventListener('scroll', handleScroll);
    };
}

/**
 * 下拉刷新（仅移动端）
 */
function pullToRefresh(element, callback) {
    if (typeof callback !== 'function' || !element || !device.isMobile()) return;

    let startY = 0;
    let isPulling = false;

    const touchstartHandler = (e) => {
        if (element.scrollTop === 0) {
            startY = e.touches[0].pageY;
            isPulling = true;
        }
    };

    const touchmoveHandler = (e) => {
        if (!isPulling) return;

        const currentY = e.touches[0].pageY;
        const diff = currentY - startY;

        if (diff > 0 && diff < 100) {
            element.style.transform = `translateY(${diff * 0.5}px)`;
            element.style.transition = 'transform 0.2s ease';
        }
    };

    const touchendHandler = (e) => {
        if (!isPulling) return;

        isPulling = false;
        element.style.transform = '';
        element.style.transition = 'transform 0.2s ease';

        const currentY = e.changedTouches[0].pageY;
        if (currentY - startY > 80) {
            callback();
        }
    };

    // 添加事件监听
    element.addEventListener('touchstart', touchstartHandler);
    element.addEventListener('touchmove', touchmoveHandler, { passive: true });
    element.addEventListener('touchend', touchendHandler);

    // 返回取消监听的函数
    return () => {
        element.removeEventListener('touchstart', touchstartHandler);
        element.removeEventListener('touchmove', touchmoveHandler);
        element.removeEventListener('touchend', touchendHandler);
    };
}

/**
 * 安全区域适配（刘海屏）
 */
function setupSafeArea() {
    // 避免重复添加meta标签
    if (document.querySelector('meta[name="viewport"][content*="viewport-fit=cover"]')) return;

    const meta = document.createElement('meta');
    meta.name = 'viewport';
    meta.content = 'width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover';
    document.head.appendChild(meta);

    // 适配iOS安全区域
    if (device.isIOS()) {
        const style = document.createElement('style');
        style.textContent = `
            body {
                padding-top: env(safe-area-inset-top);
                padding-bottom: env(safe-area-inset-bottom);
                padding-left: env(safe-area-inset-left);
                padding-right: env(safe-area-inset-right);
            }
        `;
        document.head.appendChild(style);
    }
}

/**
 * 禁止双击缩放
 */
function preventDoubleTapZoom() {
    let lastTouchEnd = 0;
    document.addEventListener('touchend', (e) => {
        const now = Date.now();
        if (now - lastTouchEnd <= 300) {
            e.preventDefault();
        }
        lastTouchEnd = now;
    }, { passive: false });
}

/**
 * 初始化移动端优化
 */
function initMobileOptimization() {
    if (!device.isMobile()) return;

    // 禁止橡皮筋效果（iOS）- 兼容passive选项
    document.body.addEventListener('touchmove', (e) => {
        if (e.target === document.body && document.body.scrollTop === 0) {
            e.preventDefault();
        }
    }, { passive: false });

    // 禁止长按弹出菜单
    document.addEventListener('contextmenu', (e) => {
        e.preventDefault();
    });

    // 禁止选中文字（仅移动端）
    document.addEventListener('selectstart', (e) => {
        e.preventDefault();
    });
}

// 导出工具函数（兼容浏览器/Node环境）
if (typeof module !== 'undefined' && module.exports) {
    module.exports = {
        showToast,
        checkLogin,
        formatTime,
        formatDateTime,
        escapeHtml,
        debounce,
        throttle,
        storage,
        validator,
        network,
        device,
        visibility,
        lazyLoadImages,
        scrollLoadMore,
        pullToRefresh,
        setupSafeArea,
        preventDoubleTapZoom,
        initMobileOptimization
    };
}

// 浏览器环境挂载到window
if (typeof window !== 'undefined') {
    window.utils = {
        showToast,
        checkLogin,
        formatTime,
        formatDateTime,
        escapeHtml,
        debounce,
        throttle,
        storage,
        validator,
        network,
        device,
        visibility,
        lazyLoadImages,
        scrollLoadMore,
        pullToRefresh,
        setupSafeArea,
        preventDoubleTapZoom,
        initMobileOptimization
    };
}
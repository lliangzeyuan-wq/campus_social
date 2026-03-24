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
        window.location.href = '../index.html';
        return false;
    }
    return true;
}

/**
 * 格式化时间
 */
function formatTime(time) {
    if (!time) return '';
    
    const date = new Date(time);
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
    
    // 超过7天显示日期
    return date.toLocaleDateString('zh-CN');
}

/**
 * 格式化日期时间
 */
function formatDateTime(time) {
    if (!time) return '';
    
    const date = new Date(time);
    return date.toLocaleString('zh-CN', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit'
    });
}

/**
 * 转义 HTML 特殊字符
 */
function escapeHtml(text) {
    if (!text) return '';
    
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

/**
 * 防抖函数
 */
function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

/**
 * 节流函数
 */
function throttle(func, limit) {
    let inThrottle;
    return function executedFunction(...args) {
        if (!inThrottle) {
            func(...args);
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
            return item ? JSON.parse(item) : defaultValue;
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
    // 验证学号
    studentId(value) {
        const pattern = /^\d{6,20}$/;
        return pattern.test(value);
    },
    
    // 验证密码
    password(value) {
        return value && value.length >= 6 && value.length <= 20;
    },
    
    // 验证手机号
    phone(value) {
        const pattern = /^1[3-9]\d{9}$/;
        return pattern.test(value);
    },
    
    // 验证邮箱
    email(value) {
        const pattern = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
        return pattern.test(value);
    },
    
    // 验证非空
    required(value) {
        return value !== null && value !== undefined && value.toString().trim() !== '';
    },
    
    // 验证长度
    length(value, min, max) {
        const len = value ? value.length : 0;
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
        return /iPad|iPhone|iPod/.test(navigator.userAgent);
    },
    
    // 是否为Android
    isAndroid() {
        return /Android/.test(navigator.userAgent);
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
        document.addEventListener('visibilitychange', () => {
            callback(document.visibilityState === 'visible');
        });
    }
};

/**
 * 图片懒加载
 */
function lazyLoadImages() {
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
    });
    
    images.forEach(img => imageObserver.observe(img));
}

/**
 * 滚动加载更多
 */
function scrollLoadMore(callback, threshold = 100) {
    const handleScroll = throttle(() => {
        const scrollTop = window.pageYOffset || document.documentElement.scrollTop;
        const windowHeight = window.innerHeight;
        const documentHeight = document.documentElement.scrollHeight;
        
        if (documentHeight - (scrollTop + windowHeight) < threshold) {
            callback();
        }
    }, 200);
    
    window.addEventListener('scroll', handleScroll);
    
    return () => window.removeEventListener('scroll', handleScroll);
}

/**
 * 下拉刷新
 */
function pullToRefresh(element, callback) {
    let startY = 0;
    let isPulling = false;
    
    element.addEventListener('touchstart', (e) => {
        if (element.scrollTop === 0) {
            startY = e.touches[0].pageY;
            isPulling = true;
        }
    });
    
    element.addEventListener('touchmove', (e) => {
        if (!isPulling) return;
        
        const currentY = e.touches[0].pageY;
        const diff = currentY - startY;
        
        if (diff > 0 && diff < 100) {
            element.style.transform = `translateY(${diff * 0.5}px)`;
        }
    });
    
    element.addEventListener('touchend', () => {
        if (!isPulling) return;
        
        isPulling = false;
        element.style.transform = '';
        
        const currentY = event.changedTouches[0].pageY;
        if (currentY - startY > 80) {
            callback();
        }
    });
}

/**
 * 安全区域适配（刘海屏）
 */
function setupSafeArea() {
    const meta = document.createElement('meta');
    meta.name = 'viewport';
    meta.content = 'width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover';
    document.head.appendChild(meta);
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
    }, false);
}

/**
 * 初始化移动端优化
 */
function initMobileOptimization() {
    // 禁止橡皮筋效果（iOS）
    document.body.addEventListener('touchmove', (e) => {
        if (e.target === document.body) {
            e.preventDefault();
        }
    }, { passive: false });
    
    // 禁止长按弹出菜单
    document.addEventListener('contextmenu', (e) => {
        e.preventDefault();
    });
    
    // 禁止选中文字
    document.addEventListener('selectstart', (e) => {
        e.preventDefault();
    });
}

// 导出工具函数
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
        visibility
    };
}

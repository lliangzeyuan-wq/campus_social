#include <iostream>
#include <time.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>

// 【关键】包含公共头文件，使用标准数据结构
#include "common.h"

// 从文件加载用户
int load_users(User* all_users, int max_size) {
    FILE* fp = fopen(USER_FILE, "rb");
    if (!fp) return 0;
    int count = 0;
    while (fread(&all_users[count], sizeof(User), 1, fp) == 1 && count < max_size) count++;
    fclose(fp);
    return count;
}

// 保存用户到文件
void save_users(User* all_users, int count) {
    FILE* fp = fopen(USER_FILE, "wb");
    if (!fp) return;
    fwrite(all_users, sizeof(User), count, fp);
    fclose(fp);
}

// 1. 查看所有用户（已适配标准结构）
void list_users() {
    User all_users[MAX_USER];
    int all_count = load_users(all_users, MAX_USER);
    if (all_count == 0) {
        printf("❌ 暂无注册用户！\n");
        return;
    }

    printf("=====================================================\n");
    printf("               🔥 所有注册用户列表 🔥                \n");
    printf("=====================================================\n");
    for (int i = 0; i < all_count; i++) {
        User* u = &all_users[i];
        printf("👤 学号：%s\n", u->id);
        printf("   当前模式：");
        switch (u->current_mode) {
        case MODE_NONE: printf("未选择\n"); break;
        case MODE_STUDY: printf("学习搭子\n"); break;
        case MODE_POSTGRAD: printf("考研搭子\n"); break;
        case MODE_FRIEND: printf("兴趣交友\n"); break;
        case MODE_LOVE: printf("异性恋爱\n"); break;
        }

        // 打印详细信息（适配字段名）
        if (strlen(u->study_subject) > 0)
            printf("   [学习] 科目：%s | 年级：%s\n", u->study_subject, u->study_grade);
        if (strlen(u->postgrad_major) > 0)
            printf("   [考研] 专业：%s | 院校：%s\n", u->postgrad_major, u->postgrad_school);
        if (strlen(u->friend_hobby) > 0)
            printf("   [交友] 兴趣：%s | 性格：%s\n", u->friend_hobby, u->friend_personality);
        if (strlen(u->love_gender) > 0)
            printf("   [恋爱] 性别：%s | 简介：%.30s...\n", u->love_gender, u->love_intro);

        printf("   好友数：%d | 待处理申请：%d/%d\n", u->friend_cnt, u->app_cnt, MAX_APPS);
        printf("-----------------------------------------------------\n");
    }
}

// 辅助：加载指定文件
int load_from_file(User* all_users, int max_size, const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) return 0;
    int count = 0;
    while (fread(&all_users[count], sizeof(User), 1, fp) == 1 && count < max_size) count++;
    fclose(fp);
    return count;
}

// 5. 查看指定备份
void view_backup() {
    char backup_name[100];
    printf("请输入备份文件名（例如：test.dat）：");
    scanf("%s", backup_name);

    User all_users[MAX_USER];
    int count = load_from_file(all_users, MAX_USER, backup_name);

    if (count == 0) {
        printf("❌ 备份文件不存在或无数据！\n");
        return;
    }

    printf("=====================================================\n");
    printf("            🔥 备份文件【%s】内容 🔥            \n", backup_name);
    printf("=====================================================\n");
    for (int i = 0; i < count; i++) {
        User* u = &all_users[i];
        printf("👤 学号：%s | 模式：%d | 好友数：%d\n", u->id, u->current_mode, u->friend_cnt);
    }
    printf("-----------------------------------------------------\n");
}

// 6. 查看所有备份
void list_backups() {
    DIR* dir;
    struct dirent* ent;
    int found = 0;
    printf("=====================================================\n");
    printf("               🔥 所有备份文件列表 🔥                \n");
    printf("=====================================================\n");
    if ((dir = opendir(".")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".dat") != NULL && strcmp(ent->d_name, USER_FILE) != 0) {
                printf("📦 %s\n", ent->d_name);
                found = 1;
            }
        }
        closedir(dir);
    }
    if (!found) printf("❌ 暂无备份文件！\n");
    printf("-----------------------------------------------------\n");
}

// 7. 重命名备份
void rename_backup() {
    char old_name[100], new_name[100], temp_name[100];
    printf("请输入要重命名的旧备份文件名：");
    scanf("%s", old_name);
    if (strcmp(old_name, USER_FILE) == 0) { printf("❌ 禁止修改原文件！\n"); return; }

    FILE* check = fopen(old_name, "rb");
    if (!check) { printf("❌ 旧文件不存在！\n"); return; }
    fclose(check);

    printf("请输入新备份文件名（无需加.dat）：");
    scanf("%s", temp_name);
    sprintf(new_name, "%s.dat", temp_name);

    check = fopen(new_name, "rb");
    if (check) { fclose(check); printf("❌ 新文件已存在！\n"); return; }

    if (rename(old_name, new_name) == 0) printf("✅ 重命名成功！\n");
    else printf("❌ 重命名失败！\n");
}

// 4. 备份数据
void backup_users() {
    FILE* src = fopen(USER_FILE, "rb");
    if (!src) { printf("❌ 无数据可备份！\n"); return; }

    int choice;
    char backup_filename[100] = { 0 };
    printf("\n请选择备份方式：\n1. 自动命名\n2. 自定义\n请输入：");
    scanf("%d", &choice);

    if (choice == 1) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        strftime(backup_filename, sizeof(backup_filename), "users_backup_%Y%m%d_%H%M%S.dat", t);
    }
    else if (choice == 2) {
        char temp[50];
        printf("请输入文件名（无需后缀）：");
        scanf("%s", temp);
        sprintf(backup_filename, "%s.dat", temp);
    }
    else {
        printf("❌ 输入错误！\n"); fclose(src); return;
    }

    FILE* dst = fopen(backup_filename, "wb");
    if (!dst) { fclose(src); return; }
    char buf[1024]; size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) fwrite(buf, 1, n, dst);
    fclose(src); fclose(dst);
    printf("✅ 已备份到：%s\n", backup_filename);
}

// 2. 删除用户
void delete_user(const char* id) {
    User all_users[MAX_USER];
    int all_count = load_users(all_users, MAX_USER);
    int found = 0;
    for (int i = 0; i < all_count; i++) {
        if (strcmp(all_users[i].id, id) == 0) {
            found = 1;
            for (int j = i; j < all_count - 1; j++) all_users[j] = all_users[j + 1];
            all_count--; break;
        }
    }
    if (!found) printf("❌ 未找到用户：%s\n", id);
    else { save_users(all_users, all_count); printf("✅ 已删除用户：%s\n", id); }
}

// 3. 清空用户
void clear_users() {
    FILE* fp = fopen(USER_FILE, "wb");
    if (fp) fclose(fp);
    printf("✅ 已清空所有数据！\n");
}

int main() {
    int choice;
    while (1) {
        printf("\n=====================================================\n");
        printf("               🔥 用户数据管理工具 🔥                \n");
        printf("=====================================================\n");
        printf("1. 查看所有用户\n");
        printf("2. 删除指定用户\n");
        printf("3. 清空所有用户\n");
        printf("4. 备份数据\n");
        printf("5. 查看指定备份\n");
        printf("6. 查看所有备份\n");
        printf("7. 重命名备份\n");
        printf("8. 退出\n");
        printf("请输入选项：");
        scanf("%d", &choice);

        switch (choice) {
        case 1: list_users(); break;
        case 2: { char id[20]; printf("请输入学号："); scanf("%s", id); delete_user(id); break; }
        case 3: clear_users(); break;
        case 4: backup_users(); break;
        case 5: view_backup(); break;
        case 6: list_backups(); break;
        case 7: rename_backup(); break;
        case 8: return 0;
        default: printf("❌ 无效选项！\n");
        }
    }
    return 0;
}
-- 初始化日志表（用于记录脚本执行情况）
CREATE TABLE IF NOT EXISTS script_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    log_message VARCHAR(255) NOT NULL,
    log_time DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 创建日志存储过程
DELIMITER $$

CREATE PROCEDURE log_message(IN message VARCHAR(255))
BEGIN
    INSERT INTO script_logs (log_message) VALUES (message);
END$$

DELIMITER ;

-- 变量声明
SET @db_name = 'talko_server';
SET @table_name = 'user_info';
SET @db_exists = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = @db_name);

-- 记录日志：检查数据库是否存在
IF @db_exists > 0 THEN
    CALL log_message(CONCAT('Database ', @db_name, ' already exists.'));
ELSE
    CREATE DATABASE talko_server;
    CALL log_message(CONCAT('Database ', @db_name, ' created successfully.'));
END IF;

-- 切换数据库
USE talko_server;

-- 捕获 SQL 失败错误
DECLARE CONTINUE HANDLER FOR SQLEXCEPTION
BEGIN
    CALL log_message(CONCAT('SQL Error occurred while creating table: ', @table_name));
END;

-- 记录日志：检查表是否存在
SET @table_exists = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = @db_name AND TABLE_NAME = @table_name);

IF @table_exists > 0 THEN
    CALL log_message(CONCAT('Table ', @table_name, ' already exists.'));
ELSE
    CREATE TABLE user_info (
        id INT AUTO_INCREMENT PRIMARY KEY,      -- 用户ID（唯一，不可重复）
        username VARCHAR(255) NOT NULL,         -- 用户名（可以重复）
        password_hash VARCHAR(255) NOT NULL,    -- 密码哈希
        nickname VARCHAR(255),                  -- 昵称（可以重复）
        avatar_url VARCHAR(255),                -- 头像URL
        gender ENUM('male', 'female', 'other'), -- 性别
        birthday DATE,                           -- 生日
        phone_number VARCHAR(20),                -- 手机号（可以重复）
        email VARCHAR(255),                      -- 邮箱（可以重复）
        region VARCHAR(255),                     -- 地区
        status ENUM('active', 'inactive', 'banned'), -- 账户状态
        last_login_time DATETIME,                -- 上次登录时间
        create_time DATETIME DEFAULT CURRENT_TIMESTAMP,  -- 创建时间
        update_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP -- 更新时间
    );

    CALL log_message(CONCAT('Table ', @table_name, ' created successfully.'));
END IF;


-- 创建用户隐私设置表
CREATE TABLE user_privacy (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    allow_friend_requests BOOLEAN DEFAULT TRUE,
    allow_message_from_strangers BOOLEAN DEFAULT TRUE,
    show_online_status BOOLEAN DEFAULT TRUE,
    show_last_seen BOOLEAN DEFAULT TRUE,
    show_profile_picture BOOLEAN DEFAULT TRUE,
    FOREIGN KEY (user_id) REFERENCES user_info(id)
);

-- 创建用户社交关系表
CREATE TABLE user_friends (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    friend_id INT NOT NULL,
    status ENUM('pending', 'accepted', 'blocked'),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES user_info(id),
    FOREIGN KEY (friend_id) REFERENCES user_info(id)
);

-- 创建用户消息记录表
CREATE TABLE user_messages (
    id INT AUTO_INCREMENT PRIMARY KEY,
    sender_id INT NOT NULL,
    receiver_id INT NOT NULL,
    message_type ENUM('text', 'image', 'video', 'file'),
    content TEXT,
    sent_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    read_at DATETIME,
    status ENUM('sent', 'delivered', 'read'),
    FOREIGN KEY (sender_id) REFERENCES user_info(id),
    FOREIGN KEY (receiver_id) REFERENCES user_info(id)
);

-- 创建用户登录日志表
CREATE TABLE user_login_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    login_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    ip_address VARCHAR(255),
    device VARCHAR(255),
    location VARCHAR(255),
    FOREIGN KEY (user_id) REFERENCES user_info(id)
);

-- 创建用户通知设置表
CREATE TABLE user_notifications (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    allow_push BOOLEAN DEFAULT TRUE,
    allow_email BOOLEAN DEFAULT TRUE,
    allow_sms BOOLEAN DEFAULT TRUE,
    FOREIGN KEY (user_id) REFERENCES user_info(id)
);

# 用户信息数据库表设计

## 1. 用户基本信息表 (user_info)

| 字段名称         | 类型             | 描述                                       |
|------------------|------------------|--------------------------------------------|
| id               | INT AUTO_INCREMENT | 用户的唯一标识符，主键                     |
| username         | VARCHAR(255)      | 用户名，唯一                               |
| password_hash    | VARCHAR(255)      | 密码的哈希值                               |
| nickname         | VARCHAR(255)      | 昵称                                       |
| avatar_url       | VARCHAR(255)      | 头像 URL                                   |
| gender           | ENUM('male', 'female', 'other') | 性别                                 |
| birthday         | DATE              | 生日                                       |
| phone_number     | VARCHAR(20)       | 手机号码                                   |
| email            | VARCHAR(255)      | 邮箱                                       |
| region           | VARCHAR(255)      | 所在地区                                   |
| status           | ENUM('active', 'inactive', 'banned') | 用户状态                               |
| last_login_time  | DATETIME          | 最后登录时间                               |
| create_time      | DATETIME          | 账户创建时间                               |
| update_time      | DATETIME          | 最近更新时间                               |

## 2. 用户隐私设置表 (user_privacy)

| 字段名称                | 类型       | 描述                                       |
|-------------------------|------------|--------------------------------------------|
| id                      | INT        | 主键                                       |
| user_id                 | INT        | 用户 ID，外键，关联 `user_info` 表         |
| allow_friend_requests   | BOOLEAN    | 是否允许陌生人加好友                       |
| allow_message_from_strangers | BOOLEAN | 是否允许陌生人发送消息                   |
| show_online_status      | BOOLEAN    | 是否显示在线状态                           |
| show_last_seen          | BOOLEAN    | 是否显示最后在线时间                       |
| show_profile_picture    | BOOLEAN    | 是否允许查看头像                           |

## 3. 用户社交关系表 (user_friends)

| 字段名称         | 类型           | 描述                                       |
|------------------|----------------|--------------------------------------------|
| id               | INT AUTO_INCREMENT | 主键                                       |
| user_id          | INT            | 用户 ID，外键，关联 `user_info` 表         |
| friend_id        | INT            | 好友的用户 ID，外键，关联 `user_info` 表   |
| status           | ENUM('pending', 'accepted', 'blocked') | 好友请求状态（待确认、已接受、已屏蔽） |
| created_at       | DATETIME        | 好友关系创建时间                           |
| updated_at       | DATETIME        | 好友关系更新时间                           |

## 4. 用户消息记录表 (user_messages)

| 字段名称         | 类型           | 描述                                       |
|------------------|----------------|--------------------------------------------|
| id               | INT AUTO_INCREMENT | 主键                                       |
| sender_id        | INT            | 发送者 ID，外键，关联 `user_info` 表       |
| receiver_id      | INT            | 接收者 ID，外键，关联 `user_info` 表       |
| message_type     | ENUM('text', 'image', 'video', 'file') | 消息类型（文本、图片、视频、文件）       |
| content          | TEXT           | 消息内容                                   |
| sent_at          | DATETIME        | 发送时间                                   |
| read_at          | DATETIME        | 阅读时间（如果有的话）                     |
| status           | ENUM('sent', 'delivered', 'read') | 消息状态                               |

## 5. 用户登录日志表 (user_login_logs)

| 字段名称         | 类型           | 描述                                       |
|------------------|----------------|--------------------------------------------|
| id               | INT AUTO_INCREMENT | 主键                                       |
| user_id          | INT            | 用户 ID，外键，关联 `user_info` 表         |
| login_time       | DATETIME        | 登录时间                                   |
| ip_address       | VARCHAR(255)    | 登录 IP 地址                               |
| device           | VARCHAR(255)    | 登录设备                                   |
| location         | VARCHAR(255)    | 登录位置                                   |

## 6. 用户通知设置表 (user_notifications)

| 字段名称         | 类型           | 描述                                       |
|------------------|----------------|--------------------------------------------|
| id               | INT AUTO_INCREMENT | 主键                                       |
| user_id          | INT            | 用户 ID，外键，关联 `user_info` 表         |
| allow_push       | BOOLEAN        | 是否允许推送通知                           |
| allow_email      | BOOLEAN        | 是否允许通过电子邮件接收通知               |
| allow_sms        | BOOLEAN        | 是否允许通过短信接收通知                   |

#!/bin/bash

# 获取脚本所在目录
SCRIPT_DIR=$(dirname "$(realpath "$0")")
# 构建挂载目录，即脚本所在目录上一级的 data 文件夹
DATA_DIR="$SCRIPT_DIR/../data"

# 配置参数
MYSQL_ROOT_PASSWORD="talko_root"
MYSQL_DATABASE="talko_server"
MYSQL_USER="talko_server"
MYSQL_PASSWORD="talko_server_pwd"
MYSQL_TABLE_NAME="user_info"
# DATA_DIR="../data/docker-mysql"  # MySQL 数据目录
ROOT_PATH="$SCRIPT_DIR/.."
DATA_DIR="$(realpath $ROOT_PATH/data/mysql)"   # 获取绝对路径
echo $DATA_DIR

# 检查 Docker 是否已安装
if ! command -v docker &> /dev/null; then
    echo "Docker 未安装，请先安装 Docker。"
    exit 1
fi


# 拉取 MySQL 镜像（如果尚未拉取）
if ! docker images mysql:8.0 --format "{{.Repository}}:{{.Tag}}" | grep -q "mysql:8.0"; then
    echo "正在拉取 MySQL 镜像..."
    docker pull mysql:8.0
else
    echo "MySQL:8.0 镜像已存在，跳过拉取。"
fi

# 创建 MySQL 数据目录（如果尚未创建）
if [ ! -d "$DATA_DIR" ]; then
    echo "创建 MySQL 数据目录: $DATA_DIR"
    mkdir -p $DATA_DIR
    # chmod 775 "$DATA_DIR"
else
    echo "MySQL 数据目录已存在，跳过创建。"
fi

# 检查 MySQL 容器是否已经在运行
if docker ps | grep -q "mysql-server"; then
    echo "MySQL 容器已在运行，跳过启动步骤。"
else
    # 运行 MySQL 容器
    echo "正在启动 MySQL 容器..."
    docker run -d \
      --name mysql-server \
      -p 6603:3306 \
      -e MYSQL_ROOT_PASSWORD=$MYSQL_ROOT_PASSWORD \
      -e MYSQL_DATABASE=$MYSQL_DATABASE \
      -e MYSQL_USER=$MYSQL_USER \
      -e MYSQL_PASSWORD=$MYSQL_PASSWORD \
      -v "$DATA_DIR":/var/lib/mysql \
      mysql:8.0 \
      --bind-address=127.0.0.1 \
fi

# 检查容器是否运行
if docker ps | grep -q "mysql-server"; then
    echo "MySQL 容器已成功启动！"
else
    echo "MySQL 容器启动失败，请检查日志。"
    exit 1
fi

# 验证 MySQL 连接
echo "验证 MySQL 连接..."
sleep 10  # 等待 MySQL 初始化完成

docker exec -it mysql-server mysql -u root -p$MYSQL_ROOT_PASSWORD -e "SHOW DATABASES;"

# 验证 MySQL 连接是否成功
if [ $? -eq 0 ]; then
    echo "MySQL 连接成功！"
else
    echo "MySQL 连接失败，请检查日志。"
    exit 1
fi

# 判断 user_info 表是否已经存在
TABLE_EXISTS=$(docker exec -it mysql-server mysql -u root -p$MYSQL_ROOT_PASSWORD -N -s -e "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '$MYSQL_DATABASE' AND table_name = 'user_info'")

if [ "$TABLE_EXISTS" -eq 1 ]; then
    echo "'$MYSQL_TABLE_NAME' 表已存在，跳过创建。"
else
    # 创建表
    echo "正在创建 '$MYSQL_TABLE_NAME' 表..."
    docker exec -i mysql-server mysql -u root -p$MYSQL_ROOT_PASSWORD $MYSQL_DATABASE <<EOF
    CREATE TABLE $MYSQL_TABLE_NAME (
        id INT AUTO_INCREMENT PRIMARY KEY,
        user_id VARCHAR(255) NOT NULL,
        nickname VARCHAR(255),
        avatar VARCHAR(255),
        signature TEXT,
        birthday DATE,
        gender ENUM('male', 'female', 'other'),
        phone_number VARCHAR(20),
        friends TEXT
    );
EOF

    # 验证表是否创建成功
    if [ $? -eq 0 ]; then
        echo "'$MYSQL_TABLE_NAME' 表创建成功！"
    else
        echo "'$MYSQL_TABLE_NAME' 表创建失败，请检查日志。"
        exit 1
    fi
fi

echo "MySQL 安装和配置完成！"
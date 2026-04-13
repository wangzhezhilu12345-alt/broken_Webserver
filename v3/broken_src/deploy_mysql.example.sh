#!/usr/bin/env bash

export BROKEN_SERVER_DB_HOST="127.0.0.1"
export BROKEN_SERVER_DB_PORT="3306"
export BROKEN_SERVER_DB_NAME="storiesdb"
export BROKEN_SERVER_DB_USER="game_login_user"
export BROKEN_SERVER_DB_PASSWORD="replace-with-real-password"
export BROKEN_SERVER_DB_TABLE="game_accounts"

# Optional fallback recognized by the server:
# export MYSQL_PWD="replace-with-real-password"

# Initialize schema manually if needed:
# mysql -h "$BROKEN_SERVER_DB_HOST" -P "$BROKEN_SERVER_DB_PORT" -u "$BROKEN_SERVER_DB_USER" -p"$BROKEN_SERVER_DB_PASSWORD" < init_login_schema.sql

# Start server:
# ./server

#!/bin/bash

curl -i -X POST -H "Accept:application/json" -H "Content-Type:application/json" localhost:8083/connectors/ -d '{
  "name": "tia-mariadb-connector",
  "config": {
    "connector.class": "io.debezium.connector.mysql.MySqlConnector",
    "tasks.max": "1",
    "topic.prefix": "tia_server",
    "database.hostname": "mariadb",
    "database.port": "3306",
    "database.user": "root",
    "database.password": "devpasswordroot",
    "database.server.id": "1",
    "database.include.list": "dev_tia_db",
    "schema.history.internal.kafka.bootstrap.servers": "kafka:9092",
    "schema.history.internal.kafka.topic": "schema-changes.tia_db"
  }
}'

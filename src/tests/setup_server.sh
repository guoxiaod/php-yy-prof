#!/bin/bash


#yum install -y --enablerepo=mariadb  \
#    redis rabbitmq-server  \
#    aerospike-server-community  aerospike-tools \
#    mongodb mongodb-server \
#    MariaDB-server  MariaDB-client

if [ -x /usr/bin/systemctl ]; then

    systemctl restart redis
    systemctl restart aerospike
    systemctl restart mysql
    systemctl restart mongod
    systemctl restart rabbitmq-server

else 

    /etc/init.d/redis restart
    /etc/init.d/aerospike restart
    /etc/init.d/mysql restart
    /etc/init.d/mongod restart
    /etc/init.d/rabbitmq-server restart

fi

function setup_mysql_db() {
    YY_PROF_SQL_FILE=/tmp/yy_prof.sql
    cat <<SQL > $YY_PROF_SQL_FILE
    create database if not exists yy_prof;
    use yy_prof;
    create table test (
        id int not null auto_increment primary key,
        name varchar(30),
        brithday date
    ) engine = innodb;
SQL
    
    mysql -uroot < $YY_PROF_SQL_FILE
    rm -rf $YY_PROF_SQL_FILE
}

function setup_redis() {
}

function setup_rabbitmq() {
    rabbitmqctl add_user yy_prof yy_prof
    rabbitmqctl add_vhost /yy_prof
    rabbitmqctl set_permissions -p /yy_prof yy_prof '.*' '.*' '.*'
}

setup_mysql_db
setup_redis
setup_rabbitmq

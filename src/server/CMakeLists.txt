
#定义了一个SRC_LIST包含了当前目录下所有文件
aux_source_directory(. SRC_LIST)

aux_source_directory(./db SRC_DB_LIST)

aux_source_directory(./model SRC_MODEL_LIST)

aux_source_directory(./redis SRC_REDIS_LIST)

#指定生成可执行文件
add_executable(chatServer ${SRC_LIST} ${SRC_DB_LIST} ${SRC_MODEL_LIST} ${SRC_REDIS_LIST})

#指定可执行文件所需的库文件
target_link_libraries(chatServer muduo_net muduo_base mysqlclient hiredis pthread)



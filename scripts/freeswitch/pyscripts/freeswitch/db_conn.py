# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: get the database connection
'''

import MySQLdb
import db_config
import router

CONN = None

def connect_db():
    '''
    @todo: 连接数据库
    '''
    global CONN
    
    # 从配置文件获取数据库配置信息
    dict = db_config.get_db_param()
    print dict
    list = []
    for item in dict:
        list.append(dict[item])
    print dict
        
    # 连接数据库
    CONN = MySQLdb.connect(list[1], list[0], list[4], list[2], int(list[5])) #connect database

#router.make_route(1)
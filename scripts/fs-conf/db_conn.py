# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: get the database connection
'''

import MySQLdb
import db_config
import file_info

CONN = None

def connect_db():
    '''
    @todo: �������ݿ�
    '''
    global CONN
    
    # 连接数据库
    try:
        _dict = db_config.get_db_param()
    except Exception,err:
        file_info.get_cur_runtime_info('Catch Exception:%s' % str(err))
        return -1
    else:
        if _dict == []:
            return -1
        
        try:
            # 连接数据库MySQLdb.connect(hostname, username, password, dbname, port)
            CONN = MySQLdb.connect(_dict['host'], _dict['username'], _dict['password'], _dict['dbname'], int(_dict['port']))
        except Exception,err:
            file_info.get_cur_runtime_info('Catch Exception:%s' % str(err))
            return -1
        else:
            return 1

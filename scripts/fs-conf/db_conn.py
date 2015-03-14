# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: get the database connection
'''

import MySQLdb
import db_config

CONN = None

def connect_db():
    '''
    @todo: �������ݿ�
    '''
    global CONN
    
    # �������ļ���ȡ���ݿ�������Ϣ
    _dict = db_config.get_db_param()
    if _dict == []:
        return -1
        
    # �������ݿ� MySQLdb.connect(hostname, username, password, dbname, port)
    CONN = MySQLdb.connect(_dict['host'], _dict['username'], _dict['password'], _dict['dbname'], int(_dict['port'])) 
    if CONN is None:
        return -1
    
    return 1

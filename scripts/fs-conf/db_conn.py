#coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@togo: connect database
'''
import MySQLdb
import db_config
import file_info
from _mysql import DatabaseError

def connect_db():
    '''
    @todo: 连接数据库
    '''
    try:
        # 获取配置参数
        _dict = db_config.get_db_param()
    except Exception, err:
        file_info.print_file_info('Catch Exception:%s' % str(err))
        return -1
    else:
        if [] == _dict:
            file_info.print_file_info('_dict is empty...........')
            return -1
        
        if -1 == _dict:
            file_info.print_file_info('Get param failure.........')
            return -1
        
        try:
            # 连接数据库 MySQLdb.connect(hostname, username, password, dbname, port) socket
            conn = MySQLdb.connect(_dict['host'], _dict['username'], _dict['password'], _dict['dbname'], int(_dict['port']), '/tmp/mysql.sock')
        except DatabaseError, err:
            file_info.print_file_info('Catch DatabaseError:%s' % str(err))
            return -1
        else:
            return conn
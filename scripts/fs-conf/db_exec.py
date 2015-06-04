# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: operation of database
'''

import file_info
import db_conn
from _mysql import DatabaseError

def exec_SQL(seqSQLCmd):
    '''
    @todo: 执行SQL语句
    '''
    
    if seqSQLCmd.strip() == '':
        file_info.print_file_info('Exec SQL FAIL, seqSQLCmd is:%s' % seqSQLCmd)
        return -1
    
    try:
        conn = db_conn.connect_db()
    except DatabaseError, err:
        file_info.print_file_info('Catch DatabaseError:%s' % str(err))
        # 不能使调用模块的程序退出
        return 1
    else:
        if -1 == conn:
            file_info.print_file_info('Database connected FAIL,is %d' % conn)
            return -1
        
        # 获取数据库游标
        cursor = conn.cursor()
        if cursor is None:
            file_info.print_file_info('Get cursor FAIL.')
            return -1
        
        # 执行SQL语句
        cursor.execute(seqSQLCmd)
        
        # 获取执行结果
        listResult = cursor.fetchall()
        
        if len(listResult) == 0:
            file_info.print_file_info('ERR: len(listResult) is: %d' % len(listResult))
            return -1
        
        conn.close()
        
        file_info.print_file_info('Exec SQL SUCC. SQL(%s)' % seqSQLCmd);
        return listResult
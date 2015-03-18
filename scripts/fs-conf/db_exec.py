#coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: operation of database
'''

import file_info
import db_conn

def exec_SQL(seqSQLCmd):
    '''
    @todo: 执行SQL语句
    '''
    
    if seqSQLCmd.strip() == '':
        file_info.print_file_info('seqSQLCmd is %s' % seqSQLCmd)
        return -1
    
    file_info.print_file_info('seqSQLCmd is %s' % seqSQLCmd)
    
    try:
        conn = db_conn.connect_db()
    except Exception, err:
        file_info.print_file_info('Catch Exception:%s' % str(err))
        return -1
    else:
        if -1 == conn:
            file_info.print_file_info('conn is %d' % conn)
            return -1
        
        # 获取数据库游标
        cursor = conn.cursor()
        if cursor is None:
            file_info.print_file_info('The cursor of database does not exist.')
            return -1
        
        # 执行SQL语句
        cursor.execute(seqSQLCmd)
        
        # 获取执行结果
        listResult = cursor.fetchall()
        
        if len(listResult) == 0:
            file_info.print_file_info('len(listResult) is %d' % len(listResult))
            return -1
        
        conn.close()
        
        file_info.print_file_info(listResult);
        return listResult

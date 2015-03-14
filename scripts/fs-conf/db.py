# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: pakete the operation about database
'''

import MySQLdb
import file_info

def get_db_conn(seqDBHost, seqDBUsername, seqDBPassword, seqDBName): 
    '''
    @param db_host: 数据库主机ip
    @param db_username: 数据库用户名
    @param db_password: 数据库密码
    @param db_name: 数据库名
    @todo: 连接数据库
    '''
    if seqDBHost.strip() == '':
        file_info.get_cur_runtime_info('seqDBHost is %s' % seqDBHost)
        return -1
    if seqDBUsername.strip() == '':
        file_info.get_cur_runtime_info('seqDBUsername is %s' % seqDBUsername)
        return -1
    if seqDBPassword.strip() == '':
        file_info.get_cur_runtime_info('seqDBPassword is %s' % seqDBPassword)
        return -1
    if seqDBName.strip() == '':
        file_info.get_cur_runtime_info('seqDBName is %s' % seqDBName)
        return -1
    
    file_info.get_cur_runtime_info("%s %s %s %s" % (seqDBHost, seqDBUsername, seqDBPassword, seqDBName))
    conn = MySQLdb.connect(seqDBHost,  seqDBUsername, seqDBPassword, seqDBName)
    if conn is None:
        file_info.get_cur_runtime_info('conn is %p' % conn)
        return -1
    return conn

def get_field_value(seqTableName, szFieldName, conn):
    '''
    @param table_name: 数据库表名
    @param field_name: 字段名称
    @param conn: 数据库连接
    @todo: 从数据库字段中获取sip列表
    '''
    if seqTableName.strip() == '':
        file_info.get_cur_runtime_info('seqTableName is %s' % seqTableName)
        return -1
    if szFieldName.strip() == '':
        file_info.get_cur_runtime_info('szFieldName is %s' % szFieldName)
        return -1
    seqSQLCmd = 'SELECT DISTINCT %s FROM %s' % (szFieldName, seqTableName)
    file_info.get_cur_runtime_info('execute SQL:%s' % seqSQLCmd)
    cursor = conn.cursor()
    cursor.execute(seqSQLCmd)
    results = cursor.fetchall()
    file_info.get_cur_runtime_info(results)
    return results

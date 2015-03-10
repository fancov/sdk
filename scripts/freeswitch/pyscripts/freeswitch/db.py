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
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'db_host is', seqDBHost
        return
    if seqDBUsername.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'db_username is', seqDBUsername
        return
    if seqDBPassword.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'db_password is', seqDBPassword
        return
    if seqDBName.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'db_name is', seqDBName
        return
    print  file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),seqDBHost,seqDBUsername,seqDBPassword,seqDBName
    conn = MySQLdb.connect(seqDBHost,  seqDBUsername, seqDBPassword, seqDBName)
    return conn

def get_field_value(seqTableName, szFieldName, conn):
    '''
    @param table_name: 数据库表名
    @param field_name: 字段名称
    @param conn: 数据库连接
    @todo: 从数据库字段中获取sip列表
    '''
    if seqTableName.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'table_name is', seqTableName
        return
    if szFieldName.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'field_name is', szFieldName
        return
    seqSQLCmd = 'SELECT DISTINCT %s FROM %s' % (szFieldName, seqTableName)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'execute SQL:', seqSQLCmd
    cursor = conn.cursor()
    cursor.execute(seqSQLCmd)
    results = cursor.fetchall()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'executed results is:', results
    return results
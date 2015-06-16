# coding=utf-8

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

DB_CONN = None

def connect_db():
    '''
    @todo: 连接数据库
    '''
    global DB_CONN

    try:
        # 获取配置参数
        _dict = db_config.get_db_param()
    except Exception, err:
        file_info.print_file_info('Catch Exception:%s' % str(err))
        return -1
    else:
        if [] == _dict:
            file_info.print_file_info('Connect database FAIL.')
            return -1
        
        if -1 == _dict:
            file_info.print_file_info('Connect database FAIL.')
            return -1
        
        try:
            # 连接数据库 MySQLdb.connect(hostname, username, password, dbname, port, sockfile)
            DB_CONN = MySQLdb.connect(_dict['host'], _dict['username'], _dict['password'], _dict['dbname'],
                                      int(_dict['port']), _dict['sockpath'])
        except DatabaseError, err:
            file_info.print_file_info('Catch DatabaseError:%s.' % str(err))
            return -1
        else:
            return DB_CONN

def exec_SQL(seqSQLCmd):
    '''
    @todo: 执行SQL语句
    '''
    global DB_CONN

    if seqSQLCmd.strip() == '':
        file_info.print_file_info('Exec SQL FAIL, seqSQLCmd is:%s.' % seqSQLCmd)
        return -1

    # 获取数据库游标
    if DB_CONN is None:
        DB_CONN = connect_db()

    with DB_CONN:
        cursor = DB_CONN.cursor()
        if cursor is None:
            file_info.print_file_info('Get cursor FAIL.')
            return -1

        # 执行SQL语句
        try:
            cursor.execute(seqSQLCmd)
        except Exception, err:
            file_info.print_file_info('Catch Exception:%s.' % str(err))
            cursor.close()
            return -1

        # 获取执行结果
        listResult = cursor.fetchall()

        if len(listResult) == 0:
            file_info.print_file_info('ERR: len(listResult) is: %d. SQL:%s, Msg: %s.'
                                      % (len(listResult), seqSQLCmd, DB_CONN.Error.message))
            cursor.close()
            return -1

        file_info.print_file_info('Exec SQL SUCC.')
        cursor.close()
        return listResult


def get_all_customer():
    '''
    @todo: 获取所有的客户
    '''

    seqSQLCmd = 'SELECT DISTINCT customer_id FROM tbl_sip;'
    listCus = exec_SQL(seqSQLCmd)
    if -1 == listCus:
        file_info.print_file_info('Get All Customers FAIL.')
        return -1

    file_info.print_file_info('Get All Customers SUCC.')
    return listCus


def get_sip_by_customer(ulCustomerID):
    '''
    @todo: 根据customer获取所有sip账户
    '''

    if str(ulCustomerID).strip() == '':
        file_info.print_file_info('Get SIP by Customer FAIL.')
        return -1

    seqSQLCmd = 'SELECT DISTINCT id FROM tbl_sip WHERE customer_id = %d;' % ulCustomerID
    listSip = exec_SQL(seqSQLCmd)
    if -1 == listSip:
        file_info.print_file_info('Get SIP by Customer FAIL.')
        return -1

    file_info.print_file_info('Get SIP by Customer SUCC.')
    return listSip

def get_sipinfo_by_userid(userid):
    '''
    @todo: 根据userid获取sip相关信息
    '''

    if str(userid).strip() == '':
        file_info.print_file_info('userid does not exist...')
        return -1

    # 根据userid获取sip相关信息
    seqSQLCmd = 'SELECT id, customer_id, extension, dispname, authname, auth_password ' \
                'FROM tbl_sip WHERE userid = \'%s\';' % str(userid)
    listSipInfo = exec_SQL(seqSQLCmd)
    if -1 == listSipInfo:
        file_info.print_file_info('Get SIP info by user ID FAIL,listSipInfo is %d.' % listSipInfo)
        return -1

    file_info.print_file_info('Get SIP info by user ID(%d) SUCC.' % int(userid))
    return listSipInfo

def get_userid_by_sipid(ulSipID):
    '''
    @todo: 根据id获取userid
    '''

    id = int(ulSipID)

    if str(ulSipID).strip() == '':
        file_info.print_file_info('Get User ID by SIP ID FAIL.')
        return -1

    seqSQLCmd = 'SELECT userid FROM tbl_sip WHERE `id` = %d;' % id
    lRet = exec_SQL(seqSQLCmd)
    if -1 == lRet:
        file_info.print_file_info('Get User ID by SIP ID(%d) FAIL,lRet is %d.' % (id, lRet))
        return -1

    seqUserID = str(lRet[0][0])
    file_info.print_file_info('Get User ID by SIP ID(%d) SUCC.' % id)
    return seqUserID

def get_customerid_by_sipid(ulSipID):
    '''
    @todo: 根据sipid获取customerid
    '''
    if str(ulSipID).strip() == '':
        file_info.print_file_info('Get Customer ID by SIP ID FAIL.')
        return -1

    seqSQLCmd = 'SELECT customer_id FROM tbl_sip WHERE id = %d;' % ulSipID
    lRet = exec_SQL(seqSQLCmd)
    if -1 == lRet:
        file_info.print_file_info('Get Customer ID by SIP ID FAIL, lRet is %d.' % lRet)
        return -1

    ulCustomerID = int(lRet[0][0])

    file_info.print_file_info('Get Customer ID by SIP ID(%s) SUCC.' % ulSipID)
    return ulCustomerID

def get_route_param(id):
    '''
    @param id: 网关id
    @todo: 获取路由参数
    '''

    # 获取路由信息
    seqSQLCmd = 'SELECT name,username,password,realm,form_user,form_domain,extension,proxy,reg_proxy,expire_secs,' \
                'CONVERT(register, CHAR(10)) AS register,reg_transport,CONVERT(retry_secs, CHAR(20)) AS retry_secs, ' \
                'CONVERT(cid_in_from,CHAR(20)) AS cid_in_from,contact_params, CONVERT(exten_in_contact, CHAR(20)) ' \
                'AS exten_in_contact,CONVERT(ping, CHAR(20)) AS ping FROM tbl_gateway WHERE id=%d;' % (id)

    results = exec_SQL(seqSQLCmd)
    if -1 == results:
        file_info.print_file_info('Get route param FAIL,results == %d.' % results)
        return -1

    file_info.print_file_info('Get route Param SUCC.')
    return results


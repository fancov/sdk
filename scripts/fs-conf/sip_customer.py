# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: generate the directories and management xml files
'''

import os
import db_conn
import db_config
import customer_file
import file_info

def generate_customer():
    '''
    @todo: generate customer directories and configuration
    @return: customer id list
    '''
    global CONN
    lRet = db_conn.connect_db()
    if -1 == lRet:
        return -1
    
    # 获取所有的customer id
    seqSQLCmd = 'SELECT CONVERT(id, CHAR(10)) AS id FROM tbl_customer'
    file_info.get_cur_runtime_info('seqSQLCmd is %s' % seqSQLCmd)
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        file_info.get_cur_runtime_info('The database connection does not exist.')
        return
    
    # 执行SQL
    ulCount = cursor.execute(seqSQLCmd) # get count of results
    
    # 获得所有执行结果
    results = cursor.fetchall()     # get all record results
    if len(results) == 0:
        file_info.get_cur_runtime_info('len(results) is %d' % len(results))
        return
    file_info.get_cur_runtime_info(results)
    
    # 获取配置文件路径
    seqCfgCurDir = db_config.get_db_param()['fs_config_path']
    if seqCfgCurDir is None or seqCfgCurDir == '' or seqCfgCurDir == -1:
        file_info.get_cur_runtime_info('seqCfgCurDir is %s' % seqCfgCurDir)
        return -1
    if seqCfgCurDir[-1] != '/':
        seqCfgCurDir = seqCfgCurDir + '/'
    seqCfgCurDir = seqCfgCurDir + 'directory/'
    
    # 生成客户目录
    for loop in range(0, ulCount):
        seqCfgPath = seqCfgCurDir + str(results[loop][0]) + '/'
        if os.path.exists(seqCfgPath) is False:
            os.makedirs(seqCfgPath)
        
        # 生成客户文件
        #lRet = customer_file.generate_customer_file(results[loop][0])
        lRet = customer_file.generate_customer_file(results[loop][0])
        if lRet == -1:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
    return 1

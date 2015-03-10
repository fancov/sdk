# coding=utf-8
'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 11th,2015
@todo: operation of customer
'''
import db_config
import file_info
import customer_file
import os

def add_customer(ulCustomerID):
    '''
    @param customer_id: 客户id
    @todo: 增加一个客户
    '''
    if str(ulCustomerID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is', ulCustomerID
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is', ulCustomerID
    
    #生成该账户的配置文件
    customer_file.generate_customer_file(ulCustomerID)
         
def del_customer(ulCustomerID):
    '''
    @param customer_id: 被删除的客户id
    @todo: 删除一个客户
    '''
    if str(ulCustomerID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is', ulCustomerID
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is', ulCustomerID
    seqCfgPath = db_config.get_db_param()['cfg_path']
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
        
    # 删除客户信息
    sqeMgntDir = seqCfgPath + 'directory/' + str(ulCustomerID) + '/'
    seqMgntXml = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'
    if os.path.exists(sqeMgntDir):
        os.system('rm -rf %s' % sqeMgntDir)
    if os.path.exists(seqMgntXml):
        os.system('rm %s' % seqMgntXml)
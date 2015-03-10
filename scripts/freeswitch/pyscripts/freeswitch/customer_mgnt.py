# -*- encoding=utf-8 -*-
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

def add_customer(customer_id):
    '''
    @param customer_id: 客户id
    @todo: 增加一个客户
    '''
    if str(customer_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is', customer_id
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is', customer_id
    
    #生成该账户的配置文件
    customer_file.generate_customer_file(customer_id)
         
def del_customer(customer_id):
    '''
    @param customer_id: 被删除的客户id
    @todo: 删除一个客户
    '''
    if str(customer_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is', customer_id
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is', customer_id
    cfg_path = db_config.get_db_param()['cfg_path']
    if cfg_path[-1] != '/':
        cfg_path = cfg_path + '/'
        
    # 删除客户信息
    mgnt_dir = cfg_path + 'directory/' + str(customer_id) + '/'
    mgnt_xml = cfg_path + 'directory/' + str(customer_id) + '.xml'
    if os.path.exists(mgnt_dir):
        os.system('rm -rf %s' % mgnt_dir)
    if os.path.exists(mgnt_xml):
        os.system('rm %s' % mgnt_xml)
    
    
    
# -*- encoding=utf-8 -*-
'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 6th,2015
@todo: get sip account
'''

import command_param
import sip_maker
import excel
import text
import db
import file_info

def is_list_contains(list, seq):
    '''
    @param list: 列表
    @param seq: 序列
    @todo: 判断列表是否含有某一序列，忽略大小写
    '''
    if list == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'list is', list
        return False
    if seq.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'seq is', seq
        return False
    for item in list:
        if item.lower() == seq.lower():
            return True
    return False
    

#采用列举法生成sip账户
def generate_by_enum(list, customer_id = 'default', path = '../cfg/' ):
    '''
    @param list: sip账户列表
    @param customer_id: 客户id，默认为'default'
    @param path: sip账户的生成路径，不指定则默认为当前父目录下的cfg子目录下
    @todo: 枚举法批量生成sip账户
    '''
    if list == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
    flag_path = 0
    flag_customer = 0
    sip_list = []
    if is_list_contains(list, '-PATH'):
        flag_path = 1
    if is_list_contains(list, '-CUSTOMER'):
        flag_customer = 1
        
    if flag_path == 1 and flag_customer == 1:
        sip_list = list[:-4]
        path_index = list.index('-PATH')
        customer_index = list.index('-CUSTOMER')
        path = list[path_index + 1]
        if path[-1] <> '/':
            path = path + '/'
        customer_id = list[customer_index + 1]
        
    elif flag_path == 1 and flag_customer == 0:
        sip_list = list[:-2]
        path_index = list.index('-PATH')
        path = list[path_index + 1]
        if path[-1] <> '/':
            path = path + '/'
        
    elif flag_customer == 1 and flag_path == 0:
        sip_list = list[:-2] 
        customer_index = list.index('-CUSTOMER')
        customer_id = list[customer_id + 1]
    else:
        sip_list = list
        
    for item in sip_list:
        sip_maker.make_sip(item, customer_id, path)
    
            
def generate_by_range(list, customer_id = 'default', path = '../cfg/' ):
    '''
    @param list: sip账户列表
    @param customer_id: 客户id，默认为'default'
    @param path: sip账户的生成路径，不指定则默认为当前父目录下的cfg子目录下
    @todo: 使用指定特定范围的方法批量生成sip账户
    '''
    if list == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
    flag_path = 0
    flag_customer = 0
    if is_list_contains(list, '-PATH'):
        flag_path = 1
    if is_list_contains(list, '-CUSTOMER'):
        flag_customer = 1
        
    if flag_path == 1 and flag_customer == 1:
        path_index = list.index('-PATH')
        customer_index = list.index('-CUSTOMER')
        path = list[path_index + 1]
        if path[-1] <> '/':
            path = path + '/'
        customer_id = list[customer_index + 1]
        
    elif flag_path == 1 and flag_customer == 0:
        path_index = list.index('-PATH')
        path = list[path_index + 1]
        if path[-1] <> '/':
            path = path + '/'
        
    elif flag_customer == 1 and flag_path == 0:
        customer_index = list.index('-CUSTOMER')
        customer_id = list[customer_id + 1]
      
    sip_list = list[:2]  
    for item in range(int(sip_list[0]), int(sip_list[1]) + 1):
        sip_maker.make_sip(item, customer_id, path)
        
def generate_by_excel(list, customer_id = 'default', path = '../cfg/'):
    '''
    @param list: sip账户列表
    @param customer_id: 客户id，默认为'default'
    @param path: sip账户的生成路径，不指定则默认为当前父目录下的cfg子目录下
    @todo: 通过读取Excel表格的方式批量生成sip账户
    '''
    if list == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
    flag_path = 0
    flag_customer = 0
    if is_list_contains(list, '-PATH'):
        flag_path = 1
    if is_list_contains(list, '-CUSTOMER'):
        flag_customer = 1
        
    if flag_path == 1 and flag_customer == 1:
        path_index = list.index('-PATH')
        customer_index = list.index('-CUSTOMER')
        path = list[path_index + 1]
        if path[-1] <> '/':
            path = path + '/'
        customer_id = list[customer_index + 1]
        
    elif flag_path == 1 and flag_customer == 0:
        path_index = list.index('-PATH')
        path = list[path_index + 1]
        if path[-1] <> '/':
            path = path + '/'
        
    elif flag_customer == 1 and flag_path == 0:
        customer_index = list.index('-CUSTOMER')
        customer_id = list[customer_id + 1] + '/'
        
    sip_list = excel.excel_table_by_col(list[0])
    for loop in range(len(sip_list)):
        sip_maker.make_sip(sip_list[loop], customer_id, path)
    
    
def generate_by_txt(list, customer_id = 'default', path = '../cfg/'):
    '''
    @param list: sip账户列表
    @param customer_id: 客户id，默认为'default'
    @param path: sip账户的生成路径，不指定则默认为当前父目录下的cfg子目录下
    @todo: 通过读取txt文件的方式去批量生成sip账户
    '''
    if list == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
    flag_path = 0
    flag_customer = 0
    if is_list_contains(list, '-PATH'):
        flag_path = 1
    if is_list_contains(list, '-CUSTOMER'):
        flag_customer = 1
        
    if flag_path == 1 and flag_customer == 1:
        path_index = list.index('-PATH')
        customer_index = list.index('-CUSTOMER')
        path = list[path_index + 1]
        if path[-1] <> '/' :
            path = path + '/'
        customer_id = list[customer_index + 1]
        
    elif flag_path == 1 and flag_customer == 0:
        path_index = list.index('-PATH')
        path = list[path_index + 1]
        if path[-1] <> '/':
            path = path + '/'
        
    elif flag_customer == 1 and flag_path == 0:
        customer_index = list.index('-CUSTOMER')
        customer_id = list[customer_id + 1] 
        
    sip_list = text.get_data_from_txt(list[0])
    for item in sip_list:
        sip_maker.make_sip(item, customer_id, path)
        
def generate_by_db(list, customer_id = 'default', path = '../cfg/'):
    '''
    @param list: sip账户列表
    @param customer_id: 客户id，默认为'default'
    @param path: sip账户的生成路径，不指定则默认为当前父目录下的cfg子目录下
    @todo: 通过指定特定的数据库字段方式去批量生成sip账户
    '''
    if list == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
    flag_path = 0
    flag_customer = 0
    if is_list_contains(list, '-PATH'):
        flag_path = 1
    if is_list_contains(list, '-CUSTOMER'):
        flag_customer = 1
        
    if flag_path == 1 and flag_customer == 1:
        path_index = list.index('-PATH')
        customer_index = list.index('-CUSTOMER')
        path = list[path_index + 1]
        if path[-1] <> '/':
            path = path + '/'
        customer_id = list[customer_index + 1]
        
    elif flag_path == 1 and flag_customer == 0:
        path_index = list.index('-PATH')
        path = list[path_index + 1]
        if path[-1] <> '/':
            path = path + '/'
        
    elif flag_customer == 1 and flag_path == 0:
        customer_index = list.index('-CUSTOMER')
        customer_id = list[customer_id + 1]
        
    conn = db.get_db_conn(list[0], list[1], list[2], list[3])
    sip_lists = db.get_field_value(list[4], list[5], conn)
    for loop in range(len(sip_lists)):
        sip_maker.make_sip(int(sip_lists[loop][0]), customer_id, path)

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
    if list == [] is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'list is', list
        return False
    if seq.strip() == '' is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'seq is', seq
        return False
    for item in list:
        if item.lower() == seq.lower():
            return True
    return False
    

#采用列举法生成sip账户
def generate_by_enum(list, customer_id = 'default', path = '../cfg/' ):
    if list == [] is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
    flag_path = 0
    flag_customer = 0
    sip_list = []
    if is_list_contains(list, '-PATH') is True:
        flag_path = 1
    if is_list_contains(list, '-CUSTOMER') is True:
        flag_customer = 1
        
    if flag_path == 1 and flag_customer == 1:
        sip_list = list[:-4]
        path_index = list.index('-PATH')
        customer_index = list.index('-CUSTOMER')
        path = list[path_index + 1]
        if path[-1] <> '/' is True:
            path = path + '/'
        customer_id = list[customer_index + 1]
        
    elif flag_path == 1 and flag_customer == 0:
        sip_list = list[:-2]
        path_index = list.index('-PATH')
        path = list[path_index + 1]
        if path[-1] <> '/' is True:
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
    if list == [] is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
    flag_path = 0
    flag_customer = 0
    if is_list_contains(list, '-PATH') is True:
        flag_path = 1
    if is_list_contains(list, '-CUSTOMER') is True:
        flag_customer = 1
        
    if flag_path == 1 and flag_customer == 1:
        path_index = list.index('-PATH')
        customer_index = list.index('-CUSTOMER')
        path = list[path_index + 1]
        if path[-1] <> '/' is True:
            path = path + '/'
        customer_id = list[customer_index + 1]
        
    elif flag_path == 1 and flag_customer == 0:
        path_index = list.index('-PATH')
        path = list[path_index + 1]
        if path[-1] <> '/' is True:
            path = path + '/'
        
    elif flag_customer == 1 and flag_path == 0:
        customer_index = list.index('-CUSTOMER')
        customer_id = list[customer_id + 1]
      
    sip_list = list[:2]  
    for item in range(int(sip_list[0]), int(sip_list[1]) + 1):
        sip_maker.make_sip(item, customer_id, path)
        
def generate_by_excel(list, customer_id = 'default', path = '../cfg/'):
    if list == [] is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
    flag_path = 0
    flag_customer = 0
    if is_list_contains(list, '-PATH') is True:
        flag_path = 1
    if is_list_contains(list, '-CUSTOMER') is True:
        flag_customer = 1
        
    if flag_path == 1 and flag_customer == 1:
        path_index = list.index('-PATH')
        customer_index = list.index('-CUSTOMER')
        path = list[path_index + 1]
        if path[-1] <> '/' is True:
            path = path + '/'
        customer_id = list[customer_index + 1]
        
    elif flag_path == 1 and flag_customer == 0:
        path_index = list.index('-PATH')
        path = list[path_index + 1]
        if path[-1] <> '/' is True:
            path = path + '/'
        
    elif flag_customer == 1 and flag_path == 0:
        customer_index = list.index('-CUSTOMER')
        customer_id = list[customer_id + 1] + '/'
        
    sip_list = excel.excel_table_by_col(list[0])
    for loop in range(len(sip_list)):
        sip_maker.make_sip(sip_list[loop], customer_id, path)
    
    
def generate_by_txt(list, customer_id = 'default', path = '../cfg/'):
    if list == [] is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
    flag_path = 0
    flag_customer = 0
    if is_list_contains(list, '-PATH') is True:
        flag_path = 1
    if is_list_contains(list, '-CUSTOMER') is True:
        flag_customer = 1
        
    if flag_path == 1 and flag_customer == 1:
        path_index = list.index('-PATH')
        customer_index = list.index('-CUSTOMER')
        path = list[path_index + 1]
        if path[-1] <> '/' is True:
            path = path + '/'
        customer_id = list[customer_index + 1]
        
    elif flag_path == 1 and flag_customer == 0:
        path_index = list.index('-PATH')
        path = list[path_index + 1]
        if path[-1] <> '/' is True:
            path = path + '/'
        
    elif flag_customer == 1 and flag_path == 0:
        customer_index = list.index('-CUSTOMER')
        customer_id = list[customer_id + 1] 
        
    sip_list = text.get_data_from_txt(list[0])
    for item in sip_list:
        sip_maker.make_sip(item, customer_id, path)
        
def generate_by_db(list, customer_id = 'default', path = '../cfg/'):
    if list == [] is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is',list
    flag_path = 0
    flag_customer = 0
    if is_list_contains(list, '-PATH') is True:
        flag_path = 1
    if is_list_contains(list, '-CUSTOMER') is True:
        flag_customer = 1
        
    if flag_path == 1 and flag_customer == 1:
        path_index = list.index('-PATH')
        customer_index = list.index('-CUSTOMER')
        path = list[path_index + 1]
        if path[-1] <> '/' is True:
            path = path + '/'
        customer_id = list[customer_index + 1]
        
    elif flag_path == 1 and flag_customer == 0:
        path_index = list.index('-PATH')
        path = list[path_index + 1]
        if path[-1] <> '/' is True:
            path = path + '/'
        
    elif flag_customer == 1 and flag_path == 0:
        customer_index = list.index('-CUSTOMER')
        customer_id = list[customer_id + 1]
        
    conn = db.get_db_conn(list[0], list[1], list[2], list[3])
    sip_lists = db.get_field_value(list[4], list[5], conn)
    for loop in range(len(sip_lists)):
        sip_maker.make_sip(int(sip_lists[loop][0]), customer_id, path)

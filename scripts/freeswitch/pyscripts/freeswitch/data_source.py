# coding=utf-8

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
def generate_by_enum(list, ulCustomerID = 'default', seqPath = '../cfg/' ):
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
    ulFlagPath = 0
    ulFlagCustomer = 0
    listSIPList = []
    if is_list_contains(list, '-PATH'):
        ulFlagPath = 1
    if is_list_contains(list, '-CUSTOMER'):
        ulFlagCustomer = 1
        
    if ulFlagPath == 1 and ulFlagCustomer == 1:
        listSIPList = list[:-4]
        ulPathIndex = list.index('-PATH')
        ulCustomerIndex = list.index('-CUSTOMER')
        seqPath = list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        ulCustomerID = list[ulCustomerIndex + 1]
        
    elif ulFlagPath == 1 and ulFlagCustomer == 0:
        listSIPList = list[:-2]
        ulPathIndex = list.index('-PATH')
        seqPath = list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        
    elif ulFlagCustomer == 1 and ulFlagPath == 0:
        listSIPList = list[:-2] 
        ulCustomerIndex = list.index('-CUSTOMER')
        ulCustomerID = list[ulCustomerID + 1]
    else:
        listSIPList = list
        
    for item in listSIPList:
        sip_maker.make_sip(item, ulCustomerID, seqPath)
    
            
def generate_by_range(list, ulCustomerID = 'default', seqPath = '../cfg/' ):
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
    ulFlagPath = 0
    ulFlagCustomer = 0
    if is_list_contains(list, '-PATH'):
        ulFlagPath = 1
    if is_list_contains(list, '-CUSTOMER'):
        ulFlagCustomer = 1
        
    if ulFlagPath == 1 and ulFlagCustomer == 1:
        ulPathIndex = list.index('-PATH')
        ulCustomerIndex = list.index('-CUSTOMER')
        seqPath = list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        ulCustomerID = list[ulCustomerIndex + 1]
        
    elif ulFlagPath == 1 and ulFlagCustomer == 0:
        ulPathIndex = list.index('-PATH')
        seqPath = list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        
    elif ulFlagCustomer == 1 and ulFlagPath == 0:
        ulCustomerIndex = list.index('-CUSTOMER')
        ulCustomerID = list[ulCustomerID + 1]
      
    listSIPList = list[:2]  
    for item in range(int(listSIPList[0]), int(listSIPList[1]) + 1):
        sip_maker.make_sip(item, ulCustomerID, seqPath)
        
def generate_by_excel(list, ulCustomerID = 'default', seqPath = '../cfg/'):
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
    ulFlagPath = 0
    ulFlagCustomer = 0
    if is_list_contains(list, '-PATH'):
        ulFlagPath = 1
    if is_list_contains(list, '-CUSTOMER'):
        ulFlagCustomer = 1
        
    if ulFlagPath == 1 and ulFlagCustomer == 1:
        ulPathIndex = list.index('-PATH')
        ulCustomerIndex = list.index('-CUSTOMER')
        seqPath = list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        ulCustomerID = list[ulCustomerIndex + 1]
        
    elif ulFlagPath == 1 and ulFlagCustomer == 0:
        ulPathIndex = list.index('-PATH')
        seqPath = list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        
    elif ulFlagCustomer == 1 and ulFlagPath == 0:
        ulCustomerIndex = list.index('-CUSTOMER')
        ulCustomerID = list[ulCustomerID + 1] + '/'
        
    listSIPList = excel.excel_table_by_col(list[0])
    for loop in range(len(listSIPList)):
        sip_maker.make_sip(listSIPList[loop], ulCustomerID, seqPath)
    
    
def generate_by_txt(list, ulCustomerID = 'default', seqPath = '../cfg/'):
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
    ulFlagPath = 0
    ulFlagCustomer = 0
    if is_list_contains(list, '-PATH'):
        ulFlagPath = 1
    if is_list_contains(list, '-CUSTOMER'):
        ulFlagCustomer = 1
        
    if ulFlagPath == 1 and ulFlagCustomer == 1:
        ulPathIndex = list.index('-PATH')
        ulCustomerIndex = list.index('-CUSTOMER')
        seqPath = list[ulPathIndex + 1]
        if seqPath[-1] <> '/' :
            seqPath = seqPath + '/'
        ulCustomerID = list[ulCustomerIndex + 1]
        
    elif ulFlagPath == 1 and ulFlagCustomer == 0:
        ulPathIndex = list.index('-PATH')
        seqPath = list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        
    elif ulFlagCustomer == 1 and ulFlagPath == 0:
        ulCustomerIndex = list.index('-CUSTOMER')
        ulCustomerID = list[ulCustomerID + 1] 
        
    listSIPList = text.get_data_from_txt(list[0])
    for item in listSIPList:
        sip_maker.make_sip(item, ulCustomerID, seqPath)
        
def generate_by_db(list, ulCustomerID = 'default', seqPath = '../cfg/'):
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
    ulFlagPath = 0
    ulFlagCustomer = 0
    if is_list_contains(list, '-PATH'):
        ulFlagPath = 1
    if is_list_contains(list, '-CUSTOMER'):
        ulFlagCustomer = 1
        
    if ulFlagPath == 1 and ulFlagCustomer == 1:
        ulPathIndex = list.index('-PATH')
        ulCustomerIndex = list.index('-CUSTOMER')
        seqPath = list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        ulCustomerID = list[ulCustomerIndex + 1]
        
    elif ulFlagPath == 1 and ulFlagCustomer == 0:
        ulPathIndex = list.index('-PATH')
        seqPath = list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        
    elif ulFlagCustomer == 1 and ulFlagPath == 0:
        ulCustomerIndex = list.index('-CUSTOMER')
        ulCustomerID = list[ulCustomerID + 1]
        
    conn = db.get_db_conn(list[0], list[1], list[2], list[3])
    listSIPLists = db.get_field_value(list[4], list[5], conn)
    for loop in range(len(listSIPLists)):
        sip_maker.make_sip(int(listSIPLists[loop][0]), ulCustomerID, seqPath)

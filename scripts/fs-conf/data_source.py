# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 6th,2015
@todo: get sip account
'''

import sip_maker
import excel
import text
import db
import file_info

def is_list_contains(_list, seq):
    '''
    @param list: 列表
    @param seq: 序列
    @todo: 判断列表是否含有某一序列，忽略大小写
    '''
    if _list == []:
        file_info.get_cur_runtime_info(_list)
        return False
    if seq.strip() == '':
        file_info.get_cur_runtime_info('seq is %s' % seq)
        return False
    for item in list:
        if item.lower() == seq.lower():
            return True
    return False
    

#采用列举法生成sip账户
def generate_by_enum(_list, ulCustomerID = 'default', seqPath = '../cfg/' ):
    '''
    @param list: sip账户列表
    @param customer_id: 客户id，默认为'default'
    @param path: sip账户的生成路径，不指定则默认为当前父目录下的cfg子目录下
    @todo: 枚举法批量生成sip账户
    '''
    if _list == []:
        file_info.get_cur_runtime_info(_list)
        return None
    
    file_info.get_cur_runtime_info(_list)
    ulFlagPath = 0
    ulFlagCustomer = 0
    listSIPList = []
    if is_list_contains(_list, '-PATH'):
        ulFlagPath = 1
    if is_list_contains(_list, '-CUSTOMER'):
        ulFlagCustomer = 1
        
    if ulFlagPath == 1 and ulFlagCustomer == 1:
        listSIPList = _list[:-4]
        ulPathIndex = _list.index('-PATH')
        ulCustomerIndex = _list.index('-CUSTOMER')
        seqPath = list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        ulCustomerID = _list[ulCustomerIndex + 1]
        
    elif ulFlagPath == 1 and ulFlagCustomer == 0:
        listSIPList = _list[:-2]
        ulPathIndex = _list.index('-PATH')
        seqPath = _list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        
    elif ulFlagCustomer == 1 and ulFlagPath == 0:
        listSIPList = _list[:-2] 
        ulCustomerIndex = _list.index('-CUSTOMER')
        ulCustomerID = _list[ulCustomerID + 1]
    else:
        listSIPList = _list
        
    for item in listSIPList:
        lRet = sip_maker.make_sip(item, ulCustomerID, seqPath)
        if -1 == lRet:
            return -1
    
    return 1
    
            
def generate_by_range(_list, ulCustomerID = 'default', seqPath = '../cfg/' ):
    '''
    @param list: sip账户列表
    @param customer_id: 客户id，默认为'default'
    @param path: sip账户的生成路径，不指定则默认为当前父目录下的cfg子目录下
    @todo: 使用指定特定范围的方法批量生成sip账户
    '''
    if _list == []:
        file_info.get_cur_runtime_info(_list)
        return None
    file_info.get_cur_runtime_info(_list)
    ulFlagPath = 0
    ulFlagCustomer = 0
    if is_list_contains(_list, '-PATH'):
        ulFlagPath = 1
    if is_list_contains(_list, '-CUSTOMER'):
        ulFlagCustomer = 1
        
    if ulFlagPath == 1 and ulFlagCustomer == 1:
        ulPathIndex = _list.index('-PATH')
        ulCustomerIndex = _list.index('-CUSTOMER')
        seqPath = list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        ulCustomerID = _list[ulCustomerIndex + 1]
        
    elif ulFlagPath == 1 and ulFlagCustomer == 0:
        ulPathIndex = _list.index('-PATH')
        seqPath = list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        
    elif ulFlagCustomer == 1 and ulFlagPath == 0:
        ulCustomerIndex = _list.index('-CUSTOMER')
        ulCustomerID = _list[ulCustomerID + 1]
      
    listSIPList = _list[:2]  
    for item in range(int(listSIPList[0]), int(listSIPList[1]) + 1):
        lRet = sip_maker.make_sip(item, ulCustomerID, seqPath)
        if -1 == lRet:
            return -1
    
    return 1
        
def generate_by_excel(_list, ulCustomerID = 'default', seqPath = '../cfg/'):
    '''
    @param list: sip账户列表
    @param customer_id: 客户id，默认为'default'
    @param path: sip账户的生成路径，不指定则默认为当前父目录下的cfg子目录下
    @todo: 通过读取Excel表格的方式批量生成sip账户
    '''
    if _list == []:
        file_info.get_cur_runtime_info(_list)
        return None
    file_info.get_cur_runtime_info(_list)
    ulFlagPath = 0
    ulFlagCustomer = 0
    if is_list_contains(_list, '-PATH'):
        ulFlagPath = 1
    if is_list_contains(_list, '-CUSTOMER'):
        ulFlagCustomer = 1
        
    if ulFlagPath == 1 and ulFlagCustomer == 1:
        ulPathIndex = _list.index('-PATH')
        ulCustomerIndex = _list.index('-CUSTOMER')
        seqPath = list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        ulCustomerID = list[ulCustomerIndex + 1]
        
    elif ulFlagPath == 1 and ulFlagCustomer == 0:
        ulPathIndex = _list.index('-PATH')
        seqPath = _list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        
    elif ulFlagCustomer == 1 and ulFlagPath == 0:
        ulCustomerIndex = _list.index('-CUSTOMER')
        ulCustomerID = _list[ulCustomerID + 1] + '/'
        
    listSIPList = excel.excel_table_by_col(_list[0])
    if -1 == listSIPList:
        file_info.get_cur_runtime_info(listSIPList)
        return -1
    for loop in range(len(listSIPList)):
        lRet = sip_maker.make_sip(listSIPList[loop], ulCustomerID, seqPath)
        if lRet == -1:
            return -1
    
    return 1
    
    
def generate_by_txt(_list, ulCustomerID = 'default', seqPath = '../cfg/'):
    '''
    @param list: sip账户列表
    @param customer_id: 客户id，默认为'default'
    @param path: sip账户的生成路径，不指定则默认为当前父目录下的cfg子目录下
    @todo: 通过读取txt文件的方式去批量生成sip账户
    '''
    if _list == []:
        file_info.get_cur_runtime_info(_list)
        return None
    file_info.get_cur_runtime_info(_list)
    ulFlagPath = 0
    ulFlagCustomer = 0
    if is_list_contains(_list, '-PATH'):
        ulFlagPath = 1
    if is_list_contains(_list, '-CUSTOMER'):
        ulFlagCustomer = 1
        
    if ulFlagPath == 1 and ulFlagCustomer == 1:
        ulPathIndex = _list.index('-PATH')
        ulCustomerIndex = _list.index('-CUSTOMER')
        seqPath = _list[ulPathIndex + 1]
        if seqPath[-1] <> '/' :
            seqPath = seqPath + '/'
        ulCustomerID = _list[ulCustomerIndex + 1]
        
    elif ulFlagPath == 1 and ulFlagCustomer == 0:
        ulPathIndex = _list.index('-PATH')
        seqPath = _list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        
    elif ulFlagCustomer == 1 and ulFlagPath == 0:
        ulCustomerIndex = _list.index('-CUSTOMER')
        ulCustomerID = _list[ulCustomerID + 1] 
        
    listSIPList = text.get_data_from_txt(_list[0])
    if -1 == listSIPList:
        return -1
    for item in listSIPList:
        lRet = sip_maker.make_sip(item, ulCustomerID, seqPath)
        if -1 == lRet:
            return -1
    
    return 1
        
def generate_by_db(_list, ulCustomerID = 'default', seqPath = '../cfg/'):
    '''
    @param list: sip账户列表
    @param customer_id: 客户id，默认为'default'
    @param path: sip账户的生成路径，不指定则默认为当前父目录下的cfg子目录下
    @todo: 通过指定特定的数据库字段方式去批量生成sip账户
    '''
    if _list == []:
        file_info.get_cur_runtime_info(_list)
        return None
    file_info.get_cur_runtime_info(_list)
    ulFlagPath = 0
    ulFlagCustomer = 0
    if is_list_contains(_list, '-PATH'):
        ulFlagPath = 1
    if is_list_contains(_list, '-CUSTOMER'):
        ulFlagCustomer = 1
        
    if ulFlagPath == 1 and ulFlagCustomer == 1:
        ulPathIndex = _list.index('-PATH')
        ulCustomerIndex = _list.index('-CUSTOMER')
        seqPath = _list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        ulCustomerID = _list[ulCustomerIndex + 1]
        
    elif ulFlagPath == 1 and ulFlagCustomer == 0:
        ulPathIndex = _list.index('-PATH')
        seqPath = _list[ulPathIndex + 1]
        if seqPath[-1] <> '/':
            seqPath = seqPath + '/'
        
    elif ulFlagCustomer == 1 and ulFlagPath == 0:
        ulCustomerIndex = _list.index('-CUSTOMER')
        ulCustomerID = _list[ulCustomerID + 1]
        
    conn = db.get_db_conn(_list[0], _list[1], _list[2], _list[3])
    if conn is None:
        file_info.get_cur_runtime_info('database cannot be connected.')
        return -1
    listSIPLists = db.get_field_value(_list[4], _list[5], conn)
    if listSIPLists == []:
        file_info.get_cur_runtime_info(listSIPLists)
        return -1
    for loop in range(len(listSIPLists)):
        lRet = sip_maker.make_sip(int(listSIPLists[loop][0]), ulCustomerID, seqPath)
        if lRet == -1:
            return -1
    
    return 1

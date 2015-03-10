# -*- encoding=utf-8 -*-

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 13th,2015
@todo: operation of customer
'''

import file_info
import sip_customer
import sip_mgnt
import group_mgnt
import customer_mgnt

def config_operation(op_obj, op_type, obj_id, param1, param2):
    '''
    @param op_obj: 表示操作对象，为sip账户、座席组、客户中的某一个对象
    @param op_type: 表示操作类型，包括增加、删除、更改、Load中的一个
    @param obj_id: 操作对象的id
    @param param1: 待定参数1，由操作对象和操作类型共同决定
    @param param2: 待定参数2，由操作对象和操作类型共同决定
    @todo: 操作配置文件，具体操作由操作对象和操作类型共同决定
    '''
    if str(op_obj).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'op_obj is', op_obj
        return None 
    if str(op_type).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'op_type is', op_type
        return None
    if str(obj_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'obj_id is', obj_id
        return None
    
    # 如果op_type值为0, 则操作类型为Load，那么加载所有的配置，其他参数均为无效参数
    if str(op_type).strip() == '0': #Load
        op_obj = -1
        obj_id = -1
        param1 = None  #set it to be invalid data
        param2 = None  #set it to be invalid data
    
        sip_customer.generate_customer()  #测试成功
        return None
    
    # 如果op_type值为1， 则表示增加操作
    elif str(op_type).strip() == '1':  #Add
        param1 = None  #set it to be invalid data
        param2 = None  #set it to be invalid data
        # 如果op_obj值为0，则表示增加一个sip账户，obj_id代表sip账户id
        if str(op_obj).strip() == '0':  #Add sip
            sip_mgnt.add_sip(obj_id)  #测试成功
            return
        # 如果op_obj为1，则表示增加一个座席组，obj_id代表座席组id
        elif str(op_obj).strip() == '1':  #Add group
            group_mgnt.add_group(obj_id) # 测试成功
            return
        # 如果op_obj为2，则表示增加一个客户，obj_id代表客户id
        elif str(op_obj).strip() == '2':  #Add customer
            customer_mgnt.add_customer(obj_id) # 测试成功
            return
        
    # 如果op_type值为2，则表示删除操作
    elif str(op_type).strip() == '2': #Delete
        # 如果op_obj为0，表示删除一个sip账户，那么参数param2必须为"delete"，参数param1为sip所属客户的id字符串，obj_id为sip账户id
        if str(op_obj).strip() == '0': #Delete sip
            if param2.lower().strip() == 'delete':
                customer_id = int(param1)
                sip_mgnt.del_sip_from_group(obj_id, customer_id) # 测试成功
                return
        # 如果op_obj为1，则表示删除一个座席组，那么参数param2必须为"delete"，参数param1为该座席组所属客户的id字符串，obj_id为座席组id
        elif str(op_obj).strip() == '1':#Delete group
            if param2.lower().strip() == 'delete':
                customer_id = int(param1)
                group_mgnt.del_group(obj_id, customer_id)  # 测试成功
                return     
        # 如果op_obj为2，则表示删除一个客户，那么param2必须为"delete"，参数param1必须为None，obj_id为客户id  
        elif str(op_obj).strip() == '2':#Delete customer
            if param2.lower().strip() == 'delete' and param1 == None:
                customer_mgnt.del_customer(obj_id)  # 测试成功
                return
    # 如果op_type值是3，那么表示更新操作          
    elif str(op_type).strip() == '3': # Update
        # 如果op_obj为0，则表示更新sip账户的相关内容
        if str(op_obj).strip() == '0': #Update sip
            # 如果param2为字符串"change"，则表示将sip账户更改到新的座席组，obj_id为sip账户id，param1为新的座席组id字符串
            if param2.lower().strip() == 'change':
                new_grp_id = int(param1)
                sip_mgnt.change_agent_group(obj_id, new_grp_id)   # 测试成功
                return
            # 如果param1不为空，且param2不是"change"，那么表示修改sip账户的参数值，其中obj_id为sip账户id，param1为配置文件的参数名
            # param2为配置文件的参数名对应的参数值
            elif param1.strip() != '':
                sip_mgnt.modify_param_value(obj_id, param1, param2) # 测试成功
                return
        # 如果op_obj为1，则表示更新group相关内容
        elif str(op_obj).strip() == '1': #Update group
            # 如果param2为"change"，那么param1为新的座席组id字符串，obj_id是原座席组id
            if param2.lower().strip() == 'change':
                new_grp_id = int(param1)
                group_mgnt.modify_group_name(obj_id, new_grp_id) # 测试成功
                return
            
if __name__ == "__main__":
    config_operation(1, 3, 2, '3', 'Change')

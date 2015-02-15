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
    if str(op_obj).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'op_obj is', op_obj
        return None 
    if str(op_type).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'op_type is', op_type
        return None
    if str(obj_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'obj_id is', obj_id
        return None
    
    if str(op_type).strip() == '0': #Load
        op_obj = -1
        obj_id = -1
        param1 = None
        param2 = None
        sip_customer.generate_customer()
        return None
    elif str(op_type).strip() == '1':#Add
        if str(op_obj).strip() == '0': #Add sip
            sip_mgnt.add_sip(obj_id)
            return
        elif str(op_obj).strip() == '1':#Add group
            group_mgnt.add_group(obj_id)
            return
        elif str(op_obj).strip() == '2':#Add customer
            customer_mgnt.add_customer(obj_id)
            return
    elif str(op_type).strip() == '2': #Delete
        if str(op_obj).strip() == '0': #Delete sip
            if param2.lower().strip() == 'delete':
                customer_id = int(param1)
                sip_mgnt.del_sip_from_group(obj_id, customer_id)
                return
        elif str(op_obj).strip() == '1':#Delete group
            if param2.lower().strip() == 'delete':
                customer_id = int(param1)
                group_mgnt.del_group(obj_id, customer_id)
                return       
        elif str(op_obj).strip() == '2':#Delete customer
            if param2.lower().strip() == 'delete' and param1 == None:
                customer_mgnt.del_customer(obj_id)
                return
                
    elif str(op_type).strip() == '3': # Update
        if str(op_obj).strip() == '0': #Update sip
            if param2.lower().strip() == 'change':
                new_grp_id = int(param1)
                sip_mgnt.change_agent_group(obj_id, new_grp_id)
                return
            elif param1.strip() != '':
                sip_mgnt.modify_param_value(obj_id, param1, param2)
                return
        elif str(op_obj).strip() == '1': #Update group
            if param2.lower().strip() == 'change':
                new_grp_id = int(param1)
                group_mgnt.modify_group_name(obj_id, new_grp_id)
                return
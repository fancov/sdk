# coding=utf-8

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

def config_operation(ulOpObj, ulOpType, ulObjID, seqParam1, seqParam2):
    '''
    @param op_obj: 表示操作对象，为sip账户、座席组、客户中的某一个对象
    @param op_type: 表示操作类型，包括增加、删除、更改、Load中的一个
    @param obj_id: 操作对象的id
    @param param1: 待定参数1，由操作对象和操作类型共同决定
    @param param2: 待定参数2，由操作对象和操作类型共同决定
    @todo: 操作配置文件，具体操作由操作对象和操作类型共同决定
    '''
    if str(ulOpObj).strip() == '':
        file_info.get_cur_runtime_info('ulOpObj is %s' % str(ulOpObj))
        return -1
    if str(ulOpType).strip() == '':
        file_info.get_cur_runtime_info('ulOpType is %s' % str(ulOpType))
        return -1
    if str(ulObjID).strip() == '':
        file_info.get_cur_runtime_info('ulObjID is %s' % str(ulObjID))
        return -1
    
    # 如果op_type值为0, 则操作类型为Load，那么加载所有的配置，其他参数均为无效参数
    if str(ulOpType).strip() == '0': #Load
        ulOpObj = -1
        ulObjID = -1
        seqParam1 = None  #set it to be invalid data
        seqParam2 = None  #set it to be invalid data
    
        lRet = sip_customer.generate_customer()  #测试成功
        if -1 == lRet:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
        return 1
    
    # 如果op_type值为1， 则表示增加操作
    elif str(ulOpType).strip() == '1':  #Add
        seqParam1 = None  #set it to be invalid data
        seqParam2 = None  #set it to be invalid data
        # 如果op_obj值为0，则表示增加一个sip账户，obj_id代表sip账户id
        if str(ulOpObj).strip() == '0':  #Add sip
            lRet = sip_mgnt.add_sip(ulObjID)  #测试成功
            if -1 == lRet:
                file_info.get_cur_runtime_info('lRet is %d' % lRet)
                return -1
            return 1
        # 如果op_obj为1，则表示增加一个座席组，obj_id代表座席组id
        elif str(ulOpObj).strip() == '1':  #Add group
            lRet = group_mgnt.add_group(ulObjID) # 测试成功
            if -1 == lRet:
                file_info.get_cur_runtime_info('lRet is %d' % lRet)
                return -1
            return 1
        # 如果op_obj为2，则表示增加一个客户，obj_id代表客户id
        elif str(ulOpObj).strip() == '2':  #Add customer
            lRet = customer_mgnt.add_customer(ulObjID) # 测试成功
            if -1 == lRet:
                file_info.get_cur_runtime_info('lRet is %d' % lRet)
                return -1
            return 1
        
    # 如果op_type值为2，则表示删除操作
    elif str(ulOpType).strip() == '2': #Delete
        # 如果op_obj为0，表示删除一个sip账户，那么参数param2必须为"delete"，参数param1为sip所属客户的id字符串，obj_id为sip账户id
        if str(ulOpObj).strip() == '0': #Delete sip
            if seqParam2.lower().strip() == 'delete':
                ulCustomerID = int(seqParam1)
                lRet = sip_mgnt.del_sip(ulObjID, ulCustomerID) # 测试成功
                if -1 == lRet:
                    file_info.get_cur_runtime_info('lRet is %d' % lRet)
                    return -1
                return 1
        # 如果op_obj为1，则表示删除一个座席组，那么参数param2必须为"delete"，参数param1为该座席组所属客户的id字符串，obj_id为座席组id
        elif str(ulOpObj).strip() == '1':#Delete group
            if seqParam2.lower().strip() == 'delete':
                ulCustomerID = int(seqParam1)
                lRet = group_mgnt.del_group(ulObjID, ulCustomerID)  # 测试成功
                if -1 == lRet:
                    file_info.get_cur_runtime_info('lRet is %d' % lRet)
                    return -1
                return 1
        # 如果op_obj为2，则表示删除一个客户，那么param2必须为"delete"，参数param1必须为None，obj_id为客户id  
        elif str(ulOpObj).strip() == '2':#Delete customer
            if seqParam2.lower().strip() == 'delete' and seqParam1 == None:
                lRet = customer_mgnt.del_customer(ulObjID)  # 测试成功
                if -1 == lRet:
                    file_info.get_cur_runtime_info('lRet is %d' % lRet)
                    return -1
                return 1
    # 如果op_type值是3，那么表示更新操作          
    elif str(ulOpType).strip() == '3': # Update
        # 如果op_obj为0，则表示更新sip账户的相关内容
        if str(ulOpObj).strip() == '0': #Update sip
            # 如果param2为字符串"change"，则表示将sip账户更改到新的座席组，obj_id为sip账户id，param1为新的座席组id字符串
            if seqParam2.lower().strip() == 'change':
                ulNewGroupID = int(seqParam1)
                lRet = sip_mgnt.change_agent_group(ulObjID, ulNewGroupID)   # 测试成功
                if -1 == lRet:
                    file_info.get_cur_runtime_info('lRet is %d' % lRet)
                    return -1
                return 1
            # 如果param1不为空，且param2不是"change"，那么表示修改sip账户的参数值，其中obj_id为sip账户id，param1为配置文件的参数名
            # param2为配置文件的参数名对应的参数值
            elif seqParam1.strip() != '':
                lRet = sip_mgnt.modify_param_value(ulObjID, seqParam1, seqParam2) # 测试成功
                if -1 == lRet:
                    file_info.get_cur_runtime_info('lRet is %d' % lRet)
                    return -1
                return 1
        # 如果op_obj为1，则表示更新group相关内容
        elif str(ulOpObj).strip() == '1': #Update group
            # 如果param2为"change"，那么param1为新的座席组id字符串，obj_id是原座席组id
            if seqParam2.lower().strip() == 'change':
                ulNewGroupID = int(seqParam1)
                lRet = group_mgnt.modify_group_name(ulObjID, ulNewGroupID) # 测试成功
                if -1 == lRet:
                    file_info.get_cur_runtime_info('lRet is %d' % lRet)
                    return -1
                return 1

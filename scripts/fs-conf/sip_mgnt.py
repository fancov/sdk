# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 10th,2015
@todo: operation of sip account
'''
import db_conn
import file_info
import db_config
import os
import sip_maker

def add_sip(ulSIPID):
    '''
    @param: sip_id  sip账户id
    @todo: 添加一个sip账户
    '''
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s', str(ulSIPID))
        return -1
    
    #获取sip账户的group id
    ulGroupID = get_grp_id(ulSIPID)
    if ulGroupID == -1:
        file_info.get_cur_runtime_info('ulGroupID is %s' % str(ulGroupID))
        return -1
    
    for item in ulGroupID[0]:
        if str(item).strip() == '0':
            # 删除group id为0的项
            del item
        else:
            # 将sip账户添加到座席组
            add_sip_to_group(ulSIPID, int(item))
            

def get_grp_id(ulSIPID):
    '''
    @param: sip_id sip账户id
    @todo: 获取sip账户所在的座席组id列表
    '''
    
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1
    
    global CONN
    lRet = db_conn.connect_db()
    if -1 == lRet:
        file_info.get_cur_runtime_info('lRet is %d' % lRet)
        return -1
    
    #查找所有sip账户id为sip_id的座席组id
    seqSQLCmd = 'SELECT CONVERT(tbl_agent.group1_id,CHAR(10)) AS group1_id,CONVERT(tbl_agent.group2_id,CHAR(10)) AS group2_id FROM tbl_agent WHERE tbl_agent.sip_id = %d' % (ulSIPID)
    file_info.get_cur_runtime_info('seqSQLCmd is %s' % seqSQLCmd)
    
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        file_info.get_cur_runtime_info('The database connection does not exist.')
        return -1
    cursor.execute(seqSQLCmd)
    ulGroupID = cursor.fetchall()
    if len(ulGroupID) == 0:
        file_info.get_cur_runtime_info('len(ulGroupID) is %d' % len(ulGroupID))
        return -1
    db_conn.CONN.close()
    file_info.get_cur_runtime_info('ulGroupID is %s' % str(ulGroupID))
    
    #返回座席组id列表，因为一个sip账户很可能属于多个账户
    return ulGroupID

def get_customer_id(ulSIPID, ulGroupID):
    '''
    @param: sip_id sip账户id; grp_id sip账户所在的座席组id
    @todo: 根据sip的id获取它所属客户的id
    '''
    
    global CONN
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1
    if str(ulGroupID).strip() == '':
        file_info.get_cur_runtime_info('ulGroupID is %s' % str(ulGroupID))
        return -1
    
    lRet = db_conn.connect_db()
    if lRet == -1:
        file_info.get_cur_runtime_info('lRet is %d' % lRet)
        return -1
    # 获取座席组id为grp_id所属的客户id
    seqSQLCmd = 'SELECT CONVERT(tbl_group.customer_id,CHAR(10)) AS customer_id FROM tbl_group WHERE tbl_group.id = %s' % (ulGroupID)
    file_info.get_cur_runtime_info('seqSQLCmd is %s' % seqSQLCmd)
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        file_info.get_cur_runtime_info('The database connection does not exist.')
        return -1
    cursor.execute(seqSQLCmd)
    ulCustomerID = cursor.fetchall()
    db_conn.CONN.close()
    if len(ulCustomerID) == 0:
        file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
        return -1
    file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
    return ulCustomerID

def get_agent_by_sip(ulSIPID):
    '''
    @param: sip_id sip账户id
    @todo: 获取sip账户对应的座席组id
    '''
    global CONN
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1
    lRet = db_conn.connect_db()
    if -1 == lRet:
        file_info.get_cur_runtime_info('lRet is %d' % lRet)
        return -1
    
    # 获取sip id是sip_id的agent id
    seqSQLCmd = 'SELECT CONVERT(id, CHAR(10)) AS id FROM tbl_agent WHERE tbl_agent.sip_id = %d' % (ulSIPID)
    file_info.get_cur_runtime_info('seqSQLCmd is %s' % seqSQLCmd)
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        file_info.get_cur_runtime_info('The database connection does not exist.')
        return -1
    cursor.execute(seqSQLCmd)
    result = cursor.fetchall()
    if len(result) == 0:
        file_info.get_cur_runtime_info('len(result) is %d' % len(result))
        return -1
    db_conn.CONN.close()
    file_info.get_cur_runtime_info(result)
    return result

def add_sip_to_group(ulSIPID, ulGroupID):
    '''
    @param: sip_id sip账户id; grp_id sip账户所属座席组id
    @todo: 将sip账户添加到group中
    '''
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1
    if str(ulGroupID).strip() == '':
        file_info.get_cur_runtime_info('ulGroupID is %s' % str(ulGroupID))
        return -1
    result = get_customer_id(ulSIPID, ulSIPID)
    if result is None or result == -1:
        file_info.get_cur_runtime_info(result)
        return -1
    file_info.get_cur_runtime_info(result)
    
    #获取客户id
    ulCustomerID = result[0][0]
    #获取座席组id
    ulAgentID = get_agent_by_sip(ulSIPID)[0][0]
    if -1 == ulAgentID:
        file_info.get_cur_runtime_info('ulAgentID is %s' % str(ulAgentID))
        return -1
    
    seqCfgPath = db_config.get_db_param()['cfg_path']
    if seqCfgPath == -1:
        file_info.get_cur_runtime_info('seqCfgPath is %s' % str(seqCfgPath))
        return -1
    #构造以'/'结尾的路径字符串
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    file_info.get_cur_runtime_info('seqCfgPath is %s' % str(seqCfgPath))
    
    # 客户配置文件所在路径设置:假如customer id是1，那么客户配置文件是 %cfg_path%/directory/1.xml
    seqCustomerPath = seqCfgPath + 'directory/' + ulCustomerID + '.xml'
    if os.path.exists(seqCustomerPath) is False:
        file_info.get_cur_runtime_info('seqCustomerPath is %s' % seqCustomerPath)
        return -1
    file_info.get_cur_runtime_info('seqCustomerPath is %s' % seqCustomerPath)
    
    # sip账户配置文件路径设置：假设customer id是1，座席组id是100，那么sip账户配置文件是 %cfg_path%/directory/1/100.xml
    seqSIPPath = seqCfgPath + 'directory/' + ulCustomerID + '/' + ulAgentID + '.xml'
    file_info.get_cur_runtime_info('seqSIPPath is %s' % seqSIPPath)
    
    # 读取配置文件，按行读取并组成Python列表
    seqMgntText = open(seqCustomerPath, 'r').readlines()
    if seqMgntText == []:
        file_info.get_cur_runtime_info('mgnt_text is empty.')
        return -1
    file_info.get_cur_runtime_info('seqMgntText is %s' % seqMgntText)
    
    # 构造新的sip配置字符串并添加
    seqAddText = '     <user id=\"%s\" type=\"pointer\"/>\n' % (ulAgentID)
    if seqAddText in seqMgntText:
        return -1
    file_info.get_cur_runtime_info('seqAddText is %s' % seqAddText)
    try:
        ulMgntIndex = seqMgntText.index('   <group name=\"%s-%s\">\n' % (ulCustomerID, ulSIPID))
        if ulMgntIndex < 0:
            file_info.get_cur_runtime_info('mgnt_text does not exist.')
            return  -1
        seqMgntText.insert(ulMgntIndex + 2, seqAddText)
        open(seqCustomerPath, 'w').writelines(seqMgntText)
        # 生成sip账户
        lRet = sip_maker.make_sip(ulSIPID, ulCustomerID, seqSIPPath)
        if lRet == -1:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
        return 1
    except Exception as e:
        print str(e)

def del_sip_from_group(ulAgentID, ulCustomerID):
    '''
    @param: agent_id 座席组id; customer_id客户id
    @todo: 将一个sip账户从一个座席组中删除
    '''
    if str(ulAgentID).strip() == '':
        file_info.get_cur_runtime_info('ulAgentID is %s' % str(ulAgentID))
        return -1
    if str(ulCustomerID).strip() == '':
        file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
        return -1
    seqCfgPath = db_config.get_db_param()['fs_config_path']
    if -1 == seqCfgPath:
        file_info.get_cur_runtime_info('seqCfgPath is %d' % seqCfgPath)
        return -1
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    seqSIPPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '/' + str(ulAgentID) + '.xml'
    if os.path.exists(seqSIPPath):
        os.remove(seqSIPPath)
        
    seqMgntPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'
    listMgntText = open(seqMgntPath,'r').readlines()
    if listMgntText == []:
        file_info.get_cur_runtime_info(listMgntText)
        return -1
    ulIndex = listMgntText.index('     <user id=\"%d\" type=\"pointer\"/>\n' % ulAgentID)
    if ulIndex < 0:
        file_info.get_cur_runtime_info('The index does not exist')
        return -1
    del listMgntText[ulIndex]
    open(seqMgntPath, 'w').writelines(listMgntText)
    return 1
   
def change_agent_group(ulSIPID, ulNewGroupID):
    '''
    @param sip_id: sip账户id
    @param new_grp_id: 新的座席组id
    @todo:将sip账户更改到另外一座席组
    '''
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1
    if str(ulNewGroupID).strip() == '':
        file_info.get_cur_runtime_info('ulNewGroupID is %s' % str(ulNewGroupID))
        return -1
    
    # 获取到座席组id
    result = get_agent_by_sip(ulSIPID)
    if -1 == result:
        file_info.get_cur_runtime_info(result)
        return -1
    file_info.get_cur_runtime_info(result)
    
    #获取到customer id
    ulCustomerID = get_customer_id(ulSIPID, ulNewGroupID)[0][0]
    if -1 == ulCustomerID:
        file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
        return -1
    ulAgentID = result[0][0]
    seqCfgPath = db_config.get_db_param()['fs_config_path']
    if -1 == seqCfgPath:
        file_info.get_cur_runtime_info('seqCfgPath is %d' % seqCfgPath)
        return -1
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    seqMgntPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'
    listText = open(seqMgntPath, 'r').readlines()
    if listText == []:
        file_info.get_cur_runtime_info(listText)
        return -1
    
    seqDelText = '     <user id=\"%s\" type=\"pointer\"/>\n' % (ulAgentID)
    while True:
        if seqDelText in listText:
            ulIndex = listText.index(seqDelText)
            del listText[ulIndex]
        else:
            break
    
    seqGroupText = '   <group name=\"%s-%s\">\n' % (ulCustomerID, ulNewGroupID)
    if seqGroupText in listText:
        ulIndex = listText.index(seqGroupText)
        listText.insert(ulIndex + 2, seqDelText)
    else:
        ulIndex = listText.index('   </group>\n')
        seqAddText = '   <group name=\"%s-%s\">\n    <users>\n     <user id="%s" type="pointer"/>\n    </users>\n   </group>\n' % (ulCustomerID, ulNewGroupID, ulAgentID)
        listText.insert(ulIndex + 1, seqAddText)
    open(seqMgntPath, 'w').writelines(listText)
    return 1
    
def modify_param_value(ulSIPID, seqParamName, seqParamValue):
    '''
    @param sip_id: sip账户id
    @param param_name: 参数名字
    @param param_value: 参数值
    @todo: 修改参数的值
    '''
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1
    if seqParamName.strip() == '':
        file_info.get_cur_runtime_info('seqParamName is %s' % seqParamName)
        return -1
    if seqParamValue.strip() == '':
        file_info.get_cur_runtime_info('seqParamValue is %s' % seqParamValue)
        return -1
    
    # 获取座席id
    result = get_agent_by_sip(ulSIPID)
    if -1 == result:
        file_info.get_cur_runtime_info('result is %d' % result)
        return -1
    ulAgentID = result[0][0]
    
    # 获取座席组id
    ulIDValue = get_grp_id(ulSIPID)
    if -1 == ulIDValue:
        file_info.get_cur_runtime_info('ulIDValue is %d' % ulIDValue)
        return -1
    ulGroupID = '0'
    if ulIDValue[0][0].strip() == '0':
        ulGroupID = ulIDValue[1][0]
    else:
        ulGroupID = ulIDValue[0][0]
        
    _res = get_customer_id(ulSIPID, ulGroupID)
    if -1 == _res:
        file_info.get_cur_runtime_info('_res is %d' % _res)
        return -1
    ulCustomerID = _res[0][0]
    seqCfgPath = db_config.get_db_param()['cfg_path']
    if -1 == seqCfgPath:
        file_info.get_cur_runtime_info('seqCfgPath is %d' % seqCfgPath)
        return -1
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    seqSIPPath = seqCfgPath + 'directory/' + ulCustomerID + '/' + ulAgentID + '.xml'
    listText = open(seqSIPPath, 'r').readlines()
    if listText == []:
        file_info.get_cur_runtime_info(listText)
        return -1
    print listText
    for loop in range(len(listText)):
        if ('   <param name=\"%s\"' % seqParamName) in listText[loop]:
            listText[loop] = '   <param name=\"%s\" value=\"%s\"/>\n' % (seqParamName, seqParamValue)
    open(seqSIPPath, 'w').writelines(listText)
    return 1
    
    
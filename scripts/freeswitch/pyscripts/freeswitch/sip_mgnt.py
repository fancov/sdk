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
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_is is', ulSIPID
        return None
    
    #获取sip账户的group id
    ulGroupID = get_grp_id(ulSIPID)
    
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
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is:', ulSIPID
        return
    
    global CONN
    db_conn.connect_db()
    
    #查找所有sip账户id为sip_id的座席组id
    seqSQLCmd = 'SELECT CONVERT(tbl_agent.group1_id,CHAR(10)) AS group1_id,CONVERT(tbl_agent.group2_id,CHAR(10)) AS group2_id FROM tbl_agent WHERE tbl_agent.sip_id = %d' % (ulSIPID)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',seqSQLCmd
    
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'The database connection does not exist.'
        return 
    cursor.execute(seqSQLCmd)
    ulGroupID = cursor.fetchall()
    if len(ulGroupID) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'len(grp_id) is', len(ulGroupID)
        return
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'result:',ulGroupID
    
    #返回座席组id列表，因为一个sip账户很可能属于多个账户
    return ulGroupID

def get_customer_id(ulSIPID, ulGroupID):
    '''
    @param: sip_id sip账户id; grp_id sip账户所在的座席组id
    @todo: 根据sip的id获取它所属客户的id
    '''
    
    global CONN
    if str(ulSIPID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is:', ulSIPID
        return None
    if str(ulGroupID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is:', ulGroupID
        return None
    
    db_conn.connect_db()
    # 获取座席组id为grp_id所属的客户id
    seqSQLCmd = 'SELECT CONVERT(tbl_group.customer_id,CHAR(10)) AS customer_id FROM tbl_group WHERE tbl_group.id = %s' % (ulGroupID)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',seqSQLCmd
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'The database connection does not exist.'
        return
    cursor.execute(seqSQLCmd)
    ulCustomerID = cursor.fetchall()
    db_conn.CONN.close()
    if len(ulCustomerID) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is:', ulCustomerID
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is:', ulCustomerID
    return ulCustomerID

def get_agent_by_sip(ulSIPID):
    '''
    @param: sip_id sip账户id
    @todo: 获取sip账户对应的座席组id
    '''
    global CONN
    if str(ulSIPID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is',ulSIPID
        return None
    db_conn.connect_db()
    
    # 获取sip id是sip_id的agent id
    seqSQLCmd = 'SELECT CONVERT(id, CHAR(10)) AS id FROM tbl_agent WHERE tbl_agent.sip_id = %d' % (ulSIPID)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:', seqSQLCmd
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'The database connection does not exist.'
        return
    cursor.execute(seqSQLCmd)
    result = cursor.fetchall()
    if len(result) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'len(result) is', len(result)
        return
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'result:',result
    return result

def add_sip_to_group(ulSIPID, ulGroupID):
    '''
    @param: sip_id sip账户id; grp_id sip账户所属座席组id
    @todo: 将sip账户添加到group中
    '''
    if str(ulSIPID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is',ulSIPID
        return None
    if str(ulSIPID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is',ulSIPID
    result = get_customer_id(ulSIPID, ulSIPID)
    if result is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'result is:', result
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'result is:', result
    
    #获取客户id
    ulCustomerID = result[0][0]
    #获取座席组id
    ulAgentID = get_agent_by_sip(ulSIPID)[0][0]
    
    seqCfgPath = db_config.get_db_param()['cfg_path']
    #构造以'/'结尾的路径字符串
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'cfg_path is:',seqCfgPath
    
    # 客户配置文件所在路径设置:假如customer id是1，那么客户配置文件是 %cfg_path%/directory/1.xml
    seqCustomerPath = seqCfgPath + 'directory/' + ulCustomerID + '.xml'
    if os.path.exists(seqCustomerPath) is False:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_path is:',seqCustomerPath
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_path is:',seqCustomerPath
    
    # sip账户配置文件路径设置：假设customer id是1，座席组id是100，那么sip账户配置文件是 %cfg_path%/directory/1/100.xml
    seqSIPPath = seqCfgPath + 'directory/' + ulCustomerID + '/' + ulAgentID + '.xml'
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_path is:', seqSIPPath
    
    # 读取配置文件，按行读取并组成Python列表
    seqMgntText = open(seqCustomerPath, 'r').readlines()
    if seqMgntText == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'mgnt_text is empty.'
        return
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'mgnt_text is:', seqMgntText
    
    # 构造新的sip配置字符串并添加
    seqAddText = '     <user id=\"%s\" type=\"pointer\"/>\n' % (ulAgentID)
    if seqAddText in seqMgntText:
        return
    
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'add_text is:', seqAddText
    try:
        ulMgntIndex = seqMgntText.index('   <group name=\"%s-%s\">\n' % (ulCustomerID, ulSIPID))
        if ulMgntIndex < 0:
            print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'mgnt_text does not exist.'
            return
        seqMgntText.insert(ulMgntIndex + 2, seqAddText)
        open(seqCustomerPath, 'w').writelines(seqMgntText)
        # 生成sip账户
        sip_maker.make_sip(ulSIPID, ulCustomerID, seqSIPPath)
    except Exception as e:
        print str(e)

def del_sip_from_group(ulAgentID, ulCustomerID):
    '''
    @param: agent_id 座席组id; customer_id客户id
    @todo: 将一个sip账户从一个座席组中删除
    '''
    if str(ulAgentID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'agent_id is', ulAgentID
        return None
    if str(ulCustomerID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is',ulCustomerID
        return None
    seqCfgPath = db_config.get_db_param()['cfg_path']
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    seqSIPPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '/' + str(ulAgentID) + '.xml'
    if os.path.exists(seqSIPPath):
        os.remove(seqSIPPath)
        
    seqMgntPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'
    listMgntText = open(seqMgntPath,'r').readlines()
    if listMgntText == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'mgnt_text is', listMgntText
        return
    ulIndex = listMgntText.index('     <user id=\"%d\" type=\"pointer\"/>\n' % ulAgentID)
    if ulIndex < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'The index does not exist'
        return
    del listMgntText[ulIndex]
    open(seqMgntPath, 'w').writelines(listMgntText)
   
def change_agent_group(ulSIPID, ulNewGroupID):
    '''
    @param sip_id: sip账户id
    @param new_grp_id: 新的座席组id
    @todo:将sip账户更改到另外一座席组
    '''
    if str(ulSIPID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is ',ulSIPID
        return None
    if str(ulNewGroupID).strip() == '':
        return file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'new_grp_id is', ulNewGroupID
        return None
    
    # 获取到座席组id
    result = get_agent_by_sip(ulSIPID)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'result is:', result
    
    #获取到customer id
    ulCustomerID = get_customer_id(ulSIPID, ulNewGroupID)[0][0]
    ulAgentID = result[0][0]
    seqCfgPath = db_config.get_db_param()['cfg_path']
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    seqMgntPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'
    listText = open(seqMgntPath, 'r').readlines()
    if listText == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'text_list is', listText
        return
    
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
    
def modify_param_value(ulSIPID, seqParamName, seqParamValue):
    '''
    @param sip_id: sip账户id
    @param param_name: 参数名字
    @param param_value: 参数值
    @todo: 修改参数的值
    '''
    if str(ulSIPID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is', ulSIPID
        return None
    if seqParamName.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'param_name is', seqParamName
        return None
    if seqParamValue.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'param_value is', seqParamValue
        return None
    
    # 获取座席id
    result = get_agent_by_sip(ulSIPID)
    ulAgentID = result[0][0]
    
    # 获取座席组id
    ulIDValue = get_grp_id(ulSIPID)
    ulGroupID = '0'
    if ulIDValue[0][0].strip() == '0':
        ulGroupID = ulIDValue[1][0]
    else:
        ulGroupID = ulIDValue[0][0]
    ulCustomerID = get_customer_id(ulSIPID, ulGroupID)[0][0]
    seqCfgPath = db_config.get_db_param()['cfg_path']
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    seqSIPPath = seqCfgPath + 'directory/' + ulCustomerID + '/' + ulAgentID + '.xml'
    listText = open(seqSIPPath, 'r').readlines()
    if listText == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'text_list is', listText
        return
    print listText
    for loop in range(len(listText)):
        if ('   <param name=\"%s\"' % seqParamName) in listText[loop]:
            listText[loop] = '   <param name=\"%s\" value=\"%s\"/>\n' % (seqParamName, seqParamValue)
    open(seqSIPPath, 'w').writelines(listText)
    
    
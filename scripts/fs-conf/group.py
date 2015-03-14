# coding=utf-8


'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: generate agent group
'''
import sip_account
import db_conn
import file_info
import types

def generate_group(doc, ulGroupID, ulCustomerID):
    '''
    @param doc: 文件对象
    @param grp_id: 座席组id
    @param customer_id: 客户id
    @todo: 生成座席组  
    '''
    if doc is None:
        file_info.get_cur_runtime_info('doc is %p' % doc)
        return -1
    if ulGroupID == '' or ulGroupID is None:
        file_info.get_cur_runtime_info('ulGroupID is %s' % str(ulGroupID))
        return -1
    if ulCustomerID == '' or ulCustomerID is None:
        file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
        return -1
    
    # 查找出所有group1_id或者group2_id为`grp_id`的座席id
    seqSQLCmd = 'SELECT CONVERT(id, CHAR(10)) AS id FROM tbl_agent WHERE tbl_agent.group1_id = %s OR tbl_agent.group2_id = %s' % (ulGroupID, ulGroupID)
    file_info.get_cur_runtime_info('seqSQLCmd is %s' % seqSQLCmd)
    
    lRet = db_conn.connect_db()
    if lRet == -1:
        file_info.get_cur_runtime_info('The database connected failed.')
        return -1
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        file_info.get_cur_runtime_info('The database connection does not exist.')
        return -1
    ulMemCnt = cursor.execute(seqSQLCmd)
    listGroupRslt = cursor.fetchall() # get all results
    if len(listGroupRslt) == 0:
        file_info.get_cur_runtime_info('len(listGroupRslt) is %d' % len(listGroupRslt))
        return -1
    db_conn.CONN.close()
    file_info.get_cur_runtime_info(listGroupRslt)
    
    domGroupNode = doc.createElement('group')
    domGroupNode.setAttribute('name', str(ulCustomerID) + '-' + str(ulGroupID))
    domUsersNode = doc.createElement('users')
    for loop in range(0, ulMemCnt):
        domUserNode = doc.createElement('user')
        domUserNode.setAttribute('id', str(listGroupRslt[loop][0]) if type(listGroupRslt[loop][0]) == types.IntType else listGroupRslt[loop][0])
        domUserNode.setAttribute('type', 'pointer')
        domUsersNode.appendChild(domUserNode)
        lRet = sip_account.generate_sip_account( listGroupRslt[loop][0], ulCustomerID)
        if -1 == lRet:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
    
    domGroupNode.appendChild(domUsersNode)
    return domGroupNode

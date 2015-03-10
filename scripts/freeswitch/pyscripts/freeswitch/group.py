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
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'doc is', doc
        return
    if ulGroupID == '' or ulGroupID is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is',ulGroupID
        return
    if ulCustomerID == '' or ulCustomerID is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is',ulCustomerID
        return
    
    # 查找出所有group1_id或者group2_id为`grp_id`的座席id
    seqSQLCmd = 'SELECT CONVERT(id, CHAR(10)) AS id FROM tbl_agent WHERE tbl_agent.group1_id = %s OR tbl_agent.group2_id = %s' % (ulGroupID, ulGroupID)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:', seqSQLCmd
    
    db_conn.connect_db()
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'The database connection does not exist.'
        return
    ulMemCnt = cursor.execute(seqSQLCmd)
    listGroupRslt = cursor.fetchall() # get all results
    if len(listGroupRslt) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'len(grp_rslt) is', len(listGroupRslt)
        return
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_rslt is', listGroupRslt
    
    domGroupNode = doc.createElement('group')
    domGroupNode.setAttribute('name', str(ulCustomerID) + '-' + str(ulGroupID))
    domUsersNode = doc.createElement('users')
    for loop in range(0, ulMemCnt):
        domUserNode = doc.createElement('user')
        domUserNode.setAttribute('id', str(listGroupRslt[loop][0]) if type(listGroupRslt[loop][0]) == types.IntType else listGroupRslt[loop][0])
        domUserNode.setAttribute('type', 'pointer')
        domUsersNode.appendChild(domUserNode)
        sip_account.generate_sip_account( listGroupRslt[loop][0], ulCustomerID)
    
    domGroupNode.appendChild(domUsersNode)
    return domGroupNode

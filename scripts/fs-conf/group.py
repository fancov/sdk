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
import sip_maker

def generate_group(doc, ulGroupID, ulCustomerID):
    '''
    @param doc: 文件对象
    @param ulGroupID: 座席组id
    @param ulCustomerID: 客户id
    @todo: 生成组
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
    
    # 找出座席组id为ulGroupID的座席
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
        # 根据坐席id获取sip账户id
        lRet = sip_account.get_sipid_by_agent(int(listGroupRslt[loop][0]))
        if -1 == lRet:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
        # 根据sip账户id获取sipuserid
        seqUserID = get_userid_by_sipid(lRet)
           
        domUserNode = doc.createElement('user')
        domUserNode.setAttribute('id', seqUserID)
        domUserNode.setAttribute('type', 'pointer')
        domUsersNode.appendChild(domUserNode)
        #sip_maker.make_sip(ulSIPID, ulCustomerID, seqPath)
        lRet = sip_account.generate_sip_account(seqUserID, ulCustomerID)
        if -1 == lRet:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
    
    domGroupNode.appendChild(domUsersNode)
    return domGroupNode


def get_userid_by_sipid(ulSipID):
    '''
    @todo: 根据sip id获得userid
    '''
    if str(ulSipID).strip() == '':
        file_info.get_cur_runtime_info('ulSipID is %s' % str(ulSipID))
        return -1
    
    seqSQLCmd = 'SELECT userid FROM tbl_sip WHERE id = %d' % ulSipID
    file_info.get_cur_runtime_info('seqSQLCmd is:%s' % seqSQLCmd)
    
    try:
        lRet = db_conn.connect_db()
    except Exception, err:
        file_info.get_cur_runtime_info('Catch Exception: %s' % str(err))
        return -1
    else:
        if -1 == lRet:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
        
        cursor = db_conn.CONN.cursor()
        cursor.execute(seqSQLCmd)
        
        listResult = cursor.fetchall()
        
        if len(listResult) == 0:
            file_info.get_cur_runtime_info('len(listResult) is %d' % len(listResult))
            return -1
        
        seqUserID = str(listResult[0][0])
        return seqUserID
    
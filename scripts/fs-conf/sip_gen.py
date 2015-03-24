#coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: handle the customer
'''

import os
from  xml.dom import minidom
from xml.dom.minidom import Document
import file_info
import db_exec
import db_config
import conf_path
import dom_to_xml

def sip_get_allcustom():
	strSQL = 'SELECT customer_id FROM tbl_sip GROUP BY customer_id;'
	listQueryResult = db_exec.exec_SQL(strSQL)
	if -1 == listQueryResult:
		return -1;

	return listQueryResult

def generate_customer_head(doc):
	domParamNode = doc.createElement('param')
	domParamNode.setAttribute('name', 'dial-string')
	domParamNode.setAttribute('value', '{^^:sip_invite_domain=${dialed_domain}:presence_id=${dialed_user}@${dialed_domain}}${sofia_contact(*/${dialed_user}@${dialed_domain})}')

	domParamsNode = doc.createElement('params')
	domParamsNode.appendChild(domParamNode)

	_dict = {'record_stereo':'true', 'default_gateway':'$${default_provider}'
				, 'default_areacode':'$${default_areacode}', 'transfer_fallback_extension':'operator'}
	file_info.print_file_info(_dict)
	domVariablesNode = doc.createElement('variables')
	arrVariNode = []
	loop = 0
	for key in _dict:
		variable = doc.createElement('variable')
		arrVariNode.append(variable)
		arrVariNode[loop] = doc.createElement('variable')
		arrVariNode[loop].setAttribute('name', key)
		arrVariNode[loop].setAttribute('value', _dict[key])
		domVariablesNode.appendChild(arrVariNode[loop])
		loop = loop + 1

	domPreNode = doc.createElement('X-PRE-PROCESS')
	domPreNode.setAttribute('cmd', 'include')

	domPreNode.setAttribute('data',	 'default/*.xml')

	domUsersNode = doc.createElement('users')
	domUsersNode.appendChild(domPreNode)

	domGroupNode = doc.createElement('group')
	domGroupNode.setAttribute('name', 'default')
	domGroupNode.appendChild(domUsersNode)

	domGroupUser = doc.createElement('users')

	domGroupNode1 = doc.createElement('group')
	domGroupNode1.setAttribute('name', 'all')
	domGroupNode1.appendChild(domGroupUser)

	domGroupsNode = doc.createElement('groups')
	domGroupsNode.appendChild(domGroupNode)
	domGroupsNode.appendChild(domGroupNode1)

	domDomainNode = doc.createElement('domain')
	domDomainNode.setAttribute('name', '$${domain}')
	domDomainNode.appendChild(domParamsNode)
	domDomainNode.appendChild(domVariablesNode)
	domDomainNode.appendChild(domGroupsNode)

	domIncludeNode = doc.createElement('include')
	domIncludeNode.appendChild(domDomainNode)
	
	doc.appendChild(domIncludeNode)

	return (doc, domGroupUser)

def generate_sip(SIPAccountInfo):
	'''
	@todo: 根据userid生成sip账户配置
	'''

	if SIPAccountInfo == []:
		return -1

	ulID = int(SIPAccountInfo[0])
	#ulCustomerID = int(SIPAccountInfo[1])
	#seqExtension = SIPAccountInfo[2]
	seqDispName = SIPAccountInfo[3]
	seqUserID = SIPAccountInfo[4]
	seqAuthName = SIPAccountInfo[5]
	seqAuthPassword = SIPAccountInfo[6]

	# 构造XML树DOM对象
	doc = Document()

	domParamsNode = doc.createElement('params')
	domVariablesNode = doc.createElement('variables')

	dictParam = {'password':'$${default_password}' if SIPAccountInfo == [] else seqAuthPassword, 'vm-password':ulID}
	dictVariable = {'toll_allow':'domestic,international,local',
				'accountcode':seqAuthName,
				'user_context':'default',
				'effective_caller_id_name':'' if SIPAccountInfo == [] else seqDispName,
				'effective_caller_id_number':seqUserID,
				'outbound_caller_id_name':'$${outbound_caller_name}',
				'outbound_caller_id_number':'$${outbound_caller_id}',
				'callgroup':'all'
		}

	listParamsNode = []
	loop = 0
	for key in dictParam:
		domParamNode = doc.createElement('param')
		domParamNode.setAttribute('name', str(key))
		domParamNode.setAttribute('value', str(dictParam[key]))
		listParamsNode.append(domParamNode)
		domParamsNode.appendChild(listParamsNode[loop])
		loop = loop + 1

	listVariNode = []
	loop = 0
	for key in dictVariable:
		domVariableNode = doc.createElement('variable')
		domVariableNode.setAttribute('name', str(key))
		domVariableNode.setAttribute('value', str(dictVariable[key]))
		listVariNode.append(domVariableNode)
		domVariablesNode.appendChild(listVariNode[loop])
		loop = loop + 1

	domUserNode = doc.createElement('user')
	domUserNode.setAttribute('id', seqUserID)
	domUserNode.appendChild(domParamsNode)
	domUserNode.appendChild(domVariablesNode)

	domIncludeNode = doc.createElement('include')
	domIncludeNode.appendChild(domUserNode)
	doc.appendChild(domIncludeNode)

	# 获取配置文件路径
	seqFsPath = conf_path.get_config_path()
	if -1 == seqFsPath:
		file_info.print_file_info('seqFsPath is %d' % seqFsPath)
		return -1

	# 构造以'/'结尾的路径字符串
	if seqFsPath[-1] != '/':
		seqFsPath = seqFsPath + '/'

	# 构造customer目录
	seqFsPath = seqFsPath + 'directory/'
	if os.path.exists(seqFsPath) is False:
		os.makedirs(seqFsPath)

	# 构造sip目录
	seqSipDir = seqFsPath + 'default/'
	if os.path.exists(seqSipDir) is False:
		os.makedirs(seqSipDir)

	# 构造sip配置文件路径
	seqSipXml = seqSipDir + str(ulID) + '.xml'

	# 将DOM对象转化为XML
	lRet = dom_to_xml.dom_to_xml(seqSipXml, doc)
	if -1 == lRet:
		file_info.print_file_info('lRet is %d' % lRet)
		return -1

	# 去掉XML文件的头部声明
	lRet = dom_to_xml.del_xml_head(seqSipXml)
	if -1 == lRet:
		file_info.print_file_info('lRet is %d' % lRet)
		return -1

	return 1

def sip_gen_all():
	# 获取数据库配置信息
	_dict = db_config.get_db_param()
	if -1 == _dict:
		file_info.print_file_info('_dict is %d' % _dict)
		return -1
	# 获取FreeSwitch配置文件路径
	seqFsPath = _dict['fs_config_path']
	# 构造以'/'结尾的字符串
	if seqFsPath[-1] != '/':
		seqFsPath = seqFsPath + '/'
	if os.path.exists(seqFsPath) is False:
		os.makedirs(seqFsPath)

	# 构造客户目录
	seqCusDir = seqFsPath + 'directory/default/'
	if os.path.exists(seqCusDir) is False:
		os.makedirs(seqCusDir)

	# 构造管理配置文件
	seqCusFile = seqFsPath + 'directory/default.xml'

	# 1.生成SIP账户目录
	# 2.创建SIP账户管理的配置文件（内存中）
	# 3.查找所有SIP账户
	# 4.循环生成SIP账户配置文件，同时将账户添加到管理SIP账户的配置文件
	# 5.写SIP账户管理配置文件
	strSQL = 'SELECT * FROM tbl_sip WHERE customer_id;'
	SIPUserIDList = db_exec.exec_SQL(strSQL)
	if -1 == SIPUserIDList:
		return -1

	doc = Document()
	lRet = generate_customer_head(doc)
	if -1 == lRet:
		return -1

	(doc, domGroupUser) = lRet

	for i in range(len(SIPUserIDList)):
		# 更新管理内存树
		domDomainNode = doc.createElement('user')
		domDomainNode.setAttribute('id', SIPUserIDList[i][4])
		domDomainNode.setAttribute('type', 'pointer')

		# 创建客户配置文件
		lRet = generate_sip(SIPUserIDList[i])
		if -1 == lRet:
			return -1
		
		domGroupUser.appendChild(domDomainNode)

	# 写配置文件
	# 将DOM转换为XML
	lRet = dom_to_xml.dom_to_xml(seqCusFile, doc)
	if lRet == -1:
		file_info.print_file_info('lRet is %d' % lRet)
		return -1

	# 删除XML头部声明
	lRet = dom_to_xml.del_xml_head(seqCusFile)
	if -1 == lRet:
		file_info.print_file_info('lRet is %d' % lRet)
		return -1
	
	return 0

def sip_delete(CustomerID, SIPID, SIPUserID):
	'''
	删除SIP账户
	1. 删除SI账户文件更新SIP账户管理配置文件
	'''
	# 获取数据库配置信息
	_dict = db_config.get_db_param()
	if -1 == _dict:
		file_info.print_file_info('_dict is %d' % _dict)
		return -1
	# 获取FreeSwitch配置文件路径
	seqFsPath = _dict['fs_config_path']
	# 构造以'/'结尾的字符串
	if seqFsPath[-1] != '/':
		seqFsPath = seqFsPath + '/'
	if os.path.exists(seqFsPath) is False:
		os.makedirs(seqFsPath)

	# 构造客户目录
	seqCusDir = seqFsPath + 'directory/default/'
	if os.path.exists(seqCusDir) is False:
		os.makedirs(seqCusDir)

	# 构造管理配置文件
	seqCusFile = seqFsPath + 'directory/default.xml'
	seqSIPConfigFile = seqFsPath + 'directory/default/' + str(SIPID) + '.xml'

	# 更新管理SIP账户的配置文件配置文件
	if os.path.exists(seqCusFile) is True:
		xmlDoc = minidom.parse(seqCusFile)
		groupList = xmlDoc.getElementsByTagName('group')
		for i in range(len(groupList)):
			if groupList[i].getAttribute('name') == 'all':
				break

		if i < len(groupList):
			usersNode = groupList[i].getElementsByTagName('users')
			userList = usersNode[0].getElementsByTagName('user')
			for j in range(len(userList)):
				if userList[j].getAttribute('id') == SIPUserID:
					usersNode[0].removeChild(userList[j])
		# 写配置文件
		# 将DOM转换为XML
		lRet = dom_to_xml.dom_to_xml(seqCusFile, xmlDoc)
		if lRet == -1:
			file_info.print_file_info('lRet is %d' % xmlDoc)
			return -1

		# 删除XML头部声明
		lRet = dom_to_xml.del_xml_head(seqCusFile)
		if -1 == lRet:
			file_info.print_file_info('lRet is %d' % lRet)
			return -1

	if os.path.exists(seqSIPConfigFile) is True:
		os.remove(seqSIPConfigFile)

	return 0

def sip_add(SIPID):
	strSQL = 'SELECT * FROM tbl_sip WHERE id = %d;' % int(SIPID)
	SIPUserIDList = db_exec.exec_SQL(strSQL)
	if -1 == SIPUserIDList:
		return -1

	if SIPUserIDList == []:
		return -1
		
	lRet = generate_sip(SIPUserIDList[0])
	if -1 == lRet:
		return -1

	# 获取数据库配置信息
	_dict = db_config.get_db_param()
	if -1 == _dict:
		file_info.print_file_info('_dict is %d' % _dict)
		return -1
	# 获取FreeSwitch配置文件路径
	seqFsPath = _dict['fs_config_path']
	# 构造以'/'结尾的字符串
	if seqFsPath[-1] != '/':
		seqFsPath = seqFsPath + '/'
	if os.path.exists(seqFsPath) is False:
		os.makedirs(seqFsPath)

	# 构造客户目录
	seqCusDir = seqFsPath + 'directory/default/'
	if os.path.exists(seqCusDir) is False:
		os.makedirs(seqCusDir)

	# 构造管理配置文件
	seqCusFile = seqFsPath + 'directory/default.xml'

	# 更新管理SIP账户的配置文件配置文件
	xmlDoc = minidom.parse(seqCusFile)
	groupList = xmlDoc.getElementsByTagName('group')

	for i in range(len(groupList)):
		if groupList[i].getAttribute('name') == 'all':
			userNode = xmlDoc.createElement('user')
			userNode.setAttribute('id', SIPUserIDList[0][4])
			userNode.setAttribute('type', 'pointer')

			usersNode = groupList[i].getElementsByTagName('users')
			usersNode[0].appendChild(userNode)

			lRet = dom_to_xml.dom_to_xml(seqCusFile, xmlDoc)
			if lRet == -1:
				file_info.print_file_info('lRet is %d' % lRet)
				return -1

			# 删除XML头部声明
			lRet = dom_to_xml.del_xml_head(seqCusFile)
			if -1 == lRet:
				file_info.print_file_info('lRet is %d' % lRet)
				return -1
	return 0

def sip_update(CustomerID, SIPID, SIPUserID):
	'''
	更新SIP账户。
	1. 删除老的
	2. 创建新的
	'''
	lRet = sip_delete(CustomerID, SIPID, SIPUserID)
	if -1 == lRet:
		return -1

	lRet = sip_add(SIPID)
	if -1 == lRet:
		return -1

	return 0

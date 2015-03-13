# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: get the configuration data of freeswitch
'''

from xml.etree import ElementTree as ET
import file_info
import os

def get_db_param():
    '''
    @todo: 获取配置文件数据
    @return: 配置文件数据字典
    '''
    
    # 首先查看"/etc/global.xml"文件是否存在
    # 如果不存在，则查找"../etc/global.xml是否存在"
    # 如果存在则读取数据，否则返回空值
    seqGlobalCfgFile = '../../../../conf/global.xml'
    
    #seqGlobalCfgFile = '/etc/global.xml'
    if os.path.exists(seqGlobalCfgFile) is False:
        seqGlobalCfgFile = '../etc/global.xml'
        if os.path.exists(seqGlobalCfgFile) is False:
            return -1
        
    parser = ET.parse(seqGlobalCfgFile)
    domFsParam = parser.findall('./mysql/param')
   
    _dict = {}
    for loop in range(len(domFsParam)):
        _dict[domFsParam[loop].get('name')] = domFsParam[loop].get('value')

    domFsParam = parser.findall('./service/path/freeswitch/param')
    for loop in range(len(domFsParam)):
        _dict[domFsParam[loop].get('name')] = domFsParam[loop].get('value')
    
    file_info.get_cur_runtime_info(_dict)
    
    # dict[0]: 数据库用户名
    # dict[1]: Python脚本路径
    # dict[2]: freeswitch配置文件根路径
    # dict[3]: 业务数据库库名
    # dict[4]: 系统资源数据库库名
    # dict[5]: 数据库主机IP地址
    # dict[6]: 数据库密码
    # dict[7]: 数据库端口号
    return _dict

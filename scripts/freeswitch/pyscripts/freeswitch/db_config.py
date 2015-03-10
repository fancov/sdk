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
    #seqGlobalCfgFile = '../../../../conf/global.xml'
    
    seqGlobalCfgFile = '/etc/global.xml'
    if os.path.exists(seqGlobalCfgFile) is False:
        seqGlobalCfgFile = '../etc/global.xml'
        if os.path.exists(seqGlobalCfgFile) is False:
            return None
        
    parser = ET.parse(seqGlobalCfgFile)
    domFsParam = parser.findall('./fs_db/param')
   
    dict = {}
    for loop in range(6):
        dict[domFsParam[loop].get('name')] = domFsParam[loop].get('value')
    print file_info.get_file_name(), file_info.get_line_number(),file_info.get_function_name(),'dict is', dict
    
    # dict[0]: 用户名
    # dict[1]: 主机IPv4地址
    # dict[2]: 数据库名
    # dict[3]: 配置文件根路径
    # dict[4]: 登录数据库密码
    # dict[5]: 端口号
    return dict


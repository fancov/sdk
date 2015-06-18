# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: get the config param
'''

from xml.etree import ElementTree as ET
import os
import file_info


def get_db_param():
    '''
    @todo: 获取数据库配置参数
    '''
    
    seqGlobalCfgFile = '/etc/global.xml'
    # seqGlobalCfgFile = 'F:/workspace/audio_ad_sys/sdk/conf/global.xml'
    
    # 如果说'/etc/global.xml'未找到，则在当前父目录下的etc子目录下寻找global.xml
    if os.path.exists(seqGlobalCfgFile) is False:
        seqGlobalCfgFile = '../etc/global.xml'
        if os.path.exists(seqGlobalCfgFile) is False:
            file_info.print_file_info('global.xml does not exist.')
            return -1
        
    try:
        # 解析配置文件，可能会抛出异常
        parser = ET.parse(seqGlobalCfgFile)
    except Exception, err:
        file_info.print_file_info('Catch Exception:%s' % str(err))
        return -1
    else:
        # 解析XML
        domFsParam = parser.findall('./mysql/param')
        _dict = {}
        
        for loop in range(len(domFsParam)):
            _dict[domFsParam[loop].get('name')] = domFsParam[loop].get('value')
                
        domFsParam = parser.findall('./service/path/freeswitch/param')
            
        for loop in range(len(domFsParam)):
            _dict[domFsParam[loop].get('name')] = domFsParam[loop].get('value')

        # 返回字典
        return _dict

# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: get the path from config file
'''

import db_config
import file_info

def get_config_path():
    '''
    @todo: 获取配置文件存放的路径
    '''
    
    # 获取配置文件信息
    _dict = db_config.get_db_param()
    
    if -1 == _dict:
        file_info.print_file_info('_dict is %d.' % _dict)
        return -1
    
    if _dict == {}:
        file_info.print_file_info('_dict is empty.')
        return -1
    
    # 获取FreeSwitch配置文件路径
    seqFsPath = _dict['fs_config_path']
    return seqFsPath
# -*- encode=UTF-8 -*-
'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: get the configuration data of freeswitch
'''

from xml.etree import ElementTree as ET
import file_info

def get_db_param():
    '''
    @todo: get configuration data
    @return: the configuration
    '''
    parser = ET.parse('../../../../conf/global.xml')
    fs_param = parser.findall('./fs_db/param')
   
    dict = {}
    for loop in range(6):
        dict[fs_param[loop].get('name')] = fs_param[loop].get('value')
    print file_info.get_file_name(), file_info.get_line_number(),file_info.get_function_name(),'dict is', dict
    return dict
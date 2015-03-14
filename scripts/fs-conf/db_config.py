# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: get the configuration data of freeswitch
'''

from xml.etree import ElementTree as ET
import os
import file_info

def get_db_param():
    '''
    @todo: ��ȡ�����ļ�����
    @return: �����ļ������ֵ�
    '''
    
    # ���Ȳ鿴"/etc/global.xml"�ļ��Ƿ����
    # ��������ڣ������"../etc/global.xml�Ƿ����"
    # ����������ȡ���ݣ����򷵻ؿ�ֵ
    seqGlobalCfgFile = '../../conf/global.xml'
    #seqGlobalCfgFile = '/etc/global.xml'
    if os.path.exists(seqGlobalCfgFile) is False:
        seqGlobalCfgFile = '../etc/global.xml'
        if os.path.exists(seqGlobalCfgFile) is False:
            return -1
    print '----------------------------'
    parser = ET.parse(seqGlobalCfgFile)
    print '~~~~~~~~~~~~~~~~~~~~~~~~~~~~'
    domFsParam = parser.findall('./mysql/param')
   
    _dict = {}
    for loop in range(len(domFsParam)):
        _dict[domFsParam[loop].get('name')] = domFsParam[loop].get('value')

    domFsParam = parser.findall('./service/path/freeswitch/param')
    for loop in range(len(domFsParam)):
        _dict[domFsParam[loop].get('name')] = domFsParam[loop].get('value')
    
    file_info.get_cur_runtime_info(_dict)
    
    # dict[0]: ���ݿ��û���
    # dict[1]: Python�ű�·��
    # dict[2]: freeswitch�����ļ���·��
    # dict[3]: ҵ�����ݿ����
    # dict[4]: ϵͳ��Դ���ݿ����
    # dict[5]: ���ݿ�����IP��ַ
    # dict[6]: ���ݿ�����
    # dict[7]: ���ݿ�˿ں�
    return _dict

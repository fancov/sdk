# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: get the data of python interpreter
'''
import os
import file_info

def get_data_from_txt(seqFileName):
    '''
    @param filename: �ļ���
    @todo: ��txt�ļ��л�ȡsip�˻�id�б�
    '''
    if seqFileName.strip() == '':
        file_info.get_cur_runtime_info('seqFileName is %s' % seqFileName)
        return -1
    if is_txt_file(seqFileName) is False:
        file_info.get_cur_runtime_info('%s is not a txt file' % seqFileName)
        return -1
    
    if os.path.exists(seqFileName) is False:
        file_info.get_cur_runtime_info('File %s does not exist.' % seqFileName)
        return -1
    listText = open(seqFileName, 'r').readlines()
    if [] == listText:
        file_info.get_cur_runtime_info('File %s is empty.' % seqFileName)
        return -1
    
    # ȥ����β�Ļ��з�
    for loop in range(len(listText)):
        if listText[loop].endswith('\n'):
            listText[loop] = listText[loop][:-1]
    
    file_info.get_cur_runtime_info(listText) 
    return listText

def is_txt_file(seqFileName):
    '''
    @param filename: �ļ���
    @todo: �ж��ļ���׺���Ƿ�Ϊ.txt��׺
    '''
    if seqFileName.endswith('.txt'):
        file_info.get_cur_runtime_info(seqFileName)
        return True
    return False

# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: get the data of python interpreter
'''

import file_info

def get_data_from_txt(seqFileName):
    '''
    @param filename: 文件名
    @todo: 从txt文件中获取sip账户id列表
    '''
    if seqFileName.strip() == '':
        file_info.get_cur_runtime_info('seqFileName is %s' % seqFileName)
        return -1
    if is_txt_file(seqFileName) is False:
        return -1
    listText = open(seqFileName, 'r').readlines()
    
    # 去掉结尾的换行符
    for loop in range(len(listText)):
        if listText[loop].endswith('\n'):
            listText[loop] = listText[loop][:-1]
    
    file_info.get_cur_runtime_info(listText) 
    return listText

def is_txt_file(seqFileName):
    '''
    @param filename: 文件名
    @todo: 判断文件后缀名是否为.txt后缀
    '''
    if seqFileName.endswith('.txt'):
        file_info.get_cur_runtime_info(seqFileName)
        return True
    return False

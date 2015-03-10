# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: get the data of python interpreter
'''

import file_info
import router

def get_data_from_txt(seqFileName):
    '''
    @param filename: 文件名
    @todo: 从txt文件中获取sip账户id列表
    '''
    if seqFileName.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'file name is', seqFileName 
        return None
    if is_txt_file(seqFileName) is False:
        return None
    listText = open(seqFileName, 'r').readlines()
    
    # 去掉结尾的换行符
    for loop in range(len(listText)):
        if listText[loop].endswith('\n'):
            listText[loop] = listText[loop][:-1]
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sips are:\n', listText
    return listText

def is_txt_file(seqFileName):
    '''
    @param filename: 文件名
    @todo: 判断文件后缀名是否为.txt后缀
    '''
    if seqFileName.endswith('.txt'):
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'file name is', seqFileName
        return True
    return False
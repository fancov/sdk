# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@togo: get information about file such as file line number,file name,function name
'''

import sys

def get_file_name():
    '''
    @todo: 获取当前的文件名
    '''
    f = sys._getframe(1)
    seqFileName = f.f_code.co_filename
    if is_windows():
        seqFileName = seqFileName.split('\\')[-1]
    elif is_linux():
        seqFileName = seqFileName.split('/')[-1]
    return seqFileName

def get_line_number():
    '''
    @todo: 获取当前所在的行号
    '''
    f = sys._getframe(1)
    ulLineNumber = f.f_lineno
    return ulLineNumber

def get_function_name():
    '''
    @todo: 获取当前的函数名
    '''
    seqFileName = sys._getframe(1).f_code.co_name
    return seqFileName

def is_windows():
    '''
    @todo: 判断当前运行平台是否为Windows平台
    '''
    if 'win' in sys.platform:
        return True
    else:
        return False
    
def is_linux():
    '''
    @todo: 判断当前运行平台是否为基于Linux平台
    '''
    if 'linux' in sys.platform:
        return True
    else:
        return False
    
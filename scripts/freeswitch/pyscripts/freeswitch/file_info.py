# -*- encoding=utf-8 -*-
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
    file_name = f.f_code.co_filename
    if is_windows():
        file_name = file_name.split('\\')[-1]
    elif is_linux():
        file_name = file_name.split('/')[-1]
    return file_name

def get_line_number():
    '''
    @todo: 获取当前所在的行号
    '''
    f = sys._getframe(1)
    line_number = f.f_lineno
    return line_number

def get_function_name():
    '''
    @todo: 获取当前的函数名
    '''
    f_name = sys._getframe(1).f_code.co_name
    return f_name

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
    
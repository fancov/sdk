# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@togo: get information about file such as file line number,file name,function name
'''

import sys

def is_windows():
    '''
    @TODO: 判断当前运行平台是否为Windows平台
    '''
    if 'win' in sys.platform:
        return True
    else:
        return False
    
def is_linux():
    '''
    @TODO: 判断当前运行平台是否为基于Linux平台
    '''
    if 'linux' in sys.platform:
        return True
    else:
        return False
    
def get_cur_runtime_info(sequence):
    '''
    @TODO: 打印当前文件的信息
    '''
    
    IS_OUTPUT = 1
    
    if IS_OUTPUT == 0:
        return -1
    
    f = sys._getframe(1)
    
    # 获取当前文件名
    seqFileName = f.f_code.co_filename
    if is_windows():
        seqFileName = seqFileName.split('\\')[-1]
    elif is_linux():
        seqFileName = seqFileName.split('/')[-1]
    
    # 获取当前行号
    ulLineNumber = f.f_lineno
    
    # 获取当前所在函数
    seqFunName   = f.f_code.co_name
    
    print '%s:Line %d:In function %s:' % (seqFileName, ulLineNumber, seqFunName), sequence
    return 1
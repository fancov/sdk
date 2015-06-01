#coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@togo: get information about file such as file line number,file name,function name
'''

import sys
import os
import log
import time

def is_windows():
    '''
    @todo: 判断是否为windows系统
    '''
    if sys.platform.find('win') < 0:
        return False
    else:
        return True
    
def is_linux():
    '''
    @todo: 判断是否为linux内核的系统
    '''
    if sys.platform.find('linux') < 0:
        return False
    else:
        return True
    
def print_file_info(info):
    '''
    @todo: 打印文件相关信息
    '''
    
    # 定义是否输出，为1则表示输出，否则不输出
    IS_OUTPUT = 0
    
    if IS_OUTPUT == 0:
        return -1
    
    f = sys._getframe(1)
    
    # 获取当前文件名
    seqFileName = f.f_code.co_filename
    if is_windows():
        seqFileName = seqFileName.split('\\')[-1]
    elif is_linux():
        seqFileName = seqFileName.split('/')[-1]
        
    #获取当前文件行号
    ulLineNumber = f.f_lineno
    
    # 获取当前函数名
    seqFuncName = f.f_code.co_name

    _info = '%s:Line %d:In function %s:' % (seqFileName, ulLineNumber, seqFuncName)
    output = time.strftime('%Y-%m-%d %H:%M:%S %A')
    print output, '=>', _info, info
    
    seqLogPath = '/var/log/dipcc/fsconf' + time.strftime('%Y%m%d') + '.log'
    if os.path.exists(seqLogPath) is False:
        log.create_fs_log_dir()
    
    fp = open(seqLogPath, 'a')

    try:
        fp = open(seqLogPath, 'a')
    except Exception, err:
        print 'Catch IOException: %s' % str(err)
        # 不能返回-1，防止将程序退出了
        return -1
    else:
        print >> fp, output, '=>', _info, info
        fp.close()
    
        return 1
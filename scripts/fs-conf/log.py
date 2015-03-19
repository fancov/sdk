#coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: handle the customer
'''

import os

def create_fs_log_dir():
    '''
    @todo: 创建freeswitch配置文件生成模块的log文件
    '''
    
    seqFsLogDir = '/var/log/dipcc/'
    
    if os.path.exists(seqFsLogDir) is False:
        os.makedirs(seqFsLogDir)
        
    return 1

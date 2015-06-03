# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: handle the XML files
'''

import file_info
import os

def dom_to_xml(seqFileName, doc):
    '''
    @todo:将DOM转化为XML
    '''

    if seqFileName.strip() == '':
        file_info.print_file_info('seqFileName is %s' % seqFileName)
        return -1
    
    if doc is None:
        file_info.print_file_info('doc is %p' % doc)
        return -1
    
    file_info.print_file_info('seqFileName is %s' % seqFileName)
    
    try:
        fp = os.open(seqFileName, os.O_RDWR | os.O_CREAT)
    except IOError, err:
        file_info.print_file_info('Catch IOException:%s' % str(err))
        return -1
    else:
        seqStr = doc.toprettyxml(indent = ' ')
        lRet = os.write(fp, seqStr)
        file_info.print_file_info('lRet == %d' % lRet)
        os.fsync(fp)
        os.close(fp)
        return 1
    
def del_xml_head(seqFileName):
    '''
    @todo: 去掉XML头部的XML声明
    '''
    
    if seqFileName.strip() == '':
        file_info.print_file_info('seqFileName is %s' % str(seqFileName))
        return -1
    
    try:
        fp = open(seqFileName, 'r')
    except IOError, err:
        file_info.print_file_info('Catch IOException: %s' % str(err))
        return -1
    else:
        seqLines = fp.readlines()
        fp.close()
        try:
            #将数据重新写进去
            fp = open(seqFileName, 'w')
        except IOError, err:
            file_info.print_file_info('Catch IOException: %s' % str(err))
            return -1
        else:
            fp.writelines(seqLines[1:])
            fp.close()
            
        return 1

def del_blank_line(seqFileName):
    fp = open(seqFileName, 'r')
    textLine = fp.readlines()
    newTextLine = []
    for i in range(len(textLine)):
        if str(textLine[i]).strip() != '':
            newTextLine.append(textLine[i])

    fp.close()
    fp = open(seqFileName, 'w')
    fp.writelines(newTextLine)
    fp.close()


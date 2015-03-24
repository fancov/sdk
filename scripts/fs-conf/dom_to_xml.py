# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: handle the XML files
'''

import file_info

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
        fp = open(seqFileName, 'w')
    except IOError, err:
        file_info.print_file_info('Catch IOException:%s' % str(err))
        return -1
    else:
        fp.write(doc.toprettyxml(indent = ' '))
        fp.close()
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

# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: convert the dom to xml file
'''

import file_info

def dom_to_pretty_xml(seqFileName, doc):
    '''
    @param filename: 文件名
    @param doc: 文件对象
    @todo: 将DOM对象转换为优雅的XML格式并生成到文件
    ''' 
    
    if seqFileName.strip() == '':
        file_info.get_cur_runtime_info('filename is %s' % seqFileName)
        return -1
    
    file_info.get_cur_runtime_info('filename is %s' % seqFileName)
    if doc is None:
        file_info.get_cur_runtime_info('doc is %p' % doc)
        return -1
    if seqFileName.strip() == '':
        file_info.get_cur_runtime_info('seqFileName is %s' % seqFileName)
        return -1
    
    try:
        open(seqFileName, 'w').write(doc.toprettyxml(indent = ' '))
        return 1
    except Exception as e:
        print str(e)

def del_xml_head(szFileName):
    '''
    @param filename: XML文件名
    @todo: 去掉XML文件的声明
    '''   
    
    if szFileName == '':
        file_info.get_cur_runtime_info('szFileName is %s' % szFileName)
        return -1
    seqLines = open(szFileName, 'r').readlines()
    open(szFileName, 'w').writelines(seqLines[1:])
    return 1
    

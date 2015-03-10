# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: convert the dom to xml file
'''

import file_info
from xml.dom.minidom import Document

def dom_to_pretty_xml(seqFileName, doc):
    '''
    @param filename: 文件名
    @param doc: 文件对象
    @todo: 将DOM对象转换为优雅的XML格式并生成到文件
    ''' 
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'file name is:', seqFileName
    if doc is None:
        print file_info.get_file_name(), file_info.get_line_number(), file_info.get_function_name(),'doc is',doc
        return
    if seqFileName.strip() == '':
        print file_info.get_file_name(), file_info.get_line_number(), file_info.get_function_name(),'file name is',seqFileName
        return
    
    try:
        open(seqFileName, 'w').write(doc.toprettyxml(indent = ' '))
    except Exception as e:
        print str(e)

def del_xml_head(szFileName):
    '''
    @param filename: XML文件名
    @todo: 去掉XML文件的声明
    '''   
    if szFileName == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'file name is', szFileName
        return
    seqLines = open(szFileName, 'r').readlines()
    open(szFileName, 'w').writelines(seqLines[1:])
    

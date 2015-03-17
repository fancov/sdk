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
    @param filename: �ļ���
    @param doc: �ļ�����
    @todo: ��DOM����ת��Ϊ���ŵ�XML��ʽ�����ɵ��ļ�
    ''' 
    
    if seqFileName.strip() == '':
        file_info.get_cur_runtime_info('filename is %s' % seqFileName)
        return -1
    
    if doc is None:
        file_info.get_cur_runtime_info('doc is %p' % doc)
        return -1
    
    file_info.get_cur_runtime_info('filename is %s' % seqFileName)
    if doc is None:
        file_info.get_cur_runtime_info('doc is %p' % doc)
        return -1
    if seqFileName.strip() == '':
        file_info.get_cur_runtime_info('seqFileName is %s' % seqFileName)
        return -1
    
    try:
        #open(seqFileName, 'w').write('hello')
        open(seqFileName, 'w').write(doc.toprettyxml(indent = ' '))
        return 1
    except Exception, err:
        file_info.get_cur_runtime_info('Catch Exception: %s' % str(err))
        return -1

def del_xml_head(szFileName):
    '''
    @param filename: XML�ļ���
    @todo: ȥ��XML�ļ�������
    '''   
    
    if szFileName == '':
        file_info.get_cur_runtime_info('szFileName is %s' % szFileName)
        return -1
    seqLines = open(szFileName, 'r').readlines()
    open(szFileName, 'w').writelines(seqLines[1:])
    return 1
    

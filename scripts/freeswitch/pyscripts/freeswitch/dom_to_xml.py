# -*- encoding=utf-8 -*-

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: convert the dom to xml file
'''

import file_info

def dom_to_pretty_xml(filename, doc): 
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'file name is:', filename
    if doc is None:
        print file_info.get_file_name(), file_info.get_line_number(), file_info.get_function_name(),'doc is',doc
        return
    if filename.strip() == '':
        print file_info.get_file_name(), file_info.get_line_number(), file_info.get_function_name(),'file name is',filename
        return
    fp = open(filename, 'w')
    fp.write(doc.toprettyxml(indent = ' '))
    fp.close()

def del_xml_head(filename):
    if filename == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'file name is', filename
        return
    lines = open(filename, 'r').readlines()
    open(filename, 'w').writelines(lines[1:])
'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@togo: get the data of python interpreter
'''

import file_info

def get_data_from_txt(filename):
    if filename.strip() == '' is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'file name is', filename 
        return None
    if is_txt_file(filename) is False:
        return None
    f = open(filename, 'r').readlines()
    for loop in range(len(f)):
        if f[loop].endswith('\n') is True:
            f[loop] = f[loop][:-1]
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sips are:\n', f
    return f

def is_txt_file(filename):
    
    if filename.endswith('.txt') is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'file name is', filename
        return True
    return False
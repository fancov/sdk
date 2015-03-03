'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: get the data of python interpreter
'''

import file_info

def get_data_from_txt(filename):
    '''
    @param filename: 文件名
    @todo: 从txt文件中获取sip账户id列表
    '''
    if filename.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'file name is', filename 
        return None
    if is_txt_file(filename) is False:
        return None
    f = open(filename, 'r').readlines()
    
    # 去掉结尾的换行符
    for loop in range(len(f)):
        if f[loop].endswith('\n'):
            f[loop] = f[loop][:-1]
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sips are:\n', f
    return f

def is_txt_file(filename):
    '''
    @param filename: 文件名
    @todo: 判断文件后缀名是否为.txt后缀
    '''
    if filename.endswith('.txt'):
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'file name is', filename
        return True
    return False


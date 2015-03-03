'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@togo: pakete the operation about database
'''

import data_source
import command_param
import file_info

def batch_generate():
    '''
    @todo: 批量生成sip账户
    '''
    
    # 获取输入参数
    list = command_param.get_param()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'list is', list
    
    # 如果指定的是参数'-e'，那么使用枚举方法生成sip账户
    if list[1].lower() == '-e':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'Enumeration method'
        list = list[2:]
        data_source.generate_by_enum(list)
        return
    # 如果指定的参数'-r'，表示使用指定特定范围的方法生成sip账户
    elif list[1].lower() == '-r':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'Specify a particular range'
        list = list[2:]
        data_source.generate_by_range(list)
        return
    # 如果执行的参数为'-m'，表示读取Excel表格中sip账户数据去生成sip账户
    elif list[1].lower() == '-m':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'data from Excel'
        list = list[2:]
        data_source.generate_by_excel(list)
        return
    # 如果指定的参数'-t'，表示读取txt文件中sip账户数据区生成sip账户
    elif list[1].lower() == '-t':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'data from txt'
        list = list[2:]
        data_source.generate_by_txt(list)
        return
    # 如果指定参数'-d'，表示通过指定特定的数据库的特定字段中的数据生成sip账户
    elif list[1].lower() == '-d':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'Field in the database table'
        list = list[2:]
        data_source.generate_by_db(list)
        return
    
    
if __name__ == "__main__":
    batch_generate()
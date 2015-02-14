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
    list = command_param.get_param()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'list is', list
    if list[1].lower() == '-e':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'Enumeration method'
        list = list[2:]
        data_source.generate_by_enum(list)
        return
    elif list[1].lower() == '-r':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'Specify a particular range'
        list = list[2:]
        data_source.generate_by_range(list)
        return
    elif list[1].lower() == '-m':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'data from Excel'
        list = list[2:]
        data_source.generate_by_excel(list)
        return
    elif list[1].lower() == '-t':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'data from txt'
        list = list[2:]
        data_source.generate_by_txt(list)
        return
    elif list[1].lower() == '-d':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'Field in the database table'
        list = list[2:]
        data_source.generate_by_db(list)
        return
    
    
if __name__ == "__main__":
    batch_generate()
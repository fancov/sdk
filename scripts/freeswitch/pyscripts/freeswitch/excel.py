# -*- encoding=utf-8 -*-
'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 6th,2015
@todo: operation about Excel files
'''

import xdrlib, sys
import xlrd
import types
import file_info
 
#获取表格     
def get_data_from_table(filename, sheet_idx = 0):
    if filename.strip() == '' is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'filename is', filename
        return None
    if sheet_idx < 0 is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sheet_idx is', sheet_idx
        return None
    book = xlrd.open_workbook(filename, 'rb')
    return book.sheet_by_index(sheet_idx) #sheet_idx index
        
#根据行索引获取表格中的数据
def excel_table_by_row(filename, row = 0, sheet_idx = 0):
    if filename.strip() == '' is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'filename is', filename
        return None
    if row < 0 is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'row is', row
        return None
    if sheet_idx < 0 is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sheet_idx is', sheet_idx
        return None
    table = get_data_from_table(filename, sheet_idx)
    rowlist = table.row_values(row)
    for loop in range(len(rowlist)):
        rowlist[loop] = float_to_int(rowlist[loop])
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'rowlist is:', rowlist
    return rowlist

#根据列索引获取表格中的数据
def excel_table_by_col(filename, col = 0, sheet_idx = 0):
    if filename.strip() == '' is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'filename is', filename
        return None
    if col < 0 is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'col is', col
        return None
    if sheet_idx < 0 is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sheet_idx is', sheet_idx
        return None
    table = get_data_from_table(filename, sheet_idx)
    collist = table.col_values(col)
    for loop in range(len(collist)):
        collist[loop] = float_to_int(collist[loop])
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'collist is:', collist
    return collist

#根据单元格所在位置获取内容
def excel_table_by_cell(filename, row = 0, col = 0, sheet_idx = 0):
    if filename.strip() == '' is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'filename is', filename
        return None
    if row < 0 is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'row is', row
        return None
    if col < 0 is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'col is', col
        return None
    if sheet_idx < 0 is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sheet_idx is', sheet_idx
        return None
    table = get_data_from_table(filename, sheet_idx)
    value = table.cell(row, col).value
    value = float_to_int(value)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'value is', value
    return value

#获取整张表数据
def excel_table_by_table(filename, sheet_idx = 0):
    if filename.strip() == '' is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'filename is', filename
        return None
    table = get_data_from_table(filename, sheet_idx)
    list = []
    nrows = table.nrows
    for loop in range(nrows):
        rowlist = excel_table_by_row(filename, loop, sheet_idx)
        list.append(rowlist)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is:', list
    return list

def is_float_str(num):
    if str(num).endswith('.0'):
        return True
    return False

def float_to_int(num):
    if type(num) == types.FloatType:
        return str(int(num))
    elif  type(num) == types.StringType and is_float_str(num) is True:
        num = num[:-2] #去掉结尾的‘.0’子串
        return num

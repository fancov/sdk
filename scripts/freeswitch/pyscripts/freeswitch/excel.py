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
 
  
def get_data_from_table(filename, sheet_idx = 0):
    '''
    @param filename: 文件名
    @param sheet_idx: Excel表单编号，第一张表单为编号0，默认从编号为0的表单读取数据
    @todo: 从Excel文件中获取数据
    '''
    if filename.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'filename is', filename
        return None
    if sheet_idx < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sheet_idx is', sheet_idx
        return None
    book = xlrd.open_workbook(filename, 'rb')
    return book.sheet_by_index(sheet_idx) #sheet_idx index
        
def excel_table_by_row(filename, row = 0, sheet_idx = 0):
    '''
    @param filename: 文件名
    @param row: sip数据列表所在的行编号，第1行编号为0，默认从编号为0的行读取数据
    @param sheet_idx: sip数据所在的表单编号，第一张表编号为0,默认从编号为0的表单读取数据
    @todo:  读取Excel某行数据
    '''
    if filename.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'filename is', filename
        return None
    if row < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'row is', row
        return None
    if sheet_idx < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sheet_idx is', sheet_idx
        return None
    table = get_data_from_table(filename, sheet_idx)
    rowlist = table.row_values(row)
    for loop in range(len(rowlist)):
        rowlist[loop] = float_to_int(rowlist[loop])
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'rowlist is:', rowlist
    return rowlist


def excel_table_by_col(filename, col = 0, sheet_idx = 0):
    '''
    @param filename: 文件名
    @param col: sip数据列表所在的列编号，第1列编号为0，默认从编号为0的列读取数据
    @param sheet_idx: sip数据所在的表单编号，第一张表编号为0,默认从编号为0的表单读取数据
    @todo:  读取Excel某列数据
    '''
    if filename.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'filename is', filename
        return None
    if col < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'col is', col
        return None
    if sheet_idx < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sheet_idx is', sheet_idx
        return None
    table = get_data_from_table(filename, sheet_idx)
    collist = table.col_values(col)
    for loop in range(len(collist)):
        collist[loop] = float_to_int(collist[loop])
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'collist is:', collist
    return collist

def excel_table_by_cell(filename, row = 0, col = 0, sheet_idx = 0):
    '''
    @param filename: 文件名
    @param row: 单元格所在行编号，第一行的行编号为0，默认为编号为0的行
    @param col: 单元格所在列表好，第一列的列表好为0，默认为编号为0的列
    @param sheet_idx: sip数据所在的表单编号，第一张表编号为0,默认从编号为0的表单读取数据
    @todo: 获取某一个单元格的数据
    '''
    if filename.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'filename is', filename
        return None
    if row < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'row is', row
        return None
    if col < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'col is', col
        return None
    if sheet_idx < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sheet_idx is', sheet_idx
        return None
    table = get_data_from_table(filename, sheet_idx)
    value = table.cell(row, col).value
    value = float_to_int(value)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'value is', value
    return value


def excel_table_by_table(filename, sheet_idx = 0):
    '''
    @param filename: 文件名
    @param sheet_idx: sip数据所在的表单编号，第一张表编号为0,默认从编号为0的表单读取数据
    @todo: 获取某一张Excel表格中的所有数据
    '''
    if filename.strip() == '':
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
    '''
    @param num: 数字
    @todo: 判断是否为浮点数
    '''
    if str(num).endswith('.0'):
        return True
    return False

def float_to_int(num):
    '''
    @param num: 数字
    @todo: 浮点型转换为整形
    '''
    if type(num) == types.FloatType:
        return str(int(num))
    elif  type(num) == types.StringType and is_float_str(num):
        num = num[:-2] #去掉结尾的‘.0’子串
        return num

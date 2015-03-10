# coding=utf-8

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
 
  
def get_data_from_table(seqFileName, ulSheetIdx = 0):
    '''
    @param filename: 文件名
    @param sheet_idx: Excel表单编号，第一张表单为编号0，默认从编号为0的表单读取数据
    @todo: 从Excel文件中获取数据
    '''
    if seqFileName.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'filename is', seqFileName
        return None
    if ulSheetIdx < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sheet_idx is', ulSheetIdx
        return None
    book = xlrd.open_workbook(seqFileName, 'rb')
    return book.sheet_by_index(ulSheetIdx) #sheet_idx index
        
def excel_table_by_row(seqFileName, ulRow = 0, ulSheetIdx = 0):
    '''
    @param filename: 文件名
    @param row: sip数据列表所在的行编号，第1行编号为0，默认从编号为0的行读取数据
    @param sheet_idx: sip数据所在的表单编号，第一张表编号为0,默认从编号为0的表单读取数据
    @todo:  读取Excel某行数据
    '''
    if seqFileName.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'filename is', seqFileName
        return None
    if ulRow < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'row is', ulRow
        return None
    if ulSheetIdx < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sheet_idx is', ulSheetIdx
        return None
    table = get_data_from_table(seqFileName, ulSheetIdx)
    listRowList = table.row_values(ulRow)
    for loop in range(len(listRowList)):
        listRowList[loop] = float_to_int(listRowList[loop])
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'rowlist is:', listRowList
    return listRowList


def excel_table_by_col(seqFileName, ulCol = 0, ulSheetIdx = 0):
    '''
    @param filename: 文件名
    @param col: sip数据列表所在的列编号，第1列编号为0，默认从编号为0的列读取数据
    @param sheet_idx: sip数据所在的表单编号，第一张表编号为0,默认从编号为0的表单读取数据
    @todo:  读取Excel某列数据
    '''
    if seqFileName.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'filename is', seqFileName
        return None
    if ulCol < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'col is', ulCol
        return None
    if ulSheetIdx < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sheet_idx is', ulSheetIdx
        return None
    table = get_data_from_table(seqFileName, ulSheetIdx)
    listColList = table.col_values(ulCol)
    for loop in range(len(listColList)):
        listColList[loop] = float_to_int(listColList[loop])
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'collist is:', listColList
    return listColList

def excel_table_by_cell(seqFileName, ulRow = 0, ulCol = 0, ulSheetIdx = 0):
    '''
    @param filename: 文件名
    @param row: 单元格所在行编号，第一行的行编号为0，默认为编号为0的行
    @param col: 单元格所在列表好，第一列的列表好为0，默认为编号为0的列
    @param sheet_idx: sip数据所在的表单编号，第一张表编号为0,默认从编号为0的表单读取数据
    @todo: 获取某一个单元格的数据
    '''
    if seqFileName.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'filename is', seqFileName
        return None
    if ulRow < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'row is', ulRow
        return None
    if ulCol < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'col is', ulCol
        return None
    if ulSheetIdx < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sheet_idx is', ulSheetIdx
        return None
    table = get_data_from_table(seqFileName, ulSheetIdx)
    value = table.cell(ulRow, ulCol).value
    value = float_to_int(value)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'value is', value
    return value


def excel_table_by_table(seqFileName, ulSheetIdx = 0):
    '''
    @param filename: 文件名
    @param sheet_idx: sip数据所在的表单编号，第一张表编号为0,默认从编号为0的表单读取数据
    @todo: 获取某一张Excel表格中的所有数据
    '''
    if seqFileName.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'filename is', seqFileName
        return None
    table = get_data_from_table(seqFileName, ulSheetIdx)
    list = []
    nRows = table.nrows
    for loop in range(nRows):
        rowlist = excel_table_by_row(seqFileName, loop, ulSheetIdx)
        list.append(rowlist)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'list is:', list
    return list

def is_float_str(ulNum):
    '''
    @param num: 数字
    @todo: 判断是否为浮点数
    '''
    if str(ulNum).endswith('.0'):
        return True
    return False

def float_to_int(ulNum):
    '''
    @param num: 数字
    @todo: 浮点型转换为整形
    '''
    if type(ulNum) == types.FloatType:
        return str(int(ulNum))
    elif  type(ulNum) == types.StringType and is_float_str(ulNum):
        ulNum = ulNum[:-2] #去掉结尾的‘.0’子串
        return ulNum

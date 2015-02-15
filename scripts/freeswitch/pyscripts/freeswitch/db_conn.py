# -*- encoding=utf-8 -*-
'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: get the database connection
'''

import MySQLdb
import db_config

CONN = None

def connect_db():
    global CONN
    dict = db_config.get_db_param()
    list = []
    for item in dict:
        list.append(dict[item])
    CONN = MySQLdb.connect(list[1], list[0], list[4], list[2], int(list[5])) #connect database
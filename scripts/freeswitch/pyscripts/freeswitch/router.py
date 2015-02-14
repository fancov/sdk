'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 9rd,2015
@todo: generate the directories and management xml files
'''
from xml.dom.minidom  import Document
import db_conn
import file_info
import db_config
import os
from db_conn import CONN
import dom_to_xml

def phone_route():
    global CONN
    
    doc = Document()
    
    db_conn.connect_db()
    sql_cmd = 'select distinct CONVERT(id, CHAR(10)) as id from tbl_relaygrp'
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:', sql_cmd
    cursor = db_conn.CONN.cursor()
    cursor.execute(sql_cmd)
    results = cursor.fetchall()
    
    cfg_dir = db_config.get_db_param()['cfg_path']
    if cfg_dir[-1] != '/':
        cfg_dir = cfg_dir + '/'
    
    cfg_dir = cfg_dir + 'sip_profiles/external/'
    if os.path.exists(cfg_dir) is False:
        os.makedirs(cfg_dir)
        
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'results:', sql_cmd
    for loop in range(len(results)):
        include_node = make_route(int(results[loop][0]), doc)
        cfg_path = cfg_dir + results[loop][0] + '.xml'
        dom_to_xml.dom_to_pretty_xml(cfg_path, doc)
        dom_to_xml.del_xml_head(cfg_path)
        
    
    
def get_route_param(id):
    '''
    @todo: create router table
    '''
    global CONN 
    db_conn.connect_db()
    sql_cmd = 'select name,username,password,realm,form_user,form_domain,extension,proxy,reg_proxy,expire_secs,CONVERT(register, CHAR(10)) as register,reg_transport,CONVERT(retry_secs, CHAR(20)) as retry_secs, CONVERT(cid_in_from,CHAR(20)) as cid_in_from,contact_params, CONVERT(exten_in_contact, CHAR(20))as exten_in_contact,CONVERT(ping, CHAR(20)) as ping from tbl_relaygrp where id=%d' % (id)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',sql_cmd
    cursor = db_conn.CONN.cursor()
    cursor.execute(sql_cmd)
    results = cursor.fetchall()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'results:',results
    db_conn.CONN.close()
    return results

def make_route(id, doc):
    results = get_route_param(id)  
    gateway_node = doc.createElement('gateway')
    gateway_node.setAttribute('name', results[0][0])
    
    param_names  = ['username', 'realm', 'from-user', 'from-domain', 'password', 'extension', 'proxy', 'register-proxy', 'expire-seconds', 'register', 'reg_transport', 'retry_secs', 'caller-id-in-from', 'contact_params','exten_in_contact', 'ping']
    param_values = [results[0][1], results[0][3], results[0][4], results[0][5], results[0][2], results[0][6], results[0][7], results[0][9], results[0][8], results[0][10], results[0][11], results[0][12], results[0][13], results[0][14], results[0][15], results[0][16]]
    
    for loop in range(len(param_names)):
        param_node = doc.createElement('param')
        param_node.setAttribute('name', param_names[loop].strip())
        if param_names[loop].strip() == 'register' or param_names[loop].strip() == 'caller-id-in-from':
            if param_values[loop].strip() == '0':     
                param_node.setAttribute('value', 'false')
            else:
                param_node.setAttribute('value', 'true')
        else:
            param_node.setAttribute('value', param_values[loop].strip())
        
        gateway_node.appendChild(param_node)
        
    include_node = doc.createElement('include')
    include_node.appendChild(gateway_node)
    doc.appendChild(include_node)

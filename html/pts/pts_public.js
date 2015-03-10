/* index.html 自动访问 */
function auto_access()
{
    var internetIP = document.getElementById("auto_internet_ip").value;
    if (internetIP == "")
    {
        alert("目的公网ip不能为空");
        return false;
    }
    var intranetIP = document.getElementById("auto_intranet_ip").value;
    if (intranetIP == "")
    {
        intranetIP = "127.0.0.1";
    }
    var port = document.getElementById("auto_port").value;
    if (port == "")
    {
        port = 80;
    }
    
    var href = "internetIP="+internetIP+"&intranetIP="+intranetIP+"&port="+port;
    
    window.open(href);
}

/* 检查IP */
function check_IP(ip)
{
    var re=/^(\d+)\.(\d+)\.(\d+)\.(\d+)$/;
    if(re.test(ip))
    {
        if( RegExp.$1<256 && RegExp.$2<256 && RegExp.$3<256 && RegExp.$4<256)
        return true;
    }
    return false;
}

/* 手动访问 */
function visit_dev()
{
    var id = "";
    var pms = "";
    var ptcs = document.getElementsByName("PtcCheckBox");

    for(var i = 0; i < ptcs.length; i++) 
    {
        if (ptcs[i].checked)
        {
            id = ptcs[i].value;
        }
    }

    if (id == "")
    {
        alert("please select ptc");
        return false;
    }
    pms += "id="+id+"&";
    var ip = document.getElementById("manual_intranet_ip").value;
    var port = document.getElementById("manual_port").value;
    if ("" != ip)
    {
        if (check_IP(ip))
        {
            pms += "ip="+ip+"&";
        }
        else
        {
            alert("ip format error");
            return false;
        }
    }
    else
    {
        pms += "ip=127.0.0.1&";
    }

    if ("" != port)
    {
        if (port > 65535)
        {
            alert("port error");
            return false;
        }
        else
        {
            pms += "port="+port;
        }
    }
    else
    {
        pms += "port=80";
    }

    var href = "visit_device?" + pms;

    window.open(href);
}

/* ptc_manage.html 切换 */
function switch_pts()
{
    var id = "";
    var pms = "";
    var ptcs = document.getElementsByName("PtcCheckBox");
    var xmlhttp = null;

    for(var i = 0; i < ptcs.length; i++) 
    {
        if (ptcs[i].type=='checkbox' && ptcs[i].checked)
        {
            if (id == "")
            {
                id = ptcs[i].value;
            }
            else
            {
                id += "!"+ptcs[i].value;
            }
        }
    }

    if (id == "")
    {
        alert("please select ptc");
        return false;
    }
    var domain = document.getElementById("switch_domain").value;
    var port = document.getElementById("switch_port").value;

    if ("" != port)
    {
        if (port > 65535)
        {
            alert("port error");
            return false;
        }
    }
    else
    {
        port = 50000;
    }

    var url = "/cgi-bin/ptcSwitchpts?id="+id;
    url += "&domain="+domain+"&port="+port;

    if (window.XMLHttpRequest)
    {
        xmlhttp=new XMLHttpRequest();
    }
    else
    {
        xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
    }
    xmlhttp.onreadystatechange=function()
    {
        if (xmlhttp.readyState==4 && xmlhttp.status==200)
        {
            var result = xmlhttp.responseText.indexOf("succ");
            if (result >= 0)
            {
                location.reload();
            }
            else
            {
                 alert("fail");
            }
        }
    }
    
    xmlhttp.open("GET", url, true);
    xmlhttp.setRequestHeader("If-Modified-Since", "0");
    xmlhttp.send();
}

/* ptc_config.html */
function change_domain(type)
{
    var domain = "";
    var port = 0;
    var id = "";
    var url = "/cgi-bin/change_domain?";

    if (0 == type)
    {
        domain = document.getElementById("config_major_doamin").value;
        port = document.getElementById("config_major_port").value;
    }
    else
    {
        domain = document.getElementById("config_minor_domain").value;
        port = document.getElementById("config_minor_port").value;
    }
    

    if (domain == '')
    {
        domain = "nil";
    }

    if (port == '')
    {
        port = "nil";
    }

    if (domain == "nil" && port == "nil")
    {
        alert("域名和端口不能同时为空");
        return false;
    }

    var ptcs = document.getElementsByName("PtcCheckBox");

    for(var i = 0; i < ptcs.length; i++) 
    {
        if (ptcs[i].type=='checkbox' && ptcs[i].checked)
        {
            if (id == "")
            {
                id = ptcs[i].value;
            }
            else
            {
                id += "!"+ptcs[i].value;
            }
        }
    }

    if (id == "")
    {
        alert("please select ptc");
        return false;
    }

    url += "type="+type+"&domain="+domain+"&port="+port+"&id="+id;

    submit_change_domain_url(url);
}

function submit_change_domain_url(url)
{
    var xmlhttp = null;
    if (window.XMLHttpRequest)
    {
        xmlhttp=new XMLHttpRequest();
    }
    else
    {
        xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
    }
    xmlhttp.onreadystatechange=function()
    {
        if (xmlhttp.readyState==4 && xmlhttp.status==200)
        {
            setTimeout("location.reload()", 2000);
        }
    }
    xmlhttp.open("GET",url,true);
    xmlhttp.setRequestHeader("If-Modified-Since", "0");
    xmlhttp.send();
}

/* system_config.html */
function create_user()
{
    var username = document.getElementById("u14").value;
    var password = document.getElementById("u15").value;
    var password2 = document.getElementById("u16").value;
    var url = "/cgi-bin/ptsCreateNewUser?";

    if (username == '' || password == ''|| password2 == '')
    {
        alert("请输入完整信息");
        return false;
    }
    if (password != password2)
    {
        alert("两次密码输入不对应");
        return false;
    }
    
    url += "username="+username+"&password="+password;
    
    submit_create_user_url(url);
}

function submit_create_user_url(url)
{
    var xmlhttp = null;
    if (window.XMLHttpRequest)
    {
        xmlhttp=new XMLHttpRequest();
    }
    else
    {
        xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
    }
    xmlhttp.onreadystatechange=function()
    {
        if (xmlhttp.readyState==4 && xmlhttp.status==200)
        {
            var start = xmlhttp.responseText.indexOf("<p>")+3;
            var end = xmlhttp.responseText.indexOf("</p>");
            alert(xmlhttp.responseText.substring(start, end));
            create_user_reset();
        }
    }
    xmlhttp.open("GET",url,true);
    xmlhttp.setRequestHeader("If-Modified-Since", "0");
    xmlhttp.send();
}

function form_check_package()
{
    if (document.getElementById("PackageFile").value == "")
    {
        alert("软件包: 请选择一个软件包文件!");
        return false;
    }
    var url = document.HttpPackageUpload.action;
    var id = "";
    var ptcs = document.getElementsByName("PtcCheckBox");

    for(var i = 0; i < ptcs.length; i++) 
    {
        if (ptcs[i].type=='checkbox' && ptcs[i].checked)
        {
            if (id == "")
            {
                id = ptcs[i].value;
            }
            else
            {
                id += "!"+ptcs[i].value;
            }
        }
    }

    if (id == "")
    {
        alert("please select ptc");
        return false;
    }

    url += "id="+id;

    document.getElementById('HttpPackageUpload').action=url;
    
    return true;
}


/* 创建新用户 */
function checkna()
{
    var na = form1.name.value;
	if (na.length <1 || na.length >12)
	{
        	divname.innerHTML='<font class="tips_false">长度是1~12个字符</font>';
	}
	else
	{
        	divname.innerHTML='<font class="tips_true">输入正确</font>';
	}
   
  }
//验证密码
function checkpsd1()
{
    var psd1=form1.password.value;  
    var flagZM=false ;
    var flagSZ=false ; 
    var flagQT=false ;
    if(psd1.length<6 || psd1.length>12)
    {
        divpassword1.innerHTML='<font class="tips_false">长度错误</font>';
    }
    else
    {
        divpassword1.innerHTML='<font class="tips_true">输入正确</font>';
    }
}

function checkpsd2()
{
    var psw1 = form1.password.value;
    var psw2 = form1.password2.value;

    if(psw1 != psw2 || psw1 == "")
    { 
         divpassword2.innerHTML='<font class="tips_false">您两次输入的密码不一样</font>';
    }
    else 
    {
         divpassword2.innerHTML='<font class="tips_true">输入正确</font>';
    }
}
//验证邮箱

function checkmail()
{
    apos=form1.mailbox.value.indexOf("@");
    dotpos=form1.mailbox.value.lastIndexOf(".");
    if (apos<1 || dotpos-apos<2) 
    {
        divmail.innerHTML='<font class="tips_false">输入错误</font>' ;
    }
    else
    {
        divmail.innerHTML='<font class="tips_true">输入正确</font>' ;
    }
}

function exit()
{
    document.execCommand('ClearAuthenticationCache'); 
}

function del_user()
{
    var url = "/cgi-bin/del_users?"
    var ptcs = document.getElementsByName("PtcCheckBox");
    var name = "";
    var xmlhttp = null;

    for(var i = 0; i < ptcs.length; i++) 
    {
        if (ptcs[i].type=='checkbox' && ptcs[i].checked)
        {
            if (ptcs[i].value == "admin")
            {
                alert("admin不可被删除");
                return false;
            }

            if (name == "")
            {
                name = ptcs[i].value;
            }
            else
            {
                name += "!"+ptcs[i].value;
            }
        }
    }

    if (name == "")
    {
        alert("please select user");
        return false;
    }

    url += "name="+name;

    if (window.XMLHttpRequest)
    {
        xmlhttp=new XMLHttpRequest();
    }
    else
    {
        xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");
    }
    xmlhttp.onreadystatechange=function()
    {
        if (xmlhttp.readyState==4 && xmlhttp.status==200)
        {
            //setTimeout("refersh('1')", 2000);
            location.reload();
        }
    }
    
    xmlhttp.open("GET", url, true);
    xmlhttp.setRequestHeader("If-Modified-Since", "0");
    xmlhttp.send();
}

function change_password()
{
    var ptcs = document.getElementsByName("PtcCheckBox");
    var url = "";
    var selectBoxCount = 0; 
    var name = "";

    for(var i = 0; i < ptcs.length; i++) 
    {
        if (ptcs[i].type=='checkbox' && ptcs[i].checked)
        {
            selectBoxCount++;
            name = ptcs[i].value;
        }
    }


    if (selectBoxCount < 1)
    {
        alert("请选择一个需要修改的用户");
        return false;
    }
    else if (selectBoxCount > 1)
    {
        alert("只能选择一个用户");
        return false;
    }

    url = "change_pwd.html?name=" + name;

    window.open(url, '_self');
}

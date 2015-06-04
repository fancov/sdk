var g_lang_type = 1;

function get_lang_type(langType)
{
    g_lang_type = langType;

    return true;
}

/* index.html 自动访问 */
function auto_access()
{
    var internetIP = document.getElementById("auto_internet_ip").value;
    if (internetIP == "")
    {
        if (g_lang_type)
        {
            alert("目的公网ip不能为空");    
        }
        else
        {

            alert("Public IP can not be empty");
        }
        
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
        if (g_lang_type)
        {
            alert("请选择PTC");    
        }
        else
        {
            alert("Please select PTC");
        }
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
            if (g_lang_type)
            {
                alert("请输入正确的端口");    
            }
            else
            {
                alert("Please enter right Port");
            }
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
        if (g_lang_type)
        {
            alert("请选择PTC");    
        }
        else
        {
            alert("Please select PTC");
        }
        return false;
    }
    var domain = document.getElementById("switch_domain").value;
    var port = document.getElementById("switch_port").value;

    if ("" != port)
    {
        if (port > 65535)
        {
            if (g_lang_type)
            {
                alert("请输入正确的端口");    
            }
            else
            {
                alert("Please enter right Port");
            }
            return false;
        }
    }
    else
    {
        port = 50000;
    }

    var url = "/cgi-bin/ptcSwitchpts?id="+id;
    url += "&domain="+domain+"&port="+port;
    show_cover();
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
                setTimeout("location.reload()", 1000);
            }
            else
            {
                hide_cover();
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
        if (g_lang_type)
        {
            alert("域名和端口不能同时为空");    
        }
        else
        {
            alert("Domain name or port can't be empty");
        }
        
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
        if (g_lang_type)
        {
            alert("请选择PTC");
        }
        else
        {
            alert("Please select PTC");
        }
        
        return false;
    }

    url += "type="+type+"&domain="+domain+"&port="+port+"&id="+id;
    show_cover();
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
        setTimeout("hide_cover()", 2000);

    }
    xmlhttp.open("GET",url,true);
    xmlhttp.setRequestHeader("If-Modified-Since", "0");
    xmlhttp.send();
}

function form_check_package()
{
    var filename = document.getElementById("PackageFile").value; 
    if (filename == "")
    {
        if (g_lang_type)
        {
            alert("请选择一个软件包文件!");    
        }
        else
        {
            alert("Please select a package file!");
        }
        
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
        if (g_lang_type)
        {
            alert("请选择PTC");
        }
        else
        {
            alert("Please select PTC");
        }
        
        return false;
    }

    url += "id="+id;

    document.getElementById('HttpPackageUpload').action = url;
    show_cover();
    return true;
}

/* 全网升级 */
function form_check_package_all()
{
    var filename = document.getElementById("PackageFileAll").value; 
    if (filename == "")
    {
        if (g_lang_type)
        {
            alert("请选择一个软件包文件!");    
        }
        else
        {
            alert("Please select a package file!");
        }
        return false;
    }

    show_cover();

    return true;
}

function get_ifream_content()
{
    var doc;
    if (document.all)
    {//IE
      doc = document.frames["hidden_frame"].document;
    }
    else
    {//Firefox   
      doc = document.getElementById("hidden_frame").contentDocument;
    }

    if (doc.body.innerHTML != "")
    {
        //hide_cover();
        location.reload();
    }

    return true;
}


/* 创建新用户 */
function checkna()
{
    var na = form1.name.value;
	if (na.length <1 || na.length >30)
	{
        if (g_lang_type)
        {
        	divname.innerHTML='<font class="tips_false">长度是1~30个字符</font>';
        }
        else
        {
            divname.innerHTML='<font class="tips_false">Length is 1 ~ 30 characters</font>';   
        }
	}
	else
	{
        if (g_lang_type)
        {
        	divname.innerHTML='<font class="tips_true">输入正确</font>';
        }
        else
        {
            divname.innerHTML='<font class="tips_true">Input right</font>';   
        }
	}
   
  }
//验证密码
function checkpsd1()
{
    var psd1=form1.password.value;  
    var flagZM=false ;
    var flagSZ=false ; 
    var flagQT=false ;
    if(psd1.length > 0)
    {
        if (g_lang_type)
        {
            divpassword1.innerHTML='<font class="tips_true">输入正确</font>';
        }
        else
        {
            divpassword1.innerHTML='<font class="tips_true">Input right</font>';
        }
    }
    else
    {
        if (g_lang_type)
        {
            divpassword1.innerHTML='<font class="tips_false">长度错误</font>';
        }
        else
        {
            divpassword1.innerHTML='<font class="tips_false">The length of the error</font>';
        }
        
    }
}

function checkpsd2()
{
    var psw1 = form1.password.value;
    var psw2 = form1.password2.value;

    if(psw1 != psw2 || psw1 == "")
    { 
        if (g_lang_type)
        {
            divpassword2.innerHTML='<font class="tips_false">您两次输入的密码不一样</font>';
        }
        else
        {
            divpassword2.innerHTML='<font class="tips_false">The two time you enter the password is not the same</font>';   
        }
    }
    else 
    {
        if (g_lang_type)
        {
            divpassword2.innerHTML='<font class="tips_true">输入正确</font>';
        }
        else
        {
            divpassword2.innerHTML='<font class="tips_true">Input right</font>';   
        }
    }
}
//验证邮箱

function checkmail()
{
    apos=form1.mailbox.value.indexOf("@");
    dotpos=form1.mailbox.value.lastIndexOf(".");
    if (apos<1 || dotpos-apos<2)
    {
        if (g_lang_type)
        {
            divmail.innerHTML='<font class="tips_false">输入错误</font>' ;
        }
        else
        {
            divmail.innerHTML='<font class="tips_false">Input error</font>' ;   
        }
    }
    else
    {
        if (g_lang_type)
        {
            divmail.innerHTML='<font class="tips_true">输入正确</font>' ;
        }
        else
        {
            divmail.innerHTML='<font class="tips_true">Input right</font>' ;   
        }
    }
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
                if (g_lang_type)
                {
                    alert("admin不可被删除");
                }
                else
                {
                    alert("admin can not be deleted");   
                }
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
        if (g_lang_type)
        {
            alert("请选择一个需要修改的用户");
        }
        else
        {
            alert("Please select a user to modify");   
        }
        return false;
    }
    else if (selectBoxCount > 1)
    {
        if (g_lang_type)
        {
            alert("只能选择一个用户");
        }
        else
        {
            alert("You can only select one user");
        }
        return false;
    }

    url = "change_pwd.html?name=" + name;

    window.open(url, '_self');
}

/* 退出按钮 */
function exit()
{
    try  
    {  
      if(navigator.userAgent.indexOf("MSIE")>0)         //IE浏览器实现注销功能  
      {  
        document.execCommand("ClearAuthenticationCache");  
      }  
      else if(navigator.userAgent.indexOf("Firefox")>0)       //Firefox实现注销功能  
      {  
        var xmlhttp = createXMLObject();  
        xmlhttp.open("GET",".force_logout_offer_login_mozilla",true,"logout","logout");  
        xmlhttp.send("");  
        xmlhttp.abort();  
      }  
      else       //Google Chrome等浏览器实现注销功能  
      {  
         var xmlHttp = false;  
         if(window.XMLHttpRequest)
         {  
            xmlHttp = new XMLHttpRequest();  
         }  
         xmlHttp.open("GET", "./", false, "logout", "logout");    
         xmlHttp.send(null);  
         xmlHttp.abort();  
      }  
    }  
    catch(e)
    {  
        alert("there was an error");  
        return false;  
    }  
    window.location = "index.html";
}

function createXMLObject()  
{  
    try  
    {  
        if(window.XMLHttpRequest)  
        {  
            xmlhttp = new XMLHttpRequest();  
        }  
        // code for IE5、IE6  
        else if(window.ActiveXObject)  
        {  
            xmlhttp=new ActiveXObject("Microsoft.XMLHTTP");  
        }  
    }  
    catch(e)  
    {  
        xmlhttp=false;  
    }  
    return xmlhttp;  
}  

function select_all()
{
    var ptcs = document.getElementsByName("PtcCheckBox");
    var checkbox_state = document.getElementById("select_all").checked;
    var i;

    if (checkbox_state)
    {
        for(i = 0; i < ptcs.length; i++) 
        {
            ptcs[i].checked = true;
        }
    }
    else
    {
        for(i = 0; i < ptcs.length; i++) 
        {
            ptcs[i].checked = false;
        }
    }

    return true;
}

/* 邮箱地址合法性检测 */
function check_mail_format()
{
    var mail_value = document.getElementsByName("mailbox")[0].value;

    var regu = "^(([0-9a-zA-Z]+)|([0-9a-zA-Z]+[_.0-9a-zA-Z-]*[0-9a-zA-Z]+))@([a-zA-Z0-9-]+[.])+([a-zA-Z]{2}|net|NET|com|COM|gov|GOV|mil|MIL|org|ORG|edu|EDU|int|INT)$"
    var re = new RegExp(regu);
    if (mail_value.search(re) != -1) 
    {
        return true;
    } 
    else 
    {
        if (g_lang_type)
        {
            window.alert ("请输入有效合法的E-mail地址 ！")
        }
        else
        {
            window.alert ("Please enter a valid E-mail address!")   
        }
        return false;
    }
}


function formatSysTime() 
{
    var week_desc;
    if (g_lang_type)
    {
        week_desc = new Array("星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六");        
    }
    else
    {
        week_desc = new Array("Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday");    
    }
    var dt = new Date();
    var year = dt.getFullYear();
    var month = dt.getMonth() + 1;
    var date = dt.getDate();
    var day = dt.getDay();
    var hours = dt.getHours();
    var minutes = dt.getMinutes();
    var seconds = dt.getSeconds();
    return checkTime(year) + '-' + checkTime(month) + '-' + checkTime(date) + ' ' + week_desc[day] + ' ' +
            checkTime(hours) + ':' + checkTime(minutes) + ':' + checkTime(seconds);
}

function updateTime() 
{
    if ($('#system-time').length > 0) 
    {
        var fmt_time = formatSysTime();
        $('#system-time').html(fmt_time);
    }

    setTimeout(function(){updateTime();}, 1000);
}

function checkTime(i) 
{
    if (i < 10) {
        i = '0' + i;
    }
    return i;
}

function show_cover()
{ 
    var cover = document.getElementById("cover"); 
    var covershow = document.getElementById("coverShow"); 
    cover.style.display = "block"; 
    covershow.style.display = "block"; 
} 

function hide_cover(){ 
    var cover = document.getElementById("cover"); 
    var covershow = document.getElementById("coverShow"); 
    cover.style.display = "none"; 
    covershow.style.display = "none"; 
} 

function set_ping_dest(ping_dest)
{
    document.getElementById("PingDest").value = ping_dest;

    return true;
}


function is_number(num_string,nMin,nMax){
    var c;
    var ch = "-0123456789";
    if(num_string.length == 0){
        return false;
    }
    for (var i = 0; i < num_string.length; i++){
        c = num_string.charAt(i);
        if (ch.indexOf(c) == -1)
            return false;
    }
    if(parseInt(num_string) < nMin || parseInt(num_string) > nMax)
        return false;
    return true;
}

var g_ping_flag = false;
var g_time;

function StartPing()
{
    var PtcID = "";
    var PingNum = document.getElementById("PingNum").value;
    var PingPacketSize = document.getElementById("PingPacketSize").value;
    var ptcs = document.getElementsByName("PtcCheckBox");

    for(var i = 0; i < ptcs.length; i++) 
    {
        if (ptcs[i].checked)
        {
            PtcID = ptcs[i].value;
        }
    }

    if (PtcID == "")
    {
        if (g_lang_type)
        {
            alert("请选择PTC!");
        }
        else
        {
            alert("Please select PTC!");   
        }
        return false;
    }   
        
    if(!is_number(PingNum, 1, 100))
    {
        if (g_lang_type)
        {
            alert("The range of Ping is 1-100!");
        }
        else
        {

        }
        return false;
    }       
        
    if(!is_number(PingPacketSize, 56, 1024))
    {
        alert("'包长'范围为56-1024!");
        return false;
    }

    document.getElementById("InfoPing").value = "Start Ping "+ PtcID +" :\n";
    var url = "/cgi-bin/startPing?ptcID="+PtcID+"&PingPacketSize="+PingPacketSize;
    g_ping_flag = true;
    submit_ping_url(url, PtcID, PingNum, 0);
}

function StopPing()
{
    g_ping_flag = false;
    clearTimeout(g_time);
    var value = document.getElementById("InfoPing").value;
    value += "End\n"
    document.getElementById("InfoPing").value = value;

    return true;
}

function submit_ping_url(url, PtcID, PingNum, seq)
{
    var urlOriginal = url;
    if (seq < PingNum - 1)
    {
        if (g_ping_flag == true)
        {
            setTimeout(function(){submit_ping_url(urlOriginal, PtcID, PingNum, seq+1); }, 2000);        
        }
        else
        {
            return true;
        }
    }
    url += "&seq="+seq;
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
            //var value = document.getElementById("InfoPing").value;
            //value += xmlhttp.responseText;
            //document.getElementById("InfoPing").value = value;
            /* 获得结果从数据库中 */
            g_time = setTimeout(function(){get_ping_result_from_db(PtcID, PingNum, seq); }, 3000);
        }
    }
    xmlhttp.open("GET",url,true);
    xmlhttp.setRequestHeader("If-Modified-Since", "0");
    xmlhttp.send();
}


function get_ping_result_from_db(PtcID, PingNum, seq)
{
    var url = "/cgi-bin/get_ping_result?ptcID="+PtcID+"&seq="+seq;
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
            var value = document.getElementById("InfoPing").value;
            value = value+"seq="+seq+" : "+xmlhttp.responseText;
            if (seq == PingNum-1)
            {
                value += "end\n"
            }
            document.getElementById("InfoPing").value = value;
        }
    }
    xmlhttp.open("GET",url,true);
    xmlhttp.setRequestHeader("If-Modified-Since", "0");
    xmlhttp.send();
}


function cancel_upgrade()
{
    var url = "/cgi-bin/cancel_upgrade";

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
            location.reload();
        }
    }
    xmlhttp.open("GET",url,true);
    xmlhttp.setRequestHeader("If-Modified-Since", "0");
    xmlhttp.send();
}

function submit_username()
{
    var username = document.getElementById("name").value;

    var url = "/cgi-bin/set_license_username?username=" + username;

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
            var result = xmlhttp.responseText.indexOf("succ");
            
            if (result >= 0)
            {
                alert("succ");
            }
            else
            {
                alert("fail");
            }

            location.reload();           
        }
    }
    xmlhttp.open("GET",url,true);
    xmlhttp.setRequestHeader("If-Modified-Since", "0");
    xmlhttp.send();
}

function form_check_license()
{
    var filename = document.getElementById("LicenseFile").value; 
    if (filename == "")
    {
        if (g_lang_type)
        {
            alert("请选择一个Licnese文件!");    
        }
        else
        {
            alert("Please select a Licnese file!");
        }
        
        return false;
    }

    return true;
}
// all Javascripts used in WebAdmin should be added here
// lizhijie, 2006.07.22
// $Id: help.js,v 1.2 2007/02/05 20:50:59 lizhijie Exp $

function open_help_window(help_target )
{
	window.open (help_target, "newwindow", "height=250, width=550, top=80, left=25, toolbar=no, menubar=no, scrollbars=yes, resizable=no,location=no, status=no") 
}

function open_scan_preview_window(help_target )
{
	window.open (help_target, "newwindow", "height=650, width=750, top=5, left=5, toolbar=no, menubar=no, scrollbars=yes, resizable=no,location=no, status=no") 
}

function open_page( page_url)
{
	window.open(page_url, "mainframe" );
}

function open_page_view( page_url)
{
	window.open(page_url, "newwindow", "height=650, width=950, toolbar=no, menubar=no, scrollbars=yes, resizable=no,location=no, status=no");
}
function analyzeIP(num,str)
{
	if(num==3 && str<100)
		return false;
	if(num==2 && str<10)
		return false;
	
	return true;
}

function inspectIP(sIPAddress)
{
	var sIPAddress=sIPAddress;
	var IPsplit;
	var chkflag=true;
	var ErrMsg="你输入的是一个不规范的IP地址段！\nIP段为:xxx.xxx.xxx.xxx（xxx为0-255)!";
	var len=0;
	var substr_one;
	len=sIPAddress.length;
	var i1;
	var i2;
	var i3;
	var i=0;
	var sub_i;
	
	substr_one =sIPAddress.substring(i,i+1);
	i1=i;i++;
	
	if ( len <7 || len >15)
		chkflag =false;
	else
	{
		while ( i <len && substr_one !='.' && chkflag != false)
		{
			substr_one = sIPAddress.substring(i,i+1);
			i++;
		}
		sub_i =sIPAddress.substring(i1, i-1);
		if ( substr_one != '.' || sub_i >255 || i-i1 <2 || sub_i=="")
			chkflag=false;
		if(chkflag==false)
			return chkflag;
			
		sub_i = sIPAddress.substring(i1,i1+1);
		if ( sub_i == '0')
			chkflag = false;
		
		i1=i;
		substr_one = sIPAddress.substring(i,i+1);
		i++;
		
		while (i<len && substr_one !='.' && chkflag !=false)
		{
			substr_one = sIPAddress.substring(i,i+1);
			i++;
		}
		
		if (chkflag == true)
		{
			sub_i =sIPAddress.substring(i1, i-1);
			if ( substr_one != '.' || sub_i >255 || i-i1 <1||sub_i=="")
				chkflag=false;
			if(chkflag!=false)
				chkflag=analyzeIP(i-i1-1,sub_i);
			i1=i;
			
			substr_one = sIPAddress.substring(i,i+1);
			i++;
		}
		
		while (i<len && substr_one !='.' && chkflag !=false)
		{
			substr_one = sIPAddress.substring(i,i+1);
			i++;
		}
		
		if (chkflag == true)
		{
			sub_i =sIPAddress.substring(i1, i-1);
			if ( substr_one != '.' || sub_i >255 || i-i1 <1||sub_i=="")
				chkflag=false;
			if(chkflag!=false)
				chkflag=analyzeIP(i-i1-1,sub_i);
			i1=i;
			substr_one = sIPAddress.substring(i,i+1);
			i++;
		}
		while ( i <len && substr_one !='.'&&  chkflag != false)
		{
			substr_one = sIPAddress.substring(i,i+1);i++;
		}
		
		if (chkflag == true)
		{
			sub_i =sIPAddress.substring(i1, i);
			if ( substr_one == '.' || sub_i >254 || i-i1 <1||sub_i=="")
				chkflag=false;
				
			if(chkflag!=false && i-i1<4)
				chkflag=analyzeIP(i-i1,sub_i);
			else
				chkflag=false;
		}
	}
	return chkflag;
}

function checkIP(sIPAddress, me)
{
	var ret=true;
	ret=inspectIP(sIPAddress);
	if (ret==false)
	{
		me.value="";
		document.me.focus();
		return false;
	}
}

function checkIntegerNumber(obj)
{
	var objInput = obj;
	cod = window.event.keyCode;
	if ( !(  (cod >= 48) && (cod <= 57) ) )
	{
		alert("Only digital number can be input here!");
		window.event.keyCode = 0;
	}
}

// digit and '.' of IP address
function checkNumber(obj)
{
	var objInput = obj;
	cod = window.event.keyCode;
	if ( !(((cod >= 48) && (cod <= 57)) || (cod == 46) ))
	{
		Window.event.keyCode = 0;
	}
}


function winconfirm()
{
	var question;
	question = confirm("重启系统!你确定吗?")
	if (question == true)
	{
		window.open("/cgi-bin/sysconf.cgi?vt=1&ct=reboot","mainframe");
	}
}

function checkIsNotNULL(checkField, msg )
{
	if( checkField.value=="")
	{
		alert("" + msg );
		return false;
	}
	return true;
}

/* browser may be block this new window */
function alertConfirm(target, alertMsg )
{
	var question;
	question = confirm( alertMsg +"?")
	if (question == true)
	{
		window.open( target, "mainframe" );
	}
}

// submit button with alert msg 
function submitConfirm(alertMsg, formName )
{
	var question;
	question = confirm( alertMsg +"?")
	if (question == true)
	{
		formName.submit(); 
	}
}

function inc(obj)
{
	var i = 0;
//	alert("increase number!");
	var curValue = parseInt(obj.data.value);
//	alert("increase number!" + curValue);
	if (curValue < 9)
		curValue ++;
	else 
		return;


	obj.data.value = curValue;

//	obj.submit();
}

function dec(obj)
{
	var i = 0;
	var curValue = parseInt(obj.data.value);
	if (curValue > 1)
		curValue --; 
	else
		return;
	obj.data.value = curValue;

//	obj.submit();
}

function mid(obj)
{
	var i = 0;
	
	obj.data.value = 5;

//	obj.submit();
}

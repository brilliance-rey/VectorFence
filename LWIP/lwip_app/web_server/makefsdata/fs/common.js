var debug = true;

function addCookie(name, value, expiresMs){
	var cookieString = name + "=" + escape(value);
	if(expiresMs > 0){    //判断是否设置过期时间
		var date=new Date();
		date.setTime(date.getTime + expiresMs);
		cookieString = cookieString + "; expires=" + date.toGMTString();
	}
	document.cookie = cookieString;
}
function getCookie(name){
	var strCookie=document.cookie;
	var arrCookie=strCookie.split("; ");
	for(var i=0;i<arrCookie.length;i++){
		var arr=arrCookie[i].split("=");
		if(arr[0]==name){
			return arr[1];
		}
	}
	return "";
}
function deleteCookie(name){
	var date=new Date();
	date.setTime(date.getTime()-10000);
	document.cookie=name+"=v; expires="+date.toGMTString();
}

var usrCk = unescape(getCookie("username"));
var pswCk = unescape(getCookie("password"));

if(usrCk == "" || pswCk == ""){
	if(!debug){
		window.location="index.shtml";
		alert("对不起，请先进行登录！");
	}
}else if((usrCk == "xql" && pswCk == "admin@xql") || (usrCk == "admin" && pswCk == "shining")){
	//登录成功。。。
}else{
	if(!debug){
		window.location="index.shtml";
		alert("对不起，请重新登录！");
	}
}

/**
 * 是超级管理员
 * @return
 */
function isSU(){
	if(usrCk == "xql"){
		return true;
	}
	return debug? true: false;
}
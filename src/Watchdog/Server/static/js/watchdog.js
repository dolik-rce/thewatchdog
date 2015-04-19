function set_cnt(sel){
	loc=window.location.href.replace(/[&]*cnt=[0-9]*/g,'')+
		(location.search?'&':'?')+'cnt='+sel.options[sel.selectedIndex].text;
	window.location.replace(loc)
}

function set_page(sel,event){
	key=event.which || event.keyCode;
	if (key == 13)
		window.location.replace("?page="+sel.value);
	if (key <48 || key>57)
		event.preventDefault();
}

var refresh_timeout = null;

function init_refresh(){
	var interval = document.cookie.replace(/(?:(?:^|.*;\s*)autorefresh\s*\=\s*([^;]*).*$$)|^.*$$/, "$$1");
	var sel = document.getElementById("refresh");
	sel.value = interval;
	set_refresh(sel);
}

function disable_refresh(){
	var sel = document.getElementById("refresh");
	sel.parentNode.style.display="none";
}

function set_refresh(sel){
	var interval = sel.options[sel.selectedIndex].value;
	if (interval == 0) {
		clearTimeout(refresh_timeout);
	} else {
		refresh_timeout = setTimeout("window.location.reload(true);", 1000*interval);
	}
	var expires = new Date();
	expires.setDate(expires.getDate() + 30);
	document.cookie = "autorefresh="+interval+";path=/;expires="+expires.toUTCString();
	return false;
}

function feedfilter(){
	url=document.getElementById('url').value;
	var rss='';
	var filter='';
	var k = ["author","client","path","branch","status"];
	var s='';
	for (var i = 0; i < k.length; i++) {
		x=document.getElementById(k[i]).value;
		if (x!=''){
			rss+=s+k[i]+'='+encodeURIComponent(x);
			filter+=s+k[i]+'='+x;
			s='&';
		}
	}
	s=(s!='')?'?':'';
	document.getElementById('feedurl').innerHTML='<a hr'+'ef="'+url+s+rss+'">'+url+s+rss+'</a>';
	document.getElementById('filter').value=filter;
	var showWarning = document.getElementById('status').value != "" && document.getElementById('f_daily').checked;
	document.getElementById('status_warning').style.display=showWarning?"":"none;";
}

function getRadioValue (group){
	g=document.getElementsByName(group);
	for (var i = 0; i < g.length; i++)
		if (g[i].checked) return g[i].value;
}

function showFilter() {
	var e=document.getElementById("filter");
	e.style.height='6eM';
	e.childNodes[2].style.opacity=0;
}

function hideFilter(){
	var e=document.getElementById("filter");
	focus=document.getElementsByName('f_branch')[0]==document.activeElement ||
	      document.getElementsByName('f_author')[0]==document.activeElement ||
	      document.getElementsByName('f_msg')[0]==document.activeElement ||
	      document.getElementsByName('f_path')[0]==document.activeElement ||
	      document.getElementsByName('f_change')[0]==document.activeElement;
	if(!focus) {
		e.style.height='1.5eM';
		e.childNodes[2].style.opacity=1;
	}
}

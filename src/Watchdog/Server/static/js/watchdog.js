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

function feedfilter(){
	url=document.getElementById('url').value
	var filter='';
	var k = ["author","client","path","branch","status"]
	var s=''
	for (var i = 0; i < k.length; i++) {
		x=document.getElementById(k[i]).value
		if (x!=''){
			filter+=s+k[i]+'='+encodeURIComponent(x)
			s='&'
		}
	}
	s=(s!='')?'?':''
	document.getElementById('feedurl').innerHTML='<a href="'+url+s+filter+'">'+url+s+filter+'</a>'
	document.getElementById('filter').value=filter
}

function getRadioValue (group){
	g=document.getElementsByName(group);
	for (var i = 0; i < g.length; i++)
		if (g[i].checked) return g[i].value;
}

function showFilter(e) {
  e.style.height='6eM';
  e.childNodes[2].style.opacity=0;
}

function hideFilter(e){
  e.style.height='1.5eM'
  e.childNodes[2].style.opacity=1;
}

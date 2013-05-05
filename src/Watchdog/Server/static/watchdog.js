function set_cnt(sel){
	loc=window.location.href.replace(/[&]*cnt=[0-9]*/g,'')+
		(location.search?'&':'?')+'cnt='+sel.options[sel.selectedIndex].text;
	window.location.replace(loc)
}

function feedfilter(){
	url=document.getElementById('url').value
	var filter='';
	var k = ["author","client","path","status"]
	var s=''
	for (var i = 0; i < k.length; i++) {
		x=document.getElementById(k[i]).value
		if (x!=''){
			filter+=s+k[i]+'='+x
			s='&'
		}
	}
	s=(s!='')?'?':''
	document.getElementById('feedurl').innerHTML='<a href="'+url+s+filter+'">'+url+s+filter+'</a>'
	document.getElementById('filter').value=filter
}

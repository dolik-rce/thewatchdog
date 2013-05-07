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

function getRadioValue (group){
	g=document.getElementsByName(group);
	for (var i = 0; i < g.length; i++)
		if (g[i].checked) return g[i].value;
}

Graph = function(arg) {
	this.init = function(arg) {
		this.name = arg.name || ++Graph.count;
		arg.element.innerHTML =
			'<div id="y_axis_'+this.name+'" class="yaxis"></div>'+
			'<div id="chart_'+this.name+'" class="chart"></div>'+
			'<div id="x_axis_'+this.name+'" class="xaxis">'+
			'</div><div id="legend_'+this.name+'"></div>';
		this.graphcfg = {
			element: document.getElementById('chart_'+this.name),
			width: arg.element.offsetWidth-40-150,
			height: arg.element.offsetHeight-30,
			interpolation: 'line',
			series: arg.series,
			renderer: arg.renderer || 'bar',
			offset: arg.offset || 'zero',
			unstack: arg.unstack || false
		}
		this.showlegend = (arg.legend==undefined)?true:arg.legend;

		this.graph = new Rickshaw.Graph( this.graphcfg );
		
		this.y_axis = new Rickshaw.Graph.Axis.Y( {
			graph: this.graph,
			orientation: 'left',
			pixelsPerTick: 25,
			element: document.getElementById('y_axis_'+this.name),
		} );
		
		this.x_axis = new Rickshaw.Graph.Axis.X( {
			graph: this.graph,
			orientation: 'bottom',
			element: document.getElementById('x_axis_'+this.name),
			pixelsPerTick: 50
		} );
		if (this.showlegend) {
			this.legend = new Rickshaw.Graph.Legend( {
				graph: this.graph,
				element: document.getElementById('legend_'+this.name)
			} );
			this.toggler = new Rickshaw.Graph.Behavior.Series.Toggle( {
				graph: this.graph,
				legend: this.legend
			} );
		}
		this.defaultFormatters = {
			x: function(x) {return x;},
			y: function(y) {return y;},
		}
		this.annotations = new Rickshaw.Graph.HoverDetail( {
			graph: this.graph,
			xFormatter: arg.xFormatter || this.defaultFormatters.x,
			yFormatter: arg.yFormatter || this.defaultFormatters.y,
//			xFormatter: function(x) {
//					return x + ": " + revlog[x];
//				},
//			yFormatter: function(y) {
//				return y;
//			}
		} );
		this.render();
	}
	
	this.render = function() {
		this.graph.render();
		this.y_axis.render();
	}
	
	this.init(arg);
}

Graph.count = 0;

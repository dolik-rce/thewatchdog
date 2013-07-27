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

colorbar = function(args) {
	return new Rickshaw.Graph.Renderer.ColorBar(args);
}

Rickshaw.namespace('Rickshaw.Graph.Renderer.ColorBar');

Rickshaw.Graph.Renderer.ColorBar = Rickshaw.Class.create( Rickshaw.Graph.Renderer, {

	name: 'colorbar',

	defaults: function($super) {

		var defaults = Rickshaw.extend( $super(), {
			gapSize: 0.05,
			unstack: false
		} );

		delete defaults.tension;
		return defaults;
	},

	initialize: function($super, args) {
		args = args || {};
		this.gapSize = args.gapSize || this.gapSize;
		$super(args);
	},

	domain: function($super) {

		var domain = $super();

		var frequentInterval = this._frequentInterval();
		domain.x[1] += parseInt(frequentInterval.magnitude, 10);

		return domain;
	},

	barWidth: function() {

		var stackedData = this.graph.stackedData || this.graph.stackData();
		var data = stackedData.slice(-1).shift();

		var frequentInterval = this._frequentInterval();
		var barWidth = this.graph.x(data[0].x + frequentInterval.magnitude * (1 - this.gapSize)); 

		return barWidth;
	},

	render: function() {

		var graph = this.graph;

		graph.vis.selectAll('*').remove();

		var barWidth = this.barWidth();
		var barXOffset = 0;

		var activeSeriesCount = graph.series.filter( function(s) { return !s.disabled; } ).length;
		var seriesBarWidth = this.unstack ? barWidth / activeSeriesCount : barWidth;

		var transform = function(d) {
			// add a matrix transform for negative values
			var matrix = [ 1, 0, 0, (d.y < 0 ? -1 : 1), 0, (d.y < 0 ? graph.y.magnitude(Math.abs(d.y)) * 2 : 0) ];
			return "matrix(" + matrix.join(',') + ")";
		};

		graph.series.forEach( function(series) {

			if (series.disabled) return;

			var nodes = graph.vis.selectAll("path")
				.data(series.stack.filter( function(d) { return d.y !== null } ))
				.enter().append("svg:rect")
				.attr("x", function(d) { return graph.x(d.x) + barXOffset })
				.attr("y", function(d) { return (graph.y(d.y0 + Math.abs(d.y))) * (d.y < 0 ? -1 : 1 ) })
				.attr("width", seriesBarWidth)
				.attr("height", function(d) { return graph.y.magnitude(Math.abs(d.y)) })
				.attr("transform", transform);

			Array.prototype.forEach.call(nodes[0], function(node, n) {
				node.setAttribute('fill', series.color[n]);
			} );

			if (this.unstack) barXOffset += seriesBarWidth;

		}, this );
	},

	_frequentInterval: function() {

		var stackedData = this.graph.stackedData || this.graph.stackData();
		var data = stackedData.slice(-1).shift();

		var intervalCounts = {};

		for (var i = 0; i < data.length - 1; i++) {
			var interval = data[i + 1].x - data[i].x;
			intervalCounts[interval] = intervalCounts[interval] || 0;
			intervalCounts[interval]++;
		}

		var frequentInterval = { count: 0 };

		Rickshaw.keys(intervalCounts).forEach( function(i) {
			if (frequentInterval.count < intervalCounts[i]) {

				frequentInterval = {
					count: intervalCounts[i],
					magnitude: i
				};
			}
		} );

		this._frequentInterval = function() { return frequentInterval };

		return frequentInterval;
	}
} );

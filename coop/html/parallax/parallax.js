
//todo:
//  drag should maybe shift the parallax, not jump based on position?


	function TouchStart(ev) {
		ev.originalEvent.preventDefault();

		if (ev.touches.length == 1) {
			var p = $(ev.target).parent();
			MoveSvgStuff(p, ev.touches[0].pageX, ev.touches[0].pageY);
		}
	}

	function TouchMove(ev) {
		ev.originalEvent.preventDefault();

		if (ev.touches.length == 1) {
			var p = $(ev.target).parent();
			MoveSvgStuff(p, ev.touches[0].pageX, ev.touches[0].pageY);
		}
	}

	function TouchEnd(ev) {
		ev.originalEvent.preventDefault();
	}

	function MoveSvgStuff(parnt, pageX,pageY) {
		var svg = parnt;
		while (svg && !svg.is("svg")) svg = svg.parent();
		var pos = svg.offset();
		var w = svg.width();
		var h = svg.height();
		var x = (pageX - pos.left)/w; //so 0..1 covering the box
		var y = (pageY - pos.top) /h;

		parnt.children().each(function() {
			var o = $(this);
			if (!o.hasClass("parallaxSvgElement")) return;

			var z = o.data("z");

			o.attr("y", parseFloat(o.data("origTop"))  + (y-.5)*z*h/50)
			 .attr("x", parseFloat(o.data("origLeft")) + (x-.5)*z*w/50);
		});
	}

	function TouchHtmlStart(ev) {
		//console.log("start", ev, ev.target.className);
		ev.originalEvent.preventDefault();

		if (ev.touches.length == 1) {
			var p = $(ev.target)[0];
			MoveHtmlStuff(p, ev.touches[0].pageX, ev.touches[0].pageY);
		}
	}

	function TouchHtmlMove(ev) {
		//console.log("move", ev);
		ev.originalEvent.preventDefault();

		if (ev.touches.length == 1) {
			var p = $(ev.target)[0];
			MoveHtmlStuff(p, ev.touches[0].pageX, ev.touches[0].pageY);
		}
	}

	function TouchHtmlEnd(ev) {
		//console.log("end", ev);
		ev.originalEvent.preventDefault();
	}

	function MoveHtmlStuff(target, pageX,pageY) {
		var x = (pageX - target.offsetLeft)/target.clientWidth; //so 0..1 covering the box
		var y = (pageY - target.offsetTop) /target.clientHeight;
		//console.log("movestuff", target, $(target).children().length, target.className, pageX,pageY, x,y);

		$(target).children().each( function() {
			var j= $(this);
			var z = j.data("z");
			var origTop  = parseInt(j.data("origTop"), 10);
			var origLeft = parseInt(j.data("origLeft"), 10);
			//console.log(j.attr("src"), "top,left", origTop, origLeft, x,y, (y-.5)*z*target.clientHeight/50 ,origLeft + (x-.5)*z* target.clientWidth/50 );
			
			$(this).css({ top: origTop + (y-.5)*z*target.clientHeight/50, left: origLeft + (x-.5)*z* target.clientWidth/50 });
		});
	}

	$(document).ready(function(){
		//-----initialize svg based boxes
		var boxes = $(".parallaxSvg");

		boxes.each(function(index, box) {
			//console.log("box: ",index,box);

			var minz = 0, maxz = 0;
			$(box).children().each(function(index, child) {
				var jchild = $(child);
				if (jchild.hasClass("parallaxSvgElement")) {
				 	var o = $(child);
					var z = o.data("z");
					if (z < minz) minz = z;
					if (z > maxz) maxz = z;
					//console.log("box child", child, index);

					o.data("origLeft", o.attr("x")).data("origTop", o.attr("y"));

				} else if (jchild.hasClass("parallaxBG")) {
					jchild.bind('touchstart', TouchStart)
						  .bind('touchmove',  TouchMove)
						  .bind('touchend',   TouchEnd)
				}
			});
			//console.log("minz",minz,"maxz",maxz);
			$(box).data("minz", minz).data("maxz", maxz);
		});


		//-----initialize pure html element based boxes
		$(".comicsContainer").each(function(index, child) {
			var jchild = $(child)
			jchild.bind('touchstart', TouchHtmlStart)
				  .bind('touchmove',  TouchHtmlMove)
				  .bind('touchend',   TouchHtmlEnd)

			jchild.children().each(function() {
				var j = $(this);
				j.data("origTop", j.css("top")).data("origLeft", j.css("left"));

				//console.log("child of comicsContainer", j, j.css("top"), j.css("left"));
			});
		});


		$(document).mousemove(function(ev) {
			var jev = $(ev.target);
			//console.log(ev);

			//if (jev.hasClass("parallaxSvg")) {
			//	//console.log("ev.target is parallelbox: ",ev.target);
			//	jev.target.children().each(function() {
			//		var o = $(this);
			//		if (!o.hasClass("parallaxSvgElement")) return;
			//		o.attr("x", o.attr("x")+.01);
			//	});
            //
			//} else
			if (jev.hasClass("parallaxBG")) { //svg elements
				var p = jev.parent();
				MoveSvgStuff(p, ev.pageX, ev.pageY);

			} else if (jev.hasClass("comicsContainer")) {
				MoveHtmlStuff(ev.target, ev.pageX,ev.pageY);
			}
		});//doc mousemove
	});//doc ready


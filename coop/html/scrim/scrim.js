//
// Scrim
// Simple static html image gallery by Tom Lechner, 2019
// This file is MIT licensed (see LICENSE file)
//
// This reads in imagelist.json and creates a clickable, slideable, mouse or touch 
// sliding image gallery.
//
//


//TO DO
//-----
//one offs
//mobile zooming doesn't let you actually zoom
//make images right clickable? add faint "share..."
//put arrows outside the image, or make them fade quickly
//very tall images don't really work
//broken image link indication
//captions with html on top/bottom/left/right
//groups, multiple groups per page

//DONE automatic compilation of images on page
//DONE data-scrim="img" for any to add Expand funcs
//DONE show cancel icon
//DONE preload current: show loading icon while not yet loaded
//DONE key controls: esc, left, right
//DONE mobile sucks for main page, since title image is badly sized
//DONE swipe
//DONE encapsulate
//DONE preload adjacent
//DONE test json import
//DONE implement next/prev



//Scrim =  new function() {
var Scrim = new function(noptions) {

	var options = {
		jsonfile    : 'imagelist.json',
		nextimg     : 'images/next.png',
		previmg     : 'images/prev.png',
		loading     : 'images/loading.gif',
		closeimg    : 'images/close.png',
		use_rows    : false,
		gap         : 8,
		columnwidth : 200,
		rowheight   : 150,
		auto        : false
	};

	if (noptions) $.extend(options, noptions);
	//console.log('options: ',noptions, options);

	this.imagelist = [];

	var groups     = []; //todo: can be array of imagelists, select according to data-scrim="img[groupname]"
	var isoneoff   = false;

	var istouch = false;

	var docwidth  = window.innerWidth;
	var docheight = window.innerHeight;
	var smallscreen = ($(window).width() < 1025);
	console.log('screen size:',window.innerWidth,window.innerHeight);

	var numcolumns = Math.floor(docwidth/(options.columnwidth + options.gap));
	if (numcolumns < 1) numcolumns = 1;


	this.Options = function(noptions) {
		if (noptions) $.extend(options, noptions);
	};

	//object to simplify tracking mouse movements
	var ButtonDown = new function() {
		this.button = 0;
		this.initialX = 0;
		this.initialY = 0;
		this.initialTime = 0; //time clicked down
		this.lastX = 0;
		this.lastY = 0;
		this.lastTime = 0;
		this.x = 0;
		this.y = 0;
		this.time = 0;
		this.origObject = null;
		this.moveObject = null; //a jquery obj
		this.moveType = null;
		this.onlayer = false; //if last down was directly on layer, not on a control

		this.v_x = 0; //distance over time
		this.v_y = 0;

		this.down = function(x,y, which, obj, orig_obj) { //return number of milliseconds since last down
			this.button = which;
			this.x = x;
			this.y = y;
			this.initialX =  x;
			this.initialY =  y;
			this.lastX =  x;
			this.lastY =  y;
			this.moveObject = obj;
			this.origObject = orig_obj;
			this.v_x = 0;
			this.v_y = 0;
			var t = performance.now();
			var dt = t-this.initialTime;
			this.initialTime = t;
			return dt;
		}
		this.move = function(x,y) { //returns [dx, dy]
			this.lastX = this.x;
			this.lastY = this.y;
			this.lastTime = this.time;
			this.x = x;
			this.y = y;
			this.time = performance.now();
			this.v_x = (this.x-this.lastX)/(this.time - this.lastTime);
			this.v_y = (this.y-this.lastY)/(this.time - this.lastTime);
			return [this.x-this.lastX, this.y-this.lastY];
		}
		this.up = function(x,y) { //return distance between start point and end point
			x = x || this.lastX;
			y = y || this.lastY;
			this.button = 0;
			this.moveObject = null;
			this.moveType = null;
			return Math.sqrt((this.x-this.initialX)*(this.x-this.initialX)+(this.y-this.initialY)*(this.y-this.initialY));
		}
		this.VFromStart = function() { return [this.x-this.initialX, this.y-this.initialY]; }
	};

	 //pop up the overlay
	function BuildOverlay() {
		// | whole page
		// |
		// | | overlay
		// | |
		// | || shield
		// | ||
		// | |||image
		// | ||
		// | || [prev]  [next]
		// | ||
		// | ||
		// | |

		 //create and install the overlay mechanics
		var overlay = document.createElement("div"); //container for everything
		overlay.setAttribute("id", "scrim");
		overlay.className="scrim-overlay";
		overlay.style.position = 'fixed';
		overlay.style.display='none'; //start off
		overlay.style.verticalAlign = "middle";
		overlay.style.left = 0;
		overlay.style.top  = 0;
		overlay.style.width  = docwidth+"px";
		overlay.style.height = docheight+"px";

		//$('<div/>', { 'id':'scrim' }).css('position','absolute').css('display','none').appendTo('#body');

		 //transparent shield, click on it to remove pop up
		var newdiv = $('<div id="above" class="scrim-shield"></div>')
						.click(scrimobj.UnExpand)
						.css({'left':0,
							  'top':0,
							  'width':docwidth+'px',
							  'height':docheight+'px'
							 });
		overlay.appendChild(newdiv[0]);

		 //the image, sits atop overlay
		var width  = 100; //default width/height
		var height = 100;

		var newimg = document.createElement("img");
		newimg.setAttribute("id", "iabove"); 
		newimg.setAttribute("data-which", 0);
		//newimg.setAttribute("src", file); 
		newimg.setAttribute("width",  width );
		newimg.setAttribute("height", height);
		newimg.style.left = (docwidth -width )/2+"px";
		newimg.style.top  = (docheight-height)/2+"px";
		newimg.style.position = 'absolute'; 
		overlay.appendChild(newimg);


		 //nav images, sits over image, which is over overlay
		var navh = 50;
		next = document.createElement("div");
		next.setAttribute("id", "nextimg");
		next.className="scrim-next";
		next.style.left = (docwidth +width)/2 -navh +"px";
		next.style.top  = (docheight-navh)/2+"px";
		next.style.position = 'absolute'; 
		//next.setAttribute("onclick", scrimobj.NextImage);
		overlay.appendChild(next);
		$(next).on("touchstart", TouchStart)
			   .on("touchmove",  TouchMove )
			   .on("touchend",   TouchEnd  )
			   //.click(scrimobj.NextImage)
			   .on("mousedown", LBDownEvent);

		prev = document.createElement("div");
		prev.setAttribute("id", "previmg");
		prev.className="scrim-prev";
		prev.style.left = (docwidth -width )/2+"px";
		prev.style.top  = (docheight-navh)/2+"px";
		prev.style.position = 'absolute'; 
		//prev.setAttribute("onclick", scrimobj.PrevImage);
		overlay.appendChild(prev);
		$(prev).on("touchstart", TouchStart)
			   .on("touchmove",  TouchMove )
			   .on("touchend",   TouchEnd  )
			   //.click(scrimobj.PrevImage)
			   .on("mousedown", LBDownEvent);

		var cancel = $('<div id="scrim-cancel"></div>')
			.appendTo($(overlay))
			.click(function() { scrimobj.UnExpand(); }); 

		document.body.appendChild(overlay);

		var loading_image = $('<img id="scrim-loading" style="display:none" src="'+options.loading+'" />').appendTo($(overlay));

		return false;
	}

	function LBDownEvent(e) {
		if (e.which != 1) return; //only use left button

		var target = $(e.target);
		if (target.hasClass('scrim-firstimg')) target = $("#nextimg");
		LBDown($(e.target), e.pageX, e.pageY, e.which);
		e.preventDefault();

		$(document).mousemove(function(e){
			e.originalEvent.preventDefault();
			if (ButtonDown.button == 0) return;
			
			LBDrag(e.pageX, e.pageY, false);
			e.preventDefault();

		}).mouseup(function(e) {
			 //remove mouseup and mousemove since we are done with them for now
			e.originalEvent.preventDefault();
			$(document).off("mouseup").off("mousemove");

			LBUp(e.pageX,e.pageY);
			e.preventDefault();
		});

		return false; 
	}

	function TouchStart(e) {
		if (istouch == false) istouch = true;
		if (e.touches.length != 1) {
			return;
		}
		e.preventDefault();
		var target = $(e.targetTouches[0].target);
		console.log('touch on ',target);
		LBDown(target, e.touches[0].pageX, e.touches[0].pageY, true);
	}
	function TouchMove(e) {
		if (ButtonDown.moveObject == null) return;
		console.log('touchmove',e);
		LBDrag(e.touches[0].pageX, e.touches[0].pageY);
	}
	function TouchEnd(e) {
		if (ButtonDown.moveObject == null) return;
		console.log('touchend',e);
		 //need to fake the final touch
		LBUp(ButtonDown.x + (ButtonDown.x - ButtonDown.lastX), ButtonDown.y + (ButtonDown.y - ButtonDown.lastY));
	}

	function LBDown(target, x,y,which) {
		console.log('lbdown');
		if (slideImg != null) {
			console.log('existing slideIMg: ',slideImg);
			if (slideImg[0].id != 'iabove') {
				console.log('lbdown remove',slideImg);
				slideImg.remove();
				slideImg = null;
			}
		}
		ButtonDown.down(x,y, which, target);
	}
	var TOUCHTHRESHHOLD = 4;
	var slideDT = 50;
	var slideImg = null;
	function LBDrag(x,y) {
		console.log('lbdrag');
		var dv = ButtonDown.move(x,y);

		var img = $("#iabove");
		slideImg = img;
		img.css('left', parseFloat(img.css('left'))+dv[0]);
        //img.css('top',  parseFloat(img.css('top' ))+dv[1]);
	}
	function LBUp(x,y) {
		console.log('lbup at v:', ButtonDown.v_x, ButtonDown.v_y);

		var next = true;
		if (ButtonDown.moveObject[0].id == 'previmg') next = false;
		//console.log('up with moveObject: ',next,ButtonDown.moveObject[0].id, ButtonDown.moveObject);

		var dist = ButtonDown.up();
		if (dist < TOUCHTHRESHHOLD) {
			//simple click down then up
			slideImg = null;
			if (next) scrimobj.NextImage(); else scrimobj.PrevImage();
			return;
		}

		 //else slide
		slideImg = slideImg.clone();
		slideImg.attr('id', 'scrim-drag')
				.appendTo("#scrim")
				.fadeOut(500)
				.data('scalehack', 1);
		//slideImg.animate({ opacity: 0.0, transform: 'scale(.25)' }, 1000);
		//slideImg.addClass('scrim-fade');
		//slideImg[0].getBoundingClientRect();
		//slideImg.addClass('scrim-fade-small');
		if (ButtonDown.v_x > 10) ButtonDown.v_x = 10;
		else if (ButtonDown.v_x < -10) ButtonDown.v_x = -10;
		setTimeout(VelocitySlide, slideDT); // cleanup after 5 seconds

		if (ButtonDown.v_x < 0) next = true; else next = false;
		if (next) scrimobj.NextImage(); else scrimobj.PrevImage();
	}

	function VelocitySlide() {		
		//console.log('tick',ButtonDown.v_x );

        if (slideImg == null || Math.abs(ButtonDown.v_x*slideDT) <=1) {
			 //done with slide
            ButtonDown.v_x = 0;
			if (slideImg != null) {
				console.log('slide finish, remove maybe',slideImg);
				if (slideImg[0].id != 'iabove') {
					slideImg.remove();
					slideImg = null;
				}
			}
			//slideImgFade.remove();
			//slideImgFade = null;
        } else {
			slideImg.css('left', parseFloat(slideImg.css('left')) + ButtonDown.v_x*slideDT);
			//img.css('top',  parseFloat(img.css('top' ))+dv[1]);
		
			var scale = .8*slideImg.data('scalehack');
			slideImg.css('transform', 'scale('+scale+')').data('scalehack', scale);
			ButtonDown.v_x *= .8;
            setTimeout(VelocitySlide, slideDT); // cleanup after 5 seconds
        }
    }



	this.NextImage = function() {
		//if (isoneoff) { scrimobj.UnExpand(); return; }

		var img = document.getElementById("iabove");
		var i = img.getAttribute("data-which");
		i++;
		if (i >= this.imagelist.length) i=0;
		scrimobj.Expand(document.getElementById("a"+i));

		//next = document.getElementById("nextimg");
		//next.style.opacity=0.;
	}

	this.PrevImage = function() {
		//if (isoneoff) { scrimobj.UnExpand(); return; }

		var img = document.getElementById("iabove");
		var i = img.getAttribute("data-which");
		i--;
		if (i<0) i = this.imagelist.length-1;

		scrimobj.Expand(document.getElementById("a"+i));
	}

	 //pop up the overlay, based on dom anchor
	this.Expand = function(anchor) {  //:Expand
		console.log("expanding... anchor: ", anchor, 'within this:', this);
		//console.log('imagelist: ',typeof(imagelist)=='undefined' ? 'undefined' : imagelist, 'this.imagelist:', typeof(this.imagelist)=='undefined'?'undefined':this.imagelist);
		console.log("anchor: "+anchor.getAttribute("id"));

		if (anchor.getAttribute("data-scrim") == "oneoff") isoneoff = true;
		else isoneoff = false;

		 //we assume aid to have format like "id"="A1"
		var file = anchor.getAttribute("href");
		var aid = anchor.getAttribute("id");
		var id = aid.slice(1); //the image index number
		//console.log("expanding "+file+" anchor:"+id+"  slice:"+id.slice(1));

		if (anchor.getAttribute("data-scrim") == "gonext") {
			id = FindImage(file);
			if (id<0) {
				console.log("Ack! Can't find image for ",anchor);
			}
		}


		var overlay = document.getElementById("scrim");
		if (overlay == null) {
			console.log("missing overlay! uh oh!");
			return;
		}
		overlay.style.display="block";
		overlay.style.left = 0;
		overlay.style.top  = 0;
		overlay.style.width  = docwidth+"px";
		overlay.style.height = docheight+"px";

		var imageinfo = this.imagelist[id];

		console.log('expanding ', imageinfo);

		var width  = imageinfo.w;
		var height = imageinfo.h;
		if ((width <= 0 || height <= 0) && imageinfo.loading_el != null) {
			width  = imageinfo.loading_el.naturalWidth;
			height = imageinfo.loading_el.naturalWidth;
		}
		if (width  == 0) width  = 100;
		if (height == 0) height = 100;
		var aspect = width/height;

		var newimg = $("#iabove");
		newimg.css('display',"block");
		newimg.attr('data-oneoff', isoneoff);


		 //scale up if necessary
		if (width<docwidth*.9 || height<docheight*.9) {
			width  = Math.floor(docwidth*.9);
			height = Math.floor(width/aspect);
		}
		 //scale down if necessary
		if (width>docwidth*.9) {
			width  = Math.floor(docwidth*.9);
			height = Math.floor(width/aspect);
		}
		if (height>docheight*.9) {
			height = Math.floor(docheight*.9);
			width  = Math.floor(height*aspect);
		}

		if (!imageinfo.loaded) Preload(id);

		newimg[0].setAttribute("data-which", id);
		newimg.data('iw', width).data('ih', height);
		if (imageinfo.loaded) {
		  newimg
			.attr("src",  file)
			.attr("width",  width )
			.attr("height", height)
			.css({'left': (docwidth -width )/2+"px",
				  'top':  (docheight-height)/2+"px",
				  'position': 'absolute'});
		} else {
		  newimg
			.attr("src", options.loading)
			.attr("width",  32 )
			.attr("height", 32)
			.css({'left': (docwidth -32 )/2+"px",
				  'top':  (docheight-32)/2+"px",
				  'position': 'absolute'});
		}

		var navh = 50;
		next = document.getElementById("nextimg");
		next.style.width = width/2+"px";
		next.style.height = height+"px";
		next.style.left = (docwidth        )/2+"px";
		next.style.top  = (docheight-height)/2+"px";
		//next.href = file;

		prev = document.getElementById("previmg");
		prev.style.left = (docwidth -width )/2+"px";
		prev.style.top  = (docheight-height)/2+"px";
		prev.style.width = width/2+"px";
		prev.style.height = height+"px";

		preloadNear(id);

		return false;
	}

	function preloadNear(i) {
		var images = (typeof(imagelist) != 'undefined') ? imagelist : scrimobj.imagelist;

		console.log("preload this: ",this);
		console.log('preload near',i, images);

		var next, prev;
		var inext=i, iprev=i;

		inext++;
		if (inext >= images.length) inext=0;
		Preload(inext);

		iprev--;
		if (iprev < 0) iprev = images.length-1;
		Preload(iprev);
	}

	function Preload(i) {
		var image = scrimobj.imagelist[i];

		if (image.loaded) {
			console.log("already preloaded!");
			return;
		}
		if (image.loading_el) {
			console.log("already preloading!");
			return; //already in process of loading
		}

		image.loading_el = $(new Image)
		  .on('load', function(e) {
			image.loaded = true;
			if (image.w == 0) image.w = this.naturalWidth;
			if (image.h == 0) image.h = this.naturalHeight;

			var floating = $("#iabove");

			console.log('preloaded!',i, floating, e, this);
			if (i == floating[0].getAttribute("data-which")) { //we just finished loading the current image, update src
			  //var width = imagelist[i].w, height = imagelist[i].h;
			  var width = floating.data('iw'), height = floating.data('ih');
			  floating.attr('src', image.file)
				.attr("width",  width )
				.attr("height", height)
				.css({'left': (docwidth -width )/2+"px",
					  'top':  (docheight-height)/2+"px",
					  'position': 'absolute'});
			}

		  })
		  .attr('src', image.file);
	}

	 //turn off the overlay
	this.UnExpand = function (element) {
		console.log('UnExpand');
		var div = document.getElementById("scrim");
		div.style.display="none";
	}


	this.LayoutImages = function(reflowing) {
		if (options.use_rows) this.LayoutRows(reflowing);
		else this.LayoutColumns(reflowing); 
	}

	//constant height rows
	this.LayoutRows = function(reflowing) {
		console.log("laying out rows..");

		var thumbs = document.getElementById("thumbs");
		if (!thumbs) return;

		if (reflowing==true) {
			 //window size was changed
			docwidth  = window.innerWidth;
			docheight = window.innerHeight;

			//numcolumns =Math.floor(docwidth/(this.columnwidth+gap));
			//if (numcolumns<1) numcolumns=1;
		}


		var gap = options.gap;
		var x = gap;
		var y = thumbs.offsetTop;
		var cur_row_height = 0;
		//console.log('layout in window size: ',$(window).width());

		var index = 0;
		for (i in this.imagelist) {
			var image = this.imagelist[i];

			//console.log(image);
			var file    = image.file;
			var thumb   = image.thumb;
			var pwidth  = image.pw;
			var pheight = image.ph;
			var width   = pwidth;
			var height  = pheight;

			if (smallscreen) {
				pwidth *= .5;
				pheight *= .5;
			}


			if (x+pwidth > docwidth || (index>0 && index == this.imagelist.length-1)) {
				 //arrange row, and advance to next
				x=gap;
				y+=gap + cur_row_height;
				cur_row_height = 0;
			}

			if (pheight > cur_row_height) cur_row_height = pheight;

			var newimg;
			if (reflowing==true) {
				 // a > img exists already
				newimg = document.getElementById(i); 
				newimg.style.position = 'static';
				//newimg.style.position = 'absolute';

			} else {
				 //new, need to create and add <a><img></a>
				newimg = document.createElement("img");
				newimg.setAttribute("id", i);
				newimg.setAttribute("src", thumb);

				newimg.style.border = (gap/2)+"px solid #000000";
				newimg.style.position = 'static';
				//newimg.style.position = 'absolute';

				newa = document.createElement("a");
				newa.setAttribute("id", "a"+i);
				newa.setAttribute("href", file);
				$(newa).click(ExpandEvent);


				newa.appendChild(newimg);
			}

			newimg.style.left = x.toString()+"px";
			newimg.style.top  = y.toString()+"px";
			newimg.setAttribute("width",  pwidth);
			newimg.setAttribute("height", pheight);
			newimg.style.verticalAlign = "middle";

			x += pwidth + gap;

			if (reflowing==false) thumbs.appendChild(newa);

			index++;
		}

	}

	 //Search in imagelist for src.
	 //Returns index, or -1.
	function FindImage(src) {
		var ii;
		for (i in scrimobj.imagelist) {
			//ii = imagelist[i].file.indexOf(src);
			//if (ii >= 0) return i;
			//------------
			if (src == scrimobj.imagelist[i].file) return i;
		}
		return -1;
	}

	 //on jquery a > img, set expand function, id. Assume href and src already set correctly.
	function SetFunctions(a, index) {
		a.attr("id", "a"+index);
		a.click(ExpandEvent);
	}

	function ExpandEvent(event) {
		//console.log('ExpandEvent: ',event);
		console.log('ExpandEvent this: ',this);
		scrimobj.Expand(event.currentTarget); 
		event.preventDefault();
		return false;
	}

	var Column = function(xx,ww) {
		this.x     = xx;
		this.width = ww;
		this.y     = 50; //next y value, we'll always choose the smallest y for the next image
	};

	//tumblr like constant width columns
	this.LayoutColumns = function(reflowing) {

		console.log("laying out columns..");
		console.log("columnwidth: "+options.columnwidth);

//		if (typeof(this.imagelist) == "undefined") {
//			console.log("Couldn't lay out yet!");
//			return;
//		}

		var thumbs = document.getElementById("thumbs");
		if (!thumbs) return;


		var gap = options.gap;

		if (reflowing == true) {
			 //window size was changed
			docwidth  = window.innerWidth;
			docheight = window.innerHeight;

			numcolumns =Math.floor(docwidth/(options.columnwidth+gap));
			if (numcolumns<1) numcolumns=1;
		}

		var columns = new Array(numcolumns);
		var initialgap = (docwidth - (numcolumns*(options.columnwidth+gap)-gap))/2
		var x = initialgap;
		var maxy = thumbs.offsetTop;

		for (c=0; c<numcolumns; c++) {
			columns[c]=new Column(x, options.columnwidth);
			x+=options.columnwidth + gap;
			columns[c].y = thumbs.offsetTop;
		}

		var nextcolumn=0;
		for (c=0; c<numcolumns; c++) {
			if (columns[c].y<columns[nextcolumn].y) nextcolumn=c;
		}

		console.log("columnwidth2: "+options.columnwidth);

		for (i in this.imagelist) {
			var image = this.imagelist[i];

			//console.log(image);
			var file    = image.file;
			var thumb   = image.thumb;
			var pwidth  = image.pw;
			var pheight = image.ph;
			var width   = pwidth;
			var height  = pheight;

			pwidth = options.columnwidth;
			pheight = Math.floor(options.columnwidth / width * height);
			if (pheight > options.columnwidth*2.5) {
				pheight = options.columnwidth*2.5;
				pwidth = Math.floor(pheight / height * width);
			}

			py = columns[nextcolumn].y;
			px = columns[nextcolumn].x;
			if (pwidth!=options.columnwidth) {
				 //image was too horizontally thin, so center in column
				px+=(options.columnwidth-pwidth)/2;
			}

			//console.log("f: "+image.file+" w:"+image.w+" h:"+image.h);

			var newimg;
			if (reflowing==true) {
				newimg = document.getElementById(i); 
				newimg.style.position = 'absolute';
			} else {
				newimg = document.createElement("img");
				newimg.setAttribute("id", i);
				newimg.setAttribute("src", thumb);
				newimg.style.position = 'absolute';

				newa = document.createElement("a");
				newa.setAttribute("id", "a"+i);
				newa.setAttribute("href", file);
				newa.appendChild(newimg);
				$(newa).click(ExpandEvent);
			}
			newimg.style.left = px.toString()+"px";
			newimg.style.top  = py.toString()+"px";
			newimg.setAttribute("width",  pwidth);
			newimg.setAttribute("height", pheight);
			//if (reflowing==false) thumbs.appendChild(newimg);
			if (reflowing==false) thumbs.appendChild(newa);

			columns[nextcolumn].y += pheight+gap;
			if (columns[nextcolumn].y > maxy) maxy = columns[nextcolumn].y;

			 //now update which column gets the next image
			for (c=0; c<numcolumns; c++) {
				if (columns[c].y<columns[nextcolumn].y) nextcolumn=c;
			}
		} //for i in imagelist

		thumbs.style.height = (maxy-thumbs.offsetTop)+"px";

	};

	this.ReFlow = function () {
		console.log("reflowing... width:"+window.innerWidth);

		docwidth  = window.innerWidth;
		docheight = window.innerHeight;
		smallscreen = ($(window).width() < 1025);
		console.log('reflow screen size:',window.innerWidth,window.innerHeight);

		overlay = document.getElementById("scrim");
		if (overlay == null) return; //sometimes this happens on mobile futzing
		overlay.style.width  = docwidth+"px";
		overlay.style.height = docheight+"px";

		overlay=document.getElementById("above");
		overlay.style.width  = docwidth+"px";
		overlay.style.height = docheight+"px";

		var overlay = document.getElementById("scrim");
		if (overlay.style.display == "block") {
			//resize the popped up image...
			var img = document.getElementById("iabove");
			var i = img.getAttribute("data-which");
			scrimobj.Expand(document.getElementById("a"+i));
		}

		UpdateCoverImage();
		this.LayoutImages(true);
	}

	 //change dimensions of the cover image to fit within current window
	function UpdateCoverImage() {
		var img = $(".scrim-firstimg");
		if (!img || img.length == 0) return;
		img = img.children(0);
		if (!img) return;
		console.log('update cover image',img);

		if (img[0].naturalWidth <= 0) return;
		var dims = Fit(img[0].naturalWidth,img[0].naturalHeight, .95*docwidth,.95*docheight);
		img.css({ width: dims[0]+'px', height: dims[1]+'px' });
	}

	function Fit(oldw,oldh, inw,inh) {
		if (oldw < inw && oldh < inh) return [oldw,oldh];
		//if (oldw >= inw && oldh < inh) return [inw,oldh * inw/oldw];
		//if (oldw < inw && oldh >= inh) return [oldw * inh/oldh, inh];
		
		var scale = inw/oldw;
        var yscale= inh/oldh;
        if (yscale<scale) scale=yscale;

		return [oldw*scale, oldh*scale];
	}

	this.Add = function(imagething) {
		this.imagelist.push(imagething);
	}

	 //this is called automatically on document.ready
	function init(o) {
		console.log("Initing Scrim.. o:", o, 'this: ',this);


		//-----------------main----------------

		if (!options.auto) {
			//load in json from options.jsonfile, then init rest
			console.log('read in json: ',options.jsonfile);
			$.getJSON(options.jsonfile, function(data) {
					console.log('data:',data);
					o.imagelist = data.images;
					console.log("json read in done!!");

					BuildOverlay();

					//imagelist[] = [ { img_element, loaded, src }, ...]
					var img;
					for (i in o.imagelist) {
						img = o.imagelist[i];
						img.loaded = false;
						img.loading_el = null;
					}

					o.LayoutImages(false); 

					console.log("json load this: ", this);
					preloadNear(0);

					var hashmatch = window.location.hash.match(/\#(\d+)/);
					console.log('hash: ', hashmatch);
					if (hashmatch != null) {
						hashmatch = hashmatch[1]-1;
						var a = $("#a"+hashmatch);
						console.log('should really start open to image #',hashmatch,a);
						if (a.length > 0) scrimobj.Expand(a[0]);
					}
				}).fail(function() {
					console.log("Warning json load fail!");
				});

		} else {
			//build manually from any anchor tagged with scrim-data="img"
			var images = $("[data-scrim=img]");
			if (images.length != 0) {
				console.log("using existing...", images);

				 //give ids sequentially a0, a1, ...
				 //build imagelist
				for (var i=0; i < images.length; i++) {
					var anchor = $(images[i]);
					console.log(anchor);

					scrimobj.Add({
								file: anchor.attr("href"),
								w: 0,
								h: 0,
								thumb: anchor.children(0).attr("src"),
								pw: 0,
								ph: 0,
								loaded: false,
								loading_el: null
							});
					anchor.attr("id", "a"+i)
						  //.click(ExpandEvent)
						;
				}

				console.log("manual build this: ", this);
				BuildOverlay();
				//LayoutImages(false); //assume images are manually laid out
				preloadNear(0);
				Preload(0);

			} else {
				console.log("Warning, no scrim images found!");
			}
		}


		window.addEventListener("resize", function() { scrimobj.ReFlow(); } );

		UpdateCoverImage();

		 //set up key controls
		$(document).keyup(function(e) {
			//console.log('key event: ',e);

			if (e.shiftKey || e.ctrlKey || e.altKey) return;

			if (e.keyCode == 27) { // escape key maps to keycode `27`
				 //escape key to get out of overlay
				scrimobj.UnExpand(null);

			} else if (e.keyCode == 37) { //left
				scrimobj.PrevImage();

			} else if (e.keyCode == 39) { //right
				scrimobj.NextImage();

			//} else if (e.keyCode == 38) { //up
			//} else if (e.keyCode == 40) { //down
			}

		});


	}; //init()

	var scrimobj = this;

	$(document).ready(function() {
		var manuals = $("[data-scrim=img]");
		console.log("manuals: ",manuals);
		if (manuals.length != 0) {
		  //we have some manually placed images, we need to set up images properly
		  manuals.click(ExpandEvent);
		}

		$("[data-scrim=gonext]")//.click(ExpandEvent);
				// *** only works if gonext on first image.. need to find image in list
				.on("click", function(e) {
					e.preventDefault();
					console.log('first click (do nothing)');
					return false;
				  })
				.on("touchstart", function(e) {
					e.preventDefault();
					console.log('first touchstart'); 
				  })
				.on("touchmove",  function(e) {
					e.preventDefault();
					console.log('first touchmove'); 
				  })
				.on("touchend", function(e) {
					console.log('first touch up'); 
					//if (e.touches.length != 1) return;
					scrimobj.Expand($("#A1")[0]);
					e.preventDefault();
				  })
				.on("mousedown", function(e) {
					if (e.which != 1) return; //only use left button
					e.preventDefault();
					$(document)
					  .on("mousemove", function(e){
						  e.preventDefault();
					    })
					  .on("mouseup", function(e) {
						e.preventDefault();
						console.log('first up',e);
						scrimobj.Expand($("#A1")[0]);
						$(document).off("mouseup").off("mousemove");
					  })
				  })
				;

		init(scrimobj);

	});

}; //Scrim





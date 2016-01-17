var contentOpacity = 20;                    // content opacity during loading (0 - 100)*
var safari = (navigator.userAgent.indexOf('Safari') != -1) ? true : false;
safari = false;

function LOADER() {
    this.iv;
    this.imgAll = 0;
    this.opacity = contentOpacity;
    this.opName = '';
    
    this.getObj = function(id) {
	var obj;
	if(document.getElementById) obj = document.getElementById(id);
	else if(document.all)       obj = document.all[id];
	return obj;
    }
    
    this.getOpName = function(obj) {
	if(obj) {
            if(typeof(obj.style.opacity)      != 'undefined') return 'opacity';
            if(typeof(obj.style.MozOpacity)   != 'undefined') return 'MozOpacity';
            if(typeof(obj.style.KhtmlOpacity) != 'undefined') return 'KhtmlOpacity';
            if(typeof(obj.style.filter)       != 'undefined') return 'filter';
	}
	return false;
    }
    
    this.setOpacity = function(obj, opacity) {
	if(obj && !document.layers) {
            if(this.opName == 'filter') obj.style.filter = 'alpha(opacity=' + opacity + ')';
            else if(this.opName)        obj.style[this.opName] = opacity / 100;
	}
    }
    
    this.fadeIn = function(id) {
	var obj = this.getObj(id);
	if(obj) {
            if(document.all) obj.style.position = 'absolute';
            obj.style.visibility = 'visible';
	    this.opacity = 100;
	    this.setOpacity(obj, 100);
	}
    }
    
    this.init = function() {
	var content = this.getObj('Content');
	this.opName = this.getOpName(content);
	this.setOpacity(content, contentOpacity);
	if(this.opName) content.style.visibility = 'visible'; 
	this.loaded();
    }
    
    this.loaded = function() {
	window.status = '';
	this.fadeIn('Content');
	//var cover = this.getObj('divBox');
	//cover.style.visibility = 'hidden';
	this.getObj('divBox').style.visibility = 'hidden';
    }
}
    
    if((document.all || document.getElementById) && !safari) {
	document.write('</div>');
	var loader = new LOADER();
	loader.init();
    }

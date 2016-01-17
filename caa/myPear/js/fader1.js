var contentOpacity = 20;                    // content opacity during loading (0 - 100)*

var safari = (navigator.userAgent.indexOf('Safari') != -1) ? true : false;
safari = false;

if((document.all || document.getElementById) && !safari) {
    document.write('<div id="divBox" class="clsBox"><img src="classes/img/fader.gif"></div>' +
		   '<div id="Content" style="width:100%; visibility:hidden">');
}

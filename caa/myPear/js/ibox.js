var indicator_img_path = "/images/loading.gif";
var indicator_img_html = '<img name="ibox_indicator" src="' + indicator_img_path + '" alt="Loading..." style="width:32px;height:32px;"/>';
var opacity_level = 7;
var ibAttr = "rel";
var imgPreloader = new Image();

function init_ibox() {
    var f = "ibox";
    createIbox(document.getElementsByTagName("body")[0]);
    var c = document.getElementsByTagName("a");
    var d;
    for (var b = 0; b < c.length - 1; b++) {
        d = c[b];
        if (d.getAttribute(ibAttr)) {
            var a = d.getAttribute(ibAttr);
            if ((a.indexOf("ibox") != -1) || a.toLowerCase() == "ibox") {
                d.onclick = function () {
                    var g = this.getAttribute(ibAttr);
                    var i = parseQuery(g.substr(5, 999));
                    var e = this.href;
                    if (this.target != "") {
                        e = this.target
                    }
                    var h = this.title;
                    if (showIbox(e, h, i)) {
                        showBG();
                        window.onscroll = maintPos;
                        window.onresize = maintPos
                    }
                    return false
                }
            }
        }
    }
} showBG = function () {
    var e = getElem("ibox_w");
    e.style.opacity = 0;
    e.style.filter = "alpha(opacity=0)";
    setBGOpacity = setOpacity;
    for (var c = 0; c <= opacity_level; c++) {
        setTimeout("setIboxOpacity('ibox_w'," + c + ")", 70 * c)
    } e.style.display = "";
    var b = new getPageSize();
    var d = new getScrollPos();
    var a = navigator.userAgent;
    if (a.indexOf("MSIE ") != -1) {
        e.style.width = b.width + "px"
    } e.style.height = b.height + d.scrollY + "px"
};
hideBG = function () {
    var a = getElem("ibox_w");
    a.style.display = "none"
};
var loadCancelled = false;
showIndicator = function () {
    var a = getElem("ibox_progress");
    a.style.display = "";
    posToCenter(a);
    a.onclick = function () {
        hideIbox();
        hideIndicator();
        loadCancelled = true
    }
};
hideIndicator = function () {
    var a = getElem("ibox_progress");
    a.style.display = "none";
    a.onclick = null
};
createIbox = function (c) {
    var a = '<div id="ibox_w" style="display:none;"></div>';
    a += '<div id="ibox_progress" style="display:none;">';
    a += indicator_img_html;
    a += "</div>";
    a += '<div id="ibox_wrapper" style="display:none">';
    a += '<div id="ibox_content"></div>';
    a += '<div id="ibox_footer_wrapper"><div id="ibox_close" style="float:right;">';
    a += '<a id="ibox_close_a" href="javascript:void(null);" >[X]</a></div>';
    a += '<div id="ibox_footer">&nbsp;</div></div></div></div>';
    var b = document.getElementsByTagName("body")[0];
    var d = document.createElement("div");
    d.setAttribute("id", "ibox");
    d.style.display = "";
    d.innerHTML = a;
    c.appendChild(d)
};
var ibox_w_height = 0;
showIbox = function (b, l, e) {
    var a = getElem("ibox_wrapper");
    var d = 0;
    var h = getElem("ibox_footer");
    if (l != "") {
        h.innerHTML = l
    } else {
        h.innerHTML = "&nbsp;"
    }
    var f = /\/\?|\.jpg|\.jpeg|\.png|\.gif|\.html|\.htm|\.php|\.cfm|\.asp|\.aspx|\.jsp|\.jst|\.rb|\.rhtml|\.txt/g;
    var n = b.match(f);
    if (n == ".jpg" || n == ".jpeg" || n == ".png" || n == ".gif") {
        d = 1
    } else {
        if (b.indexOf("#") != -1) {
            d = 2
        } else {
            if (n == "/?" || ".htm" || n == ".html" || n == ".php" || n == ".asp" || n == ".aspx" || n == ".jsp" || n == ".jst" || n == ".rb" || n == ".txt" || n == ".rhtml" || n == ".cfm") {
                d = 3
            } else {
                if (e.type) {
                    d = parseInt(e.type)
                } else {
                    hideIbox();
                    return false
                }
            }
        }
    } d = parseInt(d);
    switch (d) {
    case 1:
        showIndicator();
        imgPreloader = new Image();
        imgPreloader.onload = function () {
            imgPreloader = resizeImageToScreen(imgPreloader);
            hideIndicator();
	    var i = '<img name="ibox_img" src="' + b + '" style="width:' + imgPreloader.width + "px;height:" + imgPreloader.height + 'px;border:0;cursor:hand;margin:0;padding:0;"/>';
            if (loadCancelled == false) {
                a.style.height = imgPreloader.height + 6 + "px";
                a.style.width = imgPreloader.width + "px";
                a.style.display = "";
                a.style.visibility = "hidden";
                posToCenter(a);
                a.style.visibility = "visible";
                setIBoxContent(i)
            }
        };
        loadCancelled = false;
        imgPreloader.src = b;
        break;
    case 2:
        var j = "";
        if (e.height) {
            a.style.height = e.height + "px"
        } else {
            a.style.height = "350px"
        }
        if (e.width) {
            a.style.width = e.width + "px"
        } else {
            a.style.width = "420px"
        } a.style.display = "";
        a.style.visibility = "hidden";
        posToCenter(a);
        a.style.visibility = "visible";
        getElem("ibox_content").style.overflow = "auto";
        var c = b.substr(b.indexOf("#") + 1, 1000);
        var m = getElem(c);
        if (m) {
            j = m.innerHTML
        } setIBoxContent(j);
        break;
    case 3:
        showIndicator();
        http.open("get", b, true);
	http.onreadystatechange = function () {
            if (http.readyState == 4) {
                hideIndicator();
                if (e.height) {
                    a.style.height = e.height + "px"
                } else {
                    a.style.height = "300px"
                }
                if (e.width) {
                    a.style.width = e.width + "px"
                } else {
                    a.style.width = "800px"
                } a.style.display = "";
                a.style.visibility = "hidden";
                posToCenter(a);
                a.style.visibility = "visible";
                getElem("ibox_content").style.overflow = "auto";
                var i = http.responseText;
                setIBoxContent(i)
            }
        };
        http.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
        http.send(null);
        break;
    default:
    } a.style.opacity = 0;
    a.style.filter = "alpha(opacity=0)";
    var k = 10;
    setIboxOpacity = setOpacity;
    for (var g = 0; g <= k; g++) {
        setTimeout("setIboxOpacity('ibox_wrapper'," + g + ")", 30 * g)
    }
    if (d == 2 || d == 3) {
        a.onclick = null;
        getElem("ibox_close_a").onclick = function () {
            hideIbox()
        }
    } else {
        a.onclick = hideIbox;
        getElem("ibox_close_a").onclick = null
    }
    return true
};
setOpacity = function (b, a) {
    var c = getElem(b);
    c.style.opacity = a / 10;
    c.style.filter = "alpha(opacity=" + a * 10 + ")"
};
resizeImageToScreen = function (c) {
    var b = new getPageSize();
    var a = b.width - 100;
    var d = b.height - 100;
    if (c.width > a) {
        c.height = c.height * (a / c.width);
        c.width = a;
        if (c.height > d) {
            c.width = c.width * (d / c.height);
            c.height = d
        }
    } else {
        if (c.height > d) {
            c.width = c.width * (d / c.height);
            c.height = d;
            if (c.width > a) {
                c.height = c.height * (a / c.width);
                c.width = a
            }
        }
    }
    return c
};
maintPos = function () {
    var e = getElem("ibox_wrapper");
    var d = getElem("ibox_w");
    var b = new getPageSize();
    var c = new getScrollPos();
    var a = navigator.userAgent;
    if (a.indexOf("MSIE ") != -1) {
        d.style.width = b.width + "px"
    }
    if (a.indexOf("Opera/9") != -1) {
        d.style.height = document.body.scrollHeight + "px"
    } else {
        d.style.height = b.height + c.scrollY + "px"
    } posToCenter(e)
};
hideIbox = function () {
    hideBG();
    var a = getElem("ibox_wrapper");
    a.style.display = "none";
    clearIboxContent();
    window.onscroll = null
};
posToCenter = function (d) {
    var f = new getScrollPos();
    var b = new getPageSize();
    var c = new getElementSize(d);
    var a = Math.round(b.width / 2) - (c.width / 2) + f.scrollX;
    var e = Math.round(b.height / 2) - (c.height / 2) + f.scrollY;
    d.style.left = a + "px";
    d.style.top = e + "px"
};
getScrollPos = function () {
    var a = document.documentElement;
    this.scrollX = self.pageXOffset || (a && a.scrollLeft) || document.body.scrollLeft;
    this.scrollY = self.pageYOffset || (a && a.scrollTop) || document.body.scrollTop
};
getPageSize = function () {
    var a = document.documentElement;
    this.width = self.innerWidth || (a && a.clientWidth) || document.body.clientWidth;
    this.height = self.innerHeight || (a && a.clientHeight) || document.body.clientHeight
};
getElementSize = function (a) {
    this.width = a.offsetWidth || a.style.pixelWidth;
    this.height = a.offsetHeight || a.style.pixelHeight
};
setIBoxContent = function (b) {
    clearIboxContent();
    var a = getElem("ibox_content");
    a.style.overflow = "auto";
    a.innerHTML = b
};
clearIboxContent = function () {
    var a = getElem("ibox_content");
    a.innerHTML = ""
};
getElem = function (a) {
    return document.getElementById(a)
};
parseQuery = function (d) {
    var e = new Object();
    if (!d) {
        return e
    }
    var a = d.split(/[;&]/);
    for (var c = 0; c < a.length; c++) {
        var g = a[c].split("=");
        if (!g || g.length != 2) {
            continue
        }
        var b = unescape(g[0]);
        var f = unescape(g[1]);
        f = f.replace(/\+/g, " ");
        e[b] = f
    }
    return e
};
createRequestObject = function () {
    var xmlhttp;
/*@cc_on
    @if (@_jscript_version>= 5)
    try {
        xmlhttp = new ActiveXObject("Msxml2.XMLHTTP");
    } catch (e) {
        try { xmlhttp = new ActiveXObject("Microsoft.XMLHTTP"); }
        catch (E) { xmlhttp = false; }
    }
    @else
        xmlhttp = false;
    @end
    @*/
    if (!xmlhttp && typeof XMLHttpRequest != "undefined") {
        try {
            xmlhttp = new XMLHttpRequest()
        } catch (e) {
            xmlhttp = false
        }
    }
    return xmlhttp
};
var http = createRequestObject();

function addEvent(d, c, a) {
    if (d.addEventListener) {
        d.addEventListener(c, a, false);
        return true
    } else {
        if (d.attachEvent) {
            var b = d.attachEvent("on" + c, a);
            return b
        } else {
            return false
        }
    }
} addEvent(window, "load", init_ibox);;

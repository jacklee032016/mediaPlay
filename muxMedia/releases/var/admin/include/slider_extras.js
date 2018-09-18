/*
 * Copyright (c) 2006, www.never-online.net! All rights reserved.
 * web    : http://www.never-online
 * author : never-online, BlueDestiny
 * version: 0.10.1 
 * this is a autocomplete extras version, complete version in neverModules framework.
 * debug in IE6.0, Opera9.0, Mozilla Firefox1.5.0
 */ 

if (typeof getElementById!="function") {
  var getElementById = function (id) {
    if   (typeof(id)=="object") return id;
    if   (document.getElementById(id)) { return document.getElementById(id); } 
    else { throw new Error(id +" argument error, can not find \"" +id+ "\" element"); }
  }
}

function getElCoordinate (e) {
  var t = e.offsetTop;
  var l = e.offsetLeft;
  var w = e.offsetWidth;
  var h = e.offsetHeight;
  while (e=e.offsetParent) {
    t += e.offsetTop;
    l += e.offsetLeft;
  }; return {
    top: t,
    left: l,
    width: w,
    height: h,
    bottom: t+h,
    right: l+w
  }
}

var neverModules     = window.neverModules  || {};
neverModules.modules = neverModules.modules || {}; 

neverModules.modules.slider = function (cfg) {
  if ((typeof cfg)!="object") 
  throw new Error("config argument is not a object, error raise from slider constructor");

  this.targetId  = cfg.targetId;
  this.hints     = cfg.hints?cfg.hints:"";
  this.sliderCss = cfg.sliderCss?cfg.sliderCss:"";
  this.barCss    = cfg.barCss?cfg.barCss:"";
  this.min       = cfg.min?cfg.min:0;
  this.max       = cfg.max?cfg.max:100;
  this.onstart   = function(){};
  this.onchange  = function(){};
  this.onmouseup = function(){};
  this.onend     = function(){};

  this._defaultInitializer.apply(this);
}

neverModules.modules.slider.prototype = {
  _defaultInitializer: function () {
    this._bar     = null;
    this._slider  = null;
    this._wrapper = null;
    this._target  = getElementById(this.targetId);
    if (this.min>this.max){var x=this.min;this.min=this.max;this.max=x;}
    this._value   = this.min;
  },

  create: function () {
    this._createSlider();
  },

  dispose: function () {
    //virtual function
  },

  createBar: function () { with(this) {
    //0.10 can not create mutilple bar
    //this interface is for next version
    //by never-online
    var _self = this;
    _bar = document.createElement("DIV");
    _wrapper.appendChild(_bar);
    _bar.title = hints;
    _bar.id = targetId + "_bar";
    _bar.className = barCss;
    _bar.style.position  = "absolute";
    _bar.onmousedown = function (evt) { _self._initMoveSlider(evt); }
  }},

  setValue: function (n) { with(this) {
    if (!_bar) return; n = _Number(Number(n)); n = n>max?max:n<min?min:n;
    _bar.style.left = Math.round((n-min)*((_slider.offsetWidth-_bar.offsetWidth)/(max-min)))+"px";
    _value = n; fireChange(); fireEnd();
  }},

  getValue: function () {
    return this._value;
  },

  fireStart: function () {
    this.onstart.call(this);
  },

  fireChange: function () {
    this.onchange.call(this);
  },

 fireMouseup: function () {
	
    this.onmouseup.call(this);
    
  },

  fireEnd: function () {
    this.onend.call(this);
  },

  _createSlider: function () { with(this) {
    _wrapper = document.createElement("DIV");
    _target.appendChild(_wrapper);
    _wrapper.id = targetId + "_wrapper";
    _wrapper.style.position = "relative";

    _slider = document.createElement("DIV");
    _wrapper.appendChild(_slider);
    _slider.id = targetId + "_slider";
    _slider.className = sliderCss;
    _slider.style.position  = "absolute";

    createBar(); var _self = this;
    _slider.onclick = function (evt) { _self._moveTo(evt); }
  }},

  _moveTo: function (evt) { with(this) {
    evt = evt?evt:window.event; 
    var x = evt.clientX-getElCoordinate(_slider).left-Math.round(_bar.offsetWidth/2); x = _coordsX(x);
    _bar.style.left = x+"px"; _value = Math.round(x/((_slider.offsetWidth-_bar.offsetWidth)/(max-min))+min);
    fireChange(); fireEnd();
  }},

  _coordsX: function (x) { with(this) {
    x = _Number(x);
    x = x<=_slider.offsetLeft?_slider.offsetLeft:x>=_slider.offsetLeft+_slider.offsetWidth-_bar.offsetWidth?_slider.offsetLeft+_slider.offsetWidth-_bar.offsetWidth:x;
    return x;  
  }},
  
  _coordsY: function (y) { with(this) {

  }},
  
  _Number: function (n) {
    return isNaN(n)?0:n;
  },

  _initMoveSlider: function (evt) { with(this) {
    evt  = evt?evt:window.event; var _self = this;
    _bar.slider_x = evt.clientX-_bar.offsetLeft;
    fireStart();
    document.onmousemove = function (evt) { _self._changeHandle(evt); }
    document.onmouseup   = function (evt) { _self._endHandle(evt);    fireMouseup(); }
  }},

  _changeHandle: function (evt) { with(this) {
    evt = evt?evt:window.event; var x = evt.clientX-_bar.slider_x;
    x = _coordsX(x);
    _bar.style.left = x+"px"; _value = Math.round(x/((_slider.offsetWidth-_bar.offsetWidth)/(max-min))+min);
    fireChange();
  }},

  _endHandle: function (evt) { with(this) {
    //Release event
    document.onmousemove = null;
    document.onmouseup   = null;
    fireEnd();
  }}
}














//////////////////////////////////////////////////////

var run_flag = false;
var intervalID = null;
var vmode =    "mpeg";    
var PanTilt =  "off";            
var FrameRate = "30";

var sizetimer = 2000; // milliseconds
var vsize; 
var sizeIndex; 
var netcamObj;

var resolutionA = [160, 320, 640];
var heightA     = [154, 240, 480];
var widthA      = [306, 466, 786];
var borderA     = [120, 120,  40]; // total (top + bottom)


function init()
{
	netcamObj = document.PlayCtl;
	netcamObj.width = 320; 
	netcamObj.height =240; 
	var t=window.location.host;
	
	netcamObj.MyMethod(t,22050,1,':554/rtsp_feed.avi','sky','sky');	
	//netcamObj.MyMethod('172.21.2.225',22050,1,':554/rtsp_feed.avi','sky','sky');

	
}


//Image Control
function toggleButtonOver_if(original,over)
{
	if (document.getElementById("if").src.indexOf(original.replace('.','')) > -1)
	{
		document.getElementById("if").src = over;
	}
	else
	{
		document.getElementById("if").src = original;
	}
}

function toggleButtonOver_pad(original,over)
{
	if (document.getElementById("pad").src.indexOf(original.replace('.','')) > -1)
	{
		document.getElementById("pad").src = over;
	}
	else
	{
		document.getElementById("pad").src = original;
	}
}

function toggleButtonOver_patrol(original,over)
{
	if (document.getElementById("patrol").src.indexOf(original.replace('.','')) > -1)
	{
		document.getElementById("patrol").src = over;
	}
	else
	{
		document.getElementById("patrol").src = original;
	}
}

// Zoom

var zoom_flag = false;

function press_zoom()
{
	netcamObj = document.NetCamPlayerWeb11gv2;
	zoom_flag = !zoom_flag;
	
	VideoZoom(zoom_flag);

	if (true == zoom_flag)
	{
		document.images["zoom"].src = "zoom_up.gif";
	}
	else
	{
		document.images["zoom"].src = "zoom_over.gif";
	}
}
function VideoZoom(boolflag)
{
	netcamObj = document.NetCamPlayerWeb11gv2;
	netcamObj.VideoZoom();
}

// Snap
function press_snap()
{
	netcamObj = document.NetCamPlayerWeb11gv2;
	netcamObj.SnapShot();
}
function SnapShot(boolflag)
{
	netcamObj = document.NetCamPlayerWeb11gv2;
	netcamObj.SnapShot();
}

// Direct P/T

var direct_pt_flag = false;

function press_dir()
{
	netcamObj = document.NetCamPlayerWeb11gv2;
	direct_pt_flag = !direct_pt_flag;
	DirectPT(direct_pt_flag);
	if (true == direct_pt_flag)
	{
		document.images["dir"].src = "./buttons/dir_down.gif";
	}
	else
	{
		document.images["dir"].src = "./buttons/dir_over.gif";
	}
}
function DirectPT(boolflag)
{
	netcamObj = document.NetCamPlayerWeb11gv2;
	if(boolflag) {
		netcamObj.DirectPT(1);
	}
	else {
		netcamObj.DirectPT(0);
	}
}

// Flip
var flip_flag = false;

function press_flip()
{
	netcamObj = document.PlayCtl;
	flip_flag = !flip_flag;
	VideoFlip(flip_flag);
	if (true == flip_flag)
	{
		document.images["flip"].src = "/flip_over.gif";
	}
	else
	{
		document.images["flip"].src = "/flip_up.gif";
	}
}
function VideoFlip(boolflag)
{
	netcamObj = document.PlayCtl;
	netcamObj.VideoFlip();
}

// Mirror
var mir_flag = false;
function press_mir()
{
	netcamObj = document.PlayCtl;
	mir_flag = !mir_flag;
	VideoMirror(mir_flag);
   
	if (true == mir_flag)
	{
		document.images["mir"].src = "/mir_over.gif";
	}
	else
	{
		document.images["mir"].src = "/mir_up.gif";
	}
}
function VideoMirror(boolflag)
{
	netcamObj = document.PlayCtl;
	netcamObj.VideoMirror();
}

var nVal = 9; // default
var tmp = 0;




// Sound Control
var mute_flag = false;
function pressMuteButton() {
	netcamObj = document.PlayCtl;
	mute_flag = !mute_flag;	
	MuteVolume(mute_flag);

	var el = document.getElementById("sliderDemo1");
	var ell = document.getElementById("sliderDetail1");
	if (true == mute_flag)
	{
		document.images["sound"].src = "/muteoff_over.gif";
		el.style.display = "none";
		ell.style.display = "none";

	}
	else
	{
		document.images["sound"].src = "/muteon_over.gif";
		el.style.display = "block";
		ell.style.display = "block";
		
	}

}

function MuteVolume(boolflag)
{
	netcamObj = document.PlayCtl;	
	if(mute_flag)
		netcamObj.StopPlayAudio();
	else
		netcamObj.SetPlayAudio();

}




// Move Camera



var is_auto_reload = true;
function reloadOnErr(obj) {
	if(!is_auto_reload)
		return;

	setTimeout("eval('obj.src = obj.src;'", 500);
}




//ocx control
function SetZoomPercent() 
{	
	netcamObj.SetZoomPercent(); 
}


function setWidth(obj,theWidth)
{
  document.getElementById(obj).style.width=Number(theWidth*10);
}


function setOCXSize(res) {

	if(res == 640) {
		netcamObj.width = 640; 
		netcamObj.height = 480; 
		netcamObj.SetVIewSize(640,480); 
	}
	else if(res == 320) {
		netcamObj.width = 320; 
		netcamObj.height = 240; 
		netcamObj.SetVIewSize(320,240);
	}
	else if(res == 160) {
		netcamObj.width = 160; 
		netcamObj.height = 128; 
	netcamObj.SetVIewSize(160,128);
	}
}


function URL_request(type,tempvalue){
	
		var t=window.location.host;
		var URL="http://"+t+"/cgi-bin/activeCtrl?"+type+"&"+tempvalue;
		//self.opener.location.reload(URL);
		document.getElementById("hiddenframe").src = URL;
}

function setViewmode(index){
	var URL;
	var t=window.location.host;
	if(index == 0) {
		URL="http://"+t+"/cgi-bin/activeCtrl?modeindex&0";
		//self.opener.location.reload(URL);
		document.getElementById("hiddenframe").src = URL;
	}
	else if(index == 1) {
		URL="http://"+t+"/cgi-bin/activeCtrl?modeindex&1";
		//self.opener.location.reload(URL);
		document.getElementById("hiddenframe").src = URL;
	}
	else if(index == 2) {
		URL="http://"+t+"/cgi-bin/activeCtrl?modeindex&2";
		//self.opener.location.reload(URL);
		document.getElementById("hiddenframe").src = URL;
	}
	else if(index == 3) {
		URL="http://"+t+"/cgi-bin/activeCtrl?modeindex&3";
		//self.opener.location.reload(URL);
		document.getElementById("hiddenframe").src = URL;
	}

	
}
function volume_set(vl_value){
	netcamObj.SetVolume(vl_value);
	
	}



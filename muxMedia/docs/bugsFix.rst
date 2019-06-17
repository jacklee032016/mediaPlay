
apiClient -c osdPosition -o index=2,left=560,top=35,width=260,height=160

apiClient -c logo -o media=/index.jpg 


# restore default position of alert box
::

    apiClient -c osdPosition -o index=1,left=260,top=50,width=1400,height=220



	apiClient -c osdPosition -o index=1,left=260,top=50,width=1100,height=220
	
	
	apiClient -c osdPosition -o index=1,left=260,top=50,width=400,height=220
# show background
::

    apiClient -c osdBackground -o index=1,color=0xFFFF0000
	
# test border and align
::

    apiClient -c alert -o media="test this meessage 1234454567890ABCDEFGHIJKLMNOPQRSTUVWXYZ1234454567890ABCDEFGHIJKLMNOPQRSTUVWXYZ1234454567890ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	apiClient -c alert -o media="test this meessage",halign=1,valign=1


apiClient -c play -o media=/t10.mov


# test resolution and color space, color depth

apiClient -c resolution -o resolution=1080P_60	

apiClient -c resolution -o resolution=1080P_60

apiClient -c color -o color=0


Configuration 4KP_60, 4:2:0 and 36Bits:
---------------------------------------------
* SAMSUMG TV, only HDMI-1 (Native format of HDMI is 4K_P60:"3840X2160P_60")
* Must in following order:
	* config format;
	* then ColorSpace;
	* then ColorDepth;

	"Format":	"3840X2160P_60",
	"ColorSpace":	"YCBCR420",
	"ColorDepth":	"36Bit"

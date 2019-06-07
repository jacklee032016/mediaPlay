
# restore default position of alert box
::

    apiClient -c osdPosition -o index=1,left=260,top=50,width=1400,height=220

# show background
::

    apiClient -c osdBackground -o index=1,color=FFFF0000
	
# test border and align
::

    apiClient -c alert -o media="test this meessage 1234454565555555555555555555555555555555555555555555555555555555555555555555555555555555555555555589"
	
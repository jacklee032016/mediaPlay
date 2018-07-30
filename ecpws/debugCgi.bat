
@echo off

set REQUEST_METHOD=POST

rem set	CONTENT_LENGTH=23
rem WIN7_DEBUG\cgiMd5.exe < ecpws-temp.2XG

set CONTENT_LENGTH=51
WIN7_DEBUG\cgiHtmlEntities.exe < ecpws-temp.2XG

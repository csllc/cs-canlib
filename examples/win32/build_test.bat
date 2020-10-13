where /q cl.exe
IF ERRORLEVEL 1 (
   call "c:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)
		
cl /I ..\..\ext\canlib-win32-x64\include\ test.c ..\..\ext\canlib-win32-x64\lib\canlib32.lib

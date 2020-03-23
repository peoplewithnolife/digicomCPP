mingw32-make
if %ERRORLEVEL% EQU 0 (
   echo Build Success
   digicom_r 3 115200
) else (
   echo Failure Reason Given is %errorlevel%
)

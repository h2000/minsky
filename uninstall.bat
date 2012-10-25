rem Simple install script for Minsky
set MINSKY_HOME=%PROGRAMFILES%\Minsky

rem remove file association for .mky files
assoc .mky=
ftype Minsky=

rmdir/s/q "%ALLUSERSPROFILE%\Start Menu\Programs\Minsky"
del/q "%ALLUSERSPROFILE%\Desktop\Minsky.lnk"
del/q "%MINSKY_HOME%"

rem Simple install script for Minsky
set MINSKY_HOME=%PROGRAMFILES%\Minsky

rem create a file association for .mky files
assoc .mky=Minsky
ftype Minsky="%MINSKY_HOME%\minsky" "%%1"

rem copy exe files to MINSKY_HOME
mkdir "%MINSKY_HOME%"
mkdir "%MINSKY_HOME%\library"
mkdir "%MINSKY_HOME%\icons"
xcopy /y minsky.exe "%MINSKY_HOME%"
xcopy /y accountingRules "%MINSKY_HOME%"
xcopy /y uninstall.bat "%MINSKY_HOME%"
xcopy /y *.dll "%MINSKY_HOME%"
xcopy /y windows\*.dll "%MINSKY_HOME%"
xcopy /y *.tcl "%MINSKY_HOME%"
xcopy /s/y library  "%MINSKY_HOME%\library"
xcopy /s/y icons  "%MINSKY_HOME%\icons"

rem create shortcuts on desktop and programs menu
mkdir "%ALLUSERSPROFILE%\Start Menu\Programs\Minsky"
windows\Shortcut /a:c /f:"%ALLUSERSPROFILE%\Start Menu\Programs\Minsky\Minsky.lnk" /t:"%MINSKY_HOME%\minsky.exe"
windows\Shortcut /a:c /f:"%ALLUSERSPROFILE%\Start Menu\Programs\Minsky\uninstall.lnk" /t:"%MINSKY_HOME%\uninstall.bat"
windows\Shortcut /a:c /f:"%ALLUSERSPROFILE%\Desktop\Minsky.lnk" /t:"%MINSKY_HOME%\minsky.exe"

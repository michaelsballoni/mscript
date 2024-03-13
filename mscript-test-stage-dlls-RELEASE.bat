@echo off
xcopy /Y Win32\Release\mscript-sample.dll mscript-test-scripts\.
xcopy /Y Win32\Release\mscript-timestamp.dll mscript-test-scripts\.
xcopy /Y Win32\Release\mscript-registry.dll mscript-test-scripts\.
xcopy /Y Win32\Release\mscript-log.dll mscript-test-scripts\.
xcopy /Y Win32\Release\mscript-db.dll mscript-test-scripts\.
xcopy /Y Win32\Release\mscript-http.dll mscript-test-scripts\.
pause

@echo off
xcopy /Y Win32\Debug\mscript-sample.dll mscript-test-scripts\.
xcopy /Y Win32\Debug\mscript-timestamp.dll mscript-test-scripts\.
xcopy /Y Win32\Debug\mscript-registry.dll mscript-test-scripts\.
xcopy /Y Win32\Debug\mscript-log.dll mscript-test-scripts\.
xcopy /Y Win32\Debug\mscript-db.dll mscript-test-scripts\.
pause

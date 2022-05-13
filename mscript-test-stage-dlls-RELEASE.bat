@echo off

pushd C:\git\mscript
xcopy /Y Win32\Release\mscript-sample.dll mscript-test-scripts\.
xcopy /Y Win32\Release\mscript-timestamp.dll mscript-test-scripts\.
xcopy /Y Win32\Release\mscript-log.dll mscript-test-scripts\.
popd

pause

@echo off

pushd C:\git\mscript
xcopy /Y Win32\Release\mscript-dll-sample.dll mscript-test-scripts\.
xcopy /Y Win32\Release\mscript-timestamp.dll mscript-test-scripts\.
popd

pause

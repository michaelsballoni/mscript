@echo off

pushd C:\git\mscript
xcopy /Y Win32\Debug\mscript-dll-sample.dll mscript-test-scripts\.
xcopy /Y Win32\Debug\mscript-timestamp.dll mscript-test-scripts\.
xcopy /Y Win32\Debug\mscript-registry.dll mscript-test-scripts\.
popd

pause

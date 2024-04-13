? arguments.length() != 1
	> "Usage: mscript-builder.exe build.ms version"
	> 'version should be like "3.0.0"'
	* exit(0)
}

$ version = arguments.get(0)
> "Version " + version
* setenv("ms_version", version)

echo Binaries...
>!
rmdir /S /Q binaries
mkdir binaries
xcopy /Y ..\mscript\Win32\Release\*.dll binaries\.
xcopy /Y ..\mscript\Win32\Release\*.exe binaries\.

echo Samples...
>!
rmdir /S /Q samples
mkdir samples
xcopy /Y ..\mscript\mscript-examples\*.ms samples\.

echo Licenses...
>!
rmdir /S /Q licenses
mkdir licenses
xcopy /Y ..\mscript\mscript-licenses\*.* licenses\.

/ Collect our list of filenames we care about
$ filenames = list()
* filenames.add("mscript4.exe")
* filenames.add("mscript-db.dll")
* filenames.add("mscript-http.dll")
* filenames.add("mscript-log.dll")
* filenames.add("mscript-registry.dll")
* filenames.add("mscript-sample.dll")
* filenames.add("mscript-timestamp.dll")

/ Update our filenames to be in the relative binaries folder
++ f : 0 -> filenames.length() - 1
	* filenames.set(f, "binaries\" + filenames.get(f))
}

echo Resource hacking...
>!
del resources.res
ResourceHacker.exe -open resources.rc -save resources.res -action compile
@ filename : filenames
	> "..." + filename
	* setenv("ms_filepath", filename)
	ResourceHacker.exe -open %ms_filepath% -save %ms_filepath% -action addoverwrite -resource resources.res
}

echo Signing...all at once...
>!
* setenv("filenames_combined", filenames.join(" "))
signtool sign /n "Michael Balloni" %filenames_combined%

echo Building installer...
"C:\Program Files (x86)\Caphyon\Advanced Installer 21.6\bin\x86\AdvancedInstaller.com" /rebuild mscript.aip

echo Building release site...
>!
rmdir /S /Q site\releases\%ms_version%
mkdir site\releases\%ms_version%

echo Collecting samples...
>!
rmdir /S /Q site\samples
mkdir site\samples
xcopy /Y ..\mscript\mscript-examples\*.ms site\samples\.

echo Assembling release...
@ filename : filenames
	* setenv("ms_filepath", filename)
	xcopy /Y %ms_filepath% site\releases\%ms_version%\.
}

echo Finalizing installer...
* setenv("installer_filename", "mscript4.msi")
xcopy /Y /-I mscript-SetupFiles\mscript.msi site\releases\%ms_version%\%installer_filename%
signtool sign /n "Michael Balloni" site\releases\%ms_version%\%installer_filename%

echo All done.

? arguments.length() != 1
	> "Usage: mscript-builder.exe build.ms version"
	> 'version should be like "3.0.0"'
	* exit(0)
}

$ version = arguments.get(0)
> "Version " + version
* setenv("ms_version", version)

> "Binaries..."
>!
rmdir /S /Q binaries
mkdir binaries
xcopy /Y ..\mscript\Win32\Release\*.dll binaries\.
xcopy /Y ..\mscript\Win32\Release\*.exe binaries\.

> "Samples..."
>!
rmdir /S /Q samples
mkdir samples
xcopy /Y ..\mscript\mscript-examples\*.ms samples\.

> "Licenses..."
>!
rmdir /S /Q licenses
mkdir licenses
xcopy /Y ..\mscript\mscript-licenses\*.* licenses\.

> "Resource hacking and signing..."
>!
del resources.res
ResourceHacker.exe -open resources.rc -save resources.res -action compile
$ filenames = list()
* filenames.add("mscript4.exe")
* filenames.add("mscript-db.dll")
* filenames.add("mscript-http.dll")
* filenames.add("mscript-log.dll")
* filenames.add("mscript-registry.dll")
* filenames.add("mscript-sample.dll")
* filenames.add("mscript-timestamp.dll")
@ filename : filenames
	> "..." + filename
	* setenv("ms_filepath", "binaries\" + filename)
	ResourceHacker.exe -open %ms_filepath% -save %ms_filepath% -action addoverwrite -resource resources.res
	signtool sign /f mscript.pfx /p password %ms_filepath%
}

> "Building installer..."
AdvancedInstaller.com /rebuild mscript.aip

> "Building site..."
>!
rmdir /S /Q site\releases\%ms_version%
mkdir site\releases\%ms_version%

> "Collecting samples..."
>!
rmdir /S /Q site\samples
mkdir site\samples
xcopy /Y ..\mscript\mscript-examples\*.ms site\samples\.

> "Assembling release..."
@ filename : filenames
	> "..." + filename
	* setenv("ms_filepath", "binaries\" + filename)
	xcopy /Y %ms_filepath% site\releases\%ms_version%\.
}

> "Finalizing installer..."
* setenv("installer_filename", "mscript4.msi")
xcopy /Y mscript-SetupFiles\mscript.msi  site\releases\%ms_version%\%installer_filename%
signtool sign /n "Michael Balloni" site\releases\%ms_version%\%installer_filename%

> "All done."
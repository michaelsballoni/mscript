? arguments.length() != 1
	>> Usage: mscript-builder.exe build.ms version
	>> version should be like "3.0.0"
	exit(0)
}

$ version = arguments.get(0)
> "Version " + version
setEnv("ms_version", version)

$ exec_options = index("method", "system")

$ ignore_errors = exec_options.clone()
ignore_errors.set("ignore_errors", true)

>> Binaries...
exec("rmdir /S /Q binaries", ignore_errors)
exec("mkdir binaries", exec_options)
exec("xcopy /Y ..\mscript\Win32\Release\*.dll binaries\.", exec_options)
exec("xcopy /Y ..\mscript\Win32\Release\*.exe binaries\.", exec_options)

>> Samples...
exec("rmdir /S /Q samples", ignore_errors)
exec("mkdir samples", exec_options)
exec("xcopy /Y ..\mscript\mscript-examples\*.ms samples\.", exec_options)

>> Licenses...
exec("rmdir /S /Q licenses", ignore_errors)
exec("mkdir licenses", exec_options)
exec("xcopy /Y ..\mscript\mscript-licenses\*.* licenses\.", exec_options)

>> Resource hacking and signing...
exec("del resources.res", ignore_errors)
exec("ResourceHacker.exe -open resources.rc -save resources.res -action compile", exec_options)
$ filenames = list()
filenames.add("mscript3.exe")
filenames.add("mscript-sample.dll")
filenames.add("mscript-timestamp.dll")
filenames.add("mscript-registry.dll")
filenames.add("mscript-log.dll")
filenames.add("mscript-db.dll")
@ filename : filenames
	> "..." + filename
	setEnv("ms_filepath", "binaries\" + filename)
	exec("ResourceHacker.exe -open %ms_filepath% -save %ms_filepath% -action addoverwrite -resource resources.res", exec_options)
	exec("signtool sign /f mscript.pfx /p foobar %ms_filepath%", exec_options)
}

>> Building installer
exec("AdvancedInstaller.com /rebuild mscript.aip", exec_options)

>> Building site
exec("rmdir /S /Q ..\mscript.io\releases\%ms_version%", ignore_errors)
exec("mkdir ..\mscript.io\releases\%ms_version%", exec_options)

exec("rmdir /S /Q ..\mscript.io\samples", ignore_errors)
exec("mkdir ..\mscript.io\samples", exec_options)
exec("xcopy /Y ..\mscript\mscript-examples\*.ms ..\mscript.io\samples\.", exec_options)

@ filename : filenames
	> "..." + filename
	setEnv("ms_filepath", "binaries\" + filename)
	exec("xcopy /Y %ms_filepath% ..\mscript.io\releases\%ms_version%\.", exec_options)
}

>> Finalizing installer
$ installer_filename = "mscript3.msi"
exec("xcopy /Y mscript-SetupFiles\mscript.msi " + \
	 "..\mscript.io\releases\%ms_version%\" + installer_filename, exec_options)
exec("signtool sign /f mscript.pfx /p foobar " + \
	 "..\mscript.io\releases\%ms_version%\" + installer_filename, exec_options)

>> All done.

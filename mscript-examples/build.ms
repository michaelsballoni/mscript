? arguments.length() != 1
	>> Usage: mscript2 build.ms version
	>> version should be like "2.0.3"
	* exit(0)
}

$ version = arguments.get(0)
> "Version " + version
* setEnv("ms_version", version)

$ ignore_errors = index("ignore_errors", true)

>> Binaries...
* exec("rmdir /S /Q binaries", ignore_errors)
* exec("mkdir binaries")
* exec("xcopy /Y ..\mscript\Win32\Release\*.* binaries\.")

>> Resource hacking and signing...
* exec("del resources.res", ignore_errors)
* exec("ResourceHacker.exe -open resources.rc -save resources.res -action compile")
$ filename = list("mscript2.exe")
* filename.add("mscript-dll-sample.dll")
* filename.add("mscript-timestamp.dll")
* filename.add("mscript-registry.dll")
@ filename : filename
	> "..." + filename
	* setEnv("ms_filepath", "binaries\" + filename)
	* exec("ResourceHacker.exe -open %ms_filepath% -save %ms_filepath% -action addoverwrite -resource resources.res")
	* exec("signtool sign /f mscript.pfx /p foobar %ms_filepath%")
}

>> Building installer
* exec("AdvancedInstaller.com /rebuild mscript.aip")

>> Building site
* exec("rmdir /S /Q ..\mscript.io\releases\%ms_version%", ignore_errors)
* exec("mkdir ..\mscript.io\releases\%ms_version%")

* exec("rmdir /S /Q ..\mscript.io\samples", ignore_errors)
* exec("mkdir ..\mscript.io\samples")
* exec("xcopy /Y ..\mscript\mscript-examples\*.* ..\mscript.io\samples\.")

@ filename : filename
	> "..." + filename
	* setEnv("ms_filepath", "binaries\" + filename)
	* exec("xcopy /Y %ms_filepath% ..\mscript.io\releases\%ms_version%\.")
}

>> Finalizing installer
* exec("xcopy /Y mscript-SetupFiles\mscript.msi ..\mscript.io\releases\%ms_version%\.")
* exec("signtool sign /f mscript.pfx /p foobar ..\mscript.io\releases\%ms_version%\mscript.msi")

>> All done!

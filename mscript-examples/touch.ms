+ "mscript-timestamp.dll"

? arguments.length() != 1
	>> Provide the path to the file to touch
	exit(0)
}

$ file_path = arguments.get(0)
> "File: " + file_path
> "Last Modified Be4: " + msts_last_modified(file_path)

msts_touch(file_path)

> "Last Modified Now: " + msts_last_modified(file_path)

>> Loading music files...
$ lines = loadLines()
> "Music Files: " + lines.length()

$ db = index()
* processLines(db, lines)
>> All done

{
	$ artists = db.keys()
	$ album_count = 0
	$ song_count = 0
	@ artist : artists
		$ artistIndex = db.get(artist)
		@ albumName : artistIndex.keys()
			& album_count = album_count + 1
			@ albumTrack : artistIndex.get(albumName)
				& song_count = song_count + 1
			}
		}
	}
	>>
	> "Artists: " + artists.length()
	> "Albums: " + album_count
	> "Tracks: " + song_count
}

O
{
	>>
	>> Enter artist:
	$ value = input()
	> "Artist: " + value
}

~ loadLines()
	$ text = process("dir /B /S C:\Users\%USERNAME%\Music")
	$ clean = text.replaced(CRLF, LF)
	$ lines = clean.split("\n")
	<- lines
}

~ processLines(db, lines)
	$ lineCount = 0
	@ line : lines
		! FORNOW
		> line
		? processLine(db, line)
			! FORNOW
			exit(0)
		}
		& lineCount = lineCount + 1
		? (lineCount % 100) = 0
			> lineCount
		}
	}
}

~ processLine(db, line)
	& line = line.trim()
	$ parts = line.split("\")
	? parts.length() < 3
		<- false
	}
	
	$ artist = parts.get(parts.length() - 3)
	> "Artist: " + artist

	$ album = parts.get(parts.length() - 2)
	> "Album: " + album

	$ filename = parts.get(parts.length() - 1)
	$ dot_index = filename.lastLocation(".")
	? dot_index <= 0
		<- false
	}
	$ track = filename.subset(0, dot_index)
	> "Track: " + track

	? !db.has(artist)
		db.add(artist, index())
	}
	$ artistIndex = db.get(artist)

	? !artistIndex.has(album)
		artistIndex.add(album, list())
	}
	$ albumList = artistIndex.get(album)
	* albumList.add(track)

	<- true
}

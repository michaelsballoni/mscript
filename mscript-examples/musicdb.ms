$ in_query_mode = arguments.length() > 0

$ lines
? in_query_mode
	& lines = loadLines()
<>
	>> Loading music files...
	& lines = loadLines()
	> "Music Files: " + lines.length()
}

$ db = index()
* processLines(db, lines)
? !in_query_mode
	>> All done
	* outputStats(db)
}

O
	$ matching_artists
	? in_query_mode
		& matching_artists = getMatchingArtistNames(db, arguments)
	<>
		>>
		>> Enter artist name, as much as you'd like, however you'd like:
		& matching_artists = getMatchingArtistNames(db, split(input(), " "))
		> "Matching Artists: (" + matching_artists.length() + ")"
		? matching_artists.length() > 0
			> matching_artists.join(lf)
		}
	}
	@ matching_artist : matching_artists
		* summarizeArtist(db, matching_artist)
		>>
	}
}

~ loadLines()
	<- split(process("dir /B /S C:\Users\%USERNAME%\Music"), crlf)
}

~ processLines(db, lines)
	$ line_count = 0
	@ line : lines
		* processLine(db, line)
		& line_count = line_count + 1
		? (line_count % 1000) = 0
			> line_count
		}
	}
}

~ processLine(db, line)
	! Split up path, bail if not at least artist\album\track
	& line = line.trim()
	$ parts = line.split("\")
	? parts.length() < 3
		<- false
	}
	
	$ artist = parts.get(parts.length() - 3)
	$ album = parts.get(parts.length() - 2)
	$ filename = parts.get(parts.length() - 1)

	$ dot_index = filename.lastLocation(".")
	? dot_index <= 0
		! not a file
		<- false
	}
	$ track = filename.subset(0, dot_index)

	! DEBUG
	!> "Artist: " + artist + " - Album: " + album + " - Track: " + track

	! Ensure the artist is in the DB
	? !db.has(artist)
		* db.add(artist, index())
	}
	$ artist_index = db.get(artist)

	! Ensure the artist has the album
	? !artist_index.has(album)
		* artist_index.add(album, list())
	}
	$ album_list = artist_index.get(album)

	! Add the track to the album
	* album_list.add(track)

	! All done
	<- true
}

~ outputStats(db)
	$ artists = db.keys()
	$ album_count = 0
	$ track_count = 0
	@ artist : artists
		$ artist_index = db.get(artist)
		@ album_name : artist_index.keys()
			& album_count = album_count + 1
			@ album_track : artist_index.get(album_name)
				& track_count = track_count + 1
			}
		}
	}
	>>
	> "Artists: " + artists.length()
	? artists.length() > 0
		> "Albums:  " + album_count + " - albums / artist = " + round(album_count / artists.length())
		> "Tracks:  " + track_count + " - tracks / album = " + round(track_count / album_count)
	}
}

~ getMatchingArtistNames(db, parts)
	$ matching_artists = list()
	@ artist : db.keys()
		$ artist_lower = toLower(artist)
		$ match = true
		@ part : parts
			? !artist_lower.has(toLower(part))
				& match = false
				^
			}
		}
		? match
			* matching_artists.add(artist)
		}
	}
	<- matching_artists
}

~ summarizeArtist(db, artist_name)
	> "Artist: " + artist_name
	$ artist_albums = db.get(artist_name)
	@ album : artist_albums.keys()
		> "  Album: " + album
		$ album_tracks = artist_albums.get(album)
		@ album_track : album_tracks
			> "    " + album_track
		}
	}
}

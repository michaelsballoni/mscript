/ If we have cmd line arguments, those are our search terms
/ Handle things directly, no "UI", just results
? arguments.length() > 0
	$ lines = loadLines()
	
	$ db = index()
	* processLines(db, lines, false)

	$ matching_artists = getMatchingArtistNames(db, arguments)
	@ matching_artist : matching_artists
		* summarizeArtist(db, matching_artist)
		>
	}
	/ Exit the script with a good exit code
	<- 0
}

> "Loading music files..."
$ lines = loadLines()
> "Music Files: " + lines.length()

$ db = index()
* processLines(db, lines, true)
> "All done"
* outputStats(db)

O
	>
	> "Enter artist name search string, as much as you'd like, however you'd like:"
	$ matching_artists = getMatchingArtistNames(db, split(input(), " "))

	> fmt("Matching Artists: ({0})", matching_artists.length())
	? matching_artists.length() > 0
		> matching_artists.join(lf)
	}

	@ matching_artist : matching_artists
		* summarizeArtist(db, matching_artist)
		>
	}
}

/ Do the dir of the user's Music directory
/ Surprisingly fast/
~ loadLines()
	<- splitLines(popen('dir /B /S "C:\Users\%USERNAME%\Music"'))
	! err
		> "Running the dir command failed: " + err
		<- list()
	}
}

/ Walk dir output processing each line in turn
~ processLines(db, lines, should_pacify)
	? should_pacify
		$ line_count = 0
		@ line : lines
			* processLine(db, line)
			* line_count = line_count + 1
			? (line_count % 5000) = 0
				> line_count
			}
		}
	}
	<>
		@ line : lines
			* processLine(db, line)
		}
	}
}

/ Given a line from the dir output, add a track to our database...
/ ...if it's a somewhat valid line
~ processLine(db, line)
	/ Split up path, bail if not at least artist\album\track
	* line = line.trimmed()
	$ parts = line.split('\')
	? parts.length() < 3
		/ not deep enough
		<- false
	}
	
	$ filename = parts.get(parts.length() - 1)
	$ dot_index = filename.lastLocation('.')
	? dot_index <= 0
		/ not a file
		<- false
	}
	$ track = filename.subset(0, dot_index)

	$ artist = parts.get(parts.length() - 3)
	$ album = parts.get(parts.length() - 2)

	/ Ensure the artist is in the DB
	? !db.has(artist)
		* db.add(artist, index())
	}
	$ artist_index = db.get(artist)

	/ Ensure the artist has the album
	? !artist_index.has(album)
		* artist_index.add(album, list())
	}
	$ album_list = artist_index.get(album)

	/ Add the track to the album
	* album_list.add(track)

	/ All done
	<- true
}

/ Walk the database of artist collections gathering and outputting stats
~ outputStats(db)
	$ artists = db.keys()
	$ album_count = 0
	$ track_count = 0
	@ artist : artists
		$ artist_index = db.get(artist)
		@ album_name : artist_index.keys()
			* album_count = album_count + 1
			$ album_tracks = artist_index.get(album_name)
			* track_count = track_count + album_tracks.length()
		}
	}
	>
	> fmt("Artists: {0}", artists.length())
	? artists.length() > 0
		> fmt("Albums: {0} - albums / artist = {1}", album_count, round(album_count / artists.length()))
		> fmt("Tracks: {0} - tracks / album = {1}", track_count, round(track_count / album_count))
	}
}

/ Given search terms, return the names of artists that match all terms
~ getMatchingArtistNames(db, parts)
	/ Normalize the input, trimmed and lowered
	$ normalized_parts = list()
	@ part : parts
		$ normalized_part = trimmed(toLower(part))
		? normalized_part.length() > 0
			* normalized_parts.add(normalized_part)
		}
	}

	/ Walk the artists finding matches
	$ matching_artists = list()
	? normalized_parts.length() = 0
		<- matching_artists
	}
	@ artist : db.keys()
		$ artist_lower = toLower(artist)
		$ match = true
		@ part : normalized_parts
			? !artist_lower.has(part)
				* match = false
				V
			}
		}
		? match
			* matching_artists.add(artist)
		}
	}
	<- matching_artists
}

/ Output an artist's collection
~ summarizeArtist(db, artist_name)
	> fmt("Artist: {0}", artist_name)
	$ artist_albums = db.get(artist_name)
	@ album : artist_albums.keys()
		> fmt("  Album: {0}", album)
		$ album_tracks = artist_albums.get(album)
		@ album_track : album_tracks
			> "    " + album_track
		}
	}
}

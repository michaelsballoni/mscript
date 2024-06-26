~ fib(n)
	? fib_cache.has(n)
		<- fib_cache.get(n)
	}
	
	$ fib_result
	[] true
		= n <= 0
			* fib_result = 0
		}
		= n <= 2
			* fib_result = 1
		}
		<>
			* fib_result = fib(n - 1) + fib(n - 2)
		}
	}
	
	* fib_cache.add(n, fib_result)

	<- fib_result
}

~ fib2(n)
	? fib_cache.has(n)
		<- fib_cache.get(n)
	}
	
	$ fib_result
	[] true
		= n <= 0
			* fib_result = 0
		}
		= n <= 2
			* fib_result = 1
		}
		<>
			* fib_result = fib2(n - 1) + fib2(n - 2)
		}
	}	
	* fib_cache.add(n, fib_result)
	
	<- fib_result
}

$ fib_cache = index()

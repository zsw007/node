import ebcdic2ascii, sys, re, os
from ebcdic2ascii_encoder import *

blacklist = []

# checks if the given header is in the global blacklist, meant for external use
def header_exists(header):
	return header in blacklist

# resets the blacklist to initial empty list, meant for external use
def reset_blacklist():
	blacklist = []

def recursive_headers(filepath, include_paths):

	# need to separate the path before and after the extension in order to
	# include the _temp portion
	filenames = filepath.split('/')
	filename = filenames[-1]

	dot_search = DOT.search(filename)

	# in case there isn't an extension
	if dot_search is not None:
		target_to_return = dot_search.group(1) + "_temp." + dot_search.group(2)
		filenames[-1] = target_to_return
		target = '/'.join(filenames)
	else:
		target = filepath + "_temp"
		target_to_return = filename + "_temp"
	print target
	print target_to_return

	# add the source header path to the blacklist to ensure the same
	# headers aren't being translated again
	if not header_exists(filepath):
		if not os.path.isfile(filepath):
			return 1

		blacklist.append(filepath)
		ebcdic2ascii.open_files([filepath, target], False, False, include_paths)

	# return the location to the target header path so that it can be
	# written into the translated source file
	return target_to_return

if __name__ == "__main__":
	recursive_headers(sys.argv[1], sys.argv[2], sys.argv[3])

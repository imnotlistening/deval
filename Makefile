#
# Build deval. The source code is located in src/ and include/. Make sure you
# have run buildprep.sh before this is run. 
#

# Main target.
all:
	if [ ! -e config.mak ]; then \
	echo "Run buildprep.sh before running 'make'."; \
	exit 1; \
	fi
	cd src && make


clean:
	cd src && make clean

dist-clean:
	cd src && make dist-clean
	rm -f config.mak

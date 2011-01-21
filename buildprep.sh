#!/bin/sh

#
# Extract the SVN version from this directory and use that as the version.
#

svnversion -c . >.tmp.txt
awk 'BEGIN { FS = ":" } { 
if ( $2 ) 
  print $2;
else
  print $1
}' .tmp.txt

REVISION=`svnversion .`

echo "REVISION = $REVISION" > config.mak

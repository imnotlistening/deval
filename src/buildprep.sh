#!/bin/sh

#
# Extract the SVN version from this directory and use that as the version.
#

REVISION=`svnversion`

echo "REVISION = $REVISION" > config.mak

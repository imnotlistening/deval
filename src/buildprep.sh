#!/bin/sh

#
# Extract the SVN version from this directory and use that as the version.
#

REVISION=`svn info . | awk '/Revision/ { print $2; }'`

echo "REVISION = $REVISION" > config.mak

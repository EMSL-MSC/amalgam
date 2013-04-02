#!/bin/bash

VERSION=`svnversion -n $*`
if [ "x$VERSION" == "xexported" ]
then
	pushd $* 2>/dev/null >/dev/null
	VERSION="gs$(fgit svn find-rev $(git log -1 --pretty=format:%H 2>/dev/null) 2>/dev/null)"
	popd 2>/dev/null > /dev/null
	if [ "x$VERSION" == "xgs" ]
	then
		pushd $* 2>/dev/null >/dev/null
		VERSION=`git log -1 --pretty=format:%H 2>/dev/null`
		popd 2>/dev/null > /dev/null
		if [ "x$VERSION" == "x" ]
		then
			if [ -f "$*/svnver.txt" ]
			then
				cat "$*/svnver.txt"
			else
				echo "Unknown"
			fi
		else
			echo "$VERSION"
		fi
	else
		echo "$VERSION"
	fi
else
	echo "$VERSION"
fi

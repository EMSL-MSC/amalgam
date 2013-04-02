#!/bin/bash

BRANCH=`svn info $* 2>/dev/null| grep '^URL:' | sed 's/^URL:[ \t]*//'`
if [ "x$BRANCH" == "x" ]
then
	pushd $* > /dev/null 2>/dev/null
	BRANCH=`git svn info | grep '^URL:' | sed 's/^URL:[ \t]*//'`
	popd > /dev/null 2>/dev/null
	if [ "x$BRANCH" == "x" ]
	then
		if [ -f "$*/svnbranch.txt" ]
		then
			cat "$*/svnbranch.txt"
		else
			echo "Unknown"
		fi
	else
		echo "$BRANCH"
	fi
else
	echo "$BRANCH"
fi


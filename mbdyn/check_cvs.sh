#!/bin/sh

cvs status -v 2>/dev/null | \
awk '
	BEGIN {
		movetag = "MBDYN_REL_ENG";
	}
	/Repository revision/ {
		c = $3;
		f = $4;
	} 
	$1=="MBDYN_REL_ENG" {
		if (match($3, "([.0-9]+)")) {
			r = substr($3, RSTART, RLENGTH);
			if (c != r) {
				# move tag
				printf("cvs tag -r %s -F %s %s\n", c, movetag, f);
				# diff
				# printf("cvs diff -u -r %s -r %s %s\n", r, c, f);
			}
		}
	}' | \
	sed "s;/.*mbdyn-1\.0/;;" | \
	sed "s;,v;;"


#!/bin/sh

OLD=2005
NEW=`expr $OLD + 1`

for i in `grep -ErIl 'Copyright.*-'${OLD} .`; do 
	echo $i
	sed 's;\(Copyright.*\)'${OLD}';\1'${NEW}';' $i > x 
	mv -f x $i
done

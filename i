#!/bin/bash

#
# $B%/%m!<%s:n@.D>8e!"$3$N%7%'%k%9%/%j%W%H$r<B9T$7$F!"(B
# $B!&%U%!%$%k$N%Q!<%_%C%7%g%s$rE,@Z$J$b$N$KJQ99(B
# $B!&(Btoscom$B$N;n83MQ%U%!%$%k$N%j%s%/:n@.(B
# $B!&(BsmplComm,analyzer,resetTxt $B$N(B newenv$B<B9T(B
# $B$r9T$$!"3F4D6-$G$N%S%k%I$,2DG=$J>uBV$K$9$k!#(B
#

echo "--- set 666 to all files ---"
chmod 666 .gitignore AfterClone.txt README.md
find toscom   -type f | xargs chmod 666
find analyzer -type f | xargs chmod 666
find smplComm -type f | xargs chmod 666
find resetTxt -type f | xargs chmod 666

echo "--- execute e script ---"
chmod 755 e
./e

echo "--- set toscom ---"
cd toscom
./xt
./rt
cd ..

echo "--- set smplComm ---"
cd smplComm
./newenv
cd ..

echo "--- set analyzer ---"
cd analyzer
./newenv
cd ..

echo "--- set resetTxt ---"
cd resetTxt
./newenv
cd ..

echo "--- all jobs done ---"


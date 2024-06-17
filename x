#!/bin/bash

#
# $B%j%]%8%H%jFb$N%7%'%k%9%/%j%W%H$N(B shebang$B$r=q$-49$($k!#(B
# $BK\%9%/%j%W%H$N(B shebang($B%U%!%$%kKAF,(B) $B$GE}0l$9$k!#(B
#

SHELL="bash"
SHEBANG="$(head -n 1 x)"
#echo -e "${SHEBANG}..."

function changeShebang() {
  for _file; do
    local _shebang="$(head -n 1 ${_file})"
    echo -n "${_file}  "
    if [[ ! "${_shebang}" =~ \/${SHELL}$ ]]; then
      echo -e "${_shebang}"
      echo "  ...shebang not descripted, maybe"
      continue
    fi
    echo "...shebang changed"
    sed -i "1c ${SHEBANG}" ${_file}
  done
}

. scriptList

echo "--- tools ---"
echo "(${SLtools})"
changeShebang ${SLtools}

echo "--- toscom ---"
echo "(${SLtoscom})"
cd toscom
changeShebang ${SLtoscom}

echo "--- toscom/BUILD/.lib ---"
echo "(${SLbuildlib})"
cd BUILD/.lib
changeShebang ${SLbuildlib}

cd ../../..

echo "--- smplComm ---"
echo "(${SLsmplcomm})"
cd smplComm
changeShebang ${SLsmplcomm}
cd ..

echo "--- wintest ---"
echo "(${SLwintest})"
cd wintest
changeShebang ${SLwintest}
cd ..

echo "--- analyzer ---"
echo "(${SLanalyzer})"
cd analyzer
changeShebang ${SLanalyzer}
cd ..

echo "--- resetTxt ---"
echo "(${SLresettxt})"
cd resetTxt
changeShebang ${SLresettxt}
cd ..


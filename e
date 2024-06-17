#!/bin/bash

#
# 実行属性を付与するスクリプト
#

. scriptList

echo "--- tools ---"
echo "(${SLtools})"
chmod 755 ${SLtools}

echo "--- toscom ---"
echo "(${SLtoscom})"
cd toscom
chmod 755 ${SLtoscom}

echo "--- toscom/BUILD/.lib ---"
echo "(${SLbuildlib})"
cd BUILD/.lib
chmod 755 ${SLbuildlib}

cd ../../..

echo "--- smplComm ---"
echo "(${SLsmplcomm})"
cd smplComm
chmod 755 ${SLsmplcomm}
cd ..

echo "--- wintest ---"
echo "(${SLwintest})"
cd wintest
chmod 755 ${SLwintest}
cd ..

echo "--- analyzer ---"
echo "(${SLanalyzer})"
cd analyzer
chmod 755 ${SLanalyzer}
cd ..

echo "--- resetTxt ---"
echo "(${SLresettxt})"
cd resetTxt
chmod 755 ${SLresettxt}
cd ..


#!/bin/bash

#
# 実行属性を付与するスクリプト
#

echo "--- tools ---"
chmod 755 p z c w

echo "--- toscom ---"
cd toscom
chmod 755 xnameconf xconv xname xt rt TEST/_test
cd NEWENV
chmod 755 mpu newenv
cd ../BUILD/.lib
chmod 755 collectLibFiles gcc_sample ready
cd ../../..

echo "--- smplComm ---"
cd smplComm
chmod 755 mpu newenv s1 s2
cd ..

echo "--- wintest ---"
cd wintest
chmod 755 mpu newenv
cd ..

echo "--- analyzer ---"
cd analyzer
chmod 755 mpu newenv
cd ..

echo "--- resetTxt ---"
cd resetTxt
chmod 755 mpu newenv
cd ..


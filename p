#!/bin/bash

# toscom利用開発環境のクリーンパッケージ作成
#   ./p ディレクトリ名
#  カレントディレクトリのもののみ有効
#  ディレクトリ名末尾に / が付いていても大丈夫

DIR=${1%%/*}
if [ ! -d ${DIR} ]; then
  echo "!! $DIR not exist"
  exit
fi
if [ ! -d ${DIR}/BUILD ]; then
  echo "!! maybe $1 is not target"
  exit
fi

cd ${DIR}/BUILD
make allclean
cd ../..
tarname="${DIR,,}"
if [ "x$2" != "x" ]; then tarname+="_${2}"; fi
tar --exclude .svn -zcvf ${tarname}.tar.gz ${DIR}


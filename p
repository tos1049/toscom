#!/bin/bash
DIRNAME=$1
TARSUFFIX=$2

# toscom利用開発環境のクリーンパッケージ作成
#   ./p ディレクトリ名 tarファイル名に付与する文字列
#  カレントディレクトリのもののみ有効
#  ディレクトリ名末尾に / が付いていても大丈夫

DIR=${DIRNAME%%/*}
if [ ! -d ${DIR} ]; then
  echo "!! ${DIR} not exist"
  exit
fi
if [ ! -d ${DIR}/BUILD ]; then
  echo "!! maybe ${DIRNAME} is not target"
  exit
fi

cd ${DIR}/BUILD
make allclean
cd ../..
TARNAME="${DIR,,}"
if [ -n "${TARSUFFIX}" ]; then TARNAME+="_${TARSUFFIX}"; fi
tar --exclude .svn -zcvf ${TARNAME}.tar.gz ${DIR}


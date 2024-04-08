#!/bin/bash

# リリースファイル展開とビルド確認

TESTDIR="newtest"
TESTFILE="test"
SMPLFILE="sample"
ANLZFILE="Analyzer"
CHECKPRE="_CHECKF_"

STAZ="smplcomm.tar.gz"
ATAZ="analyzer.tar.gz"

# 一番日付の新しいファイルを処理対象とする
mapfile -t TARLIST < <(ls -t toscom.*.tar.gz)
TARFILE=${TARLIST[0]}

if [ -z "${TARFILE}" ]; then
  echo "no archive file..."
  exit
fi

rm -fr ${TESTDIR}
mkdir ${TESTDIR}

cp ${TARFILE} ${TESTDIR}
cd ${TESTDIR}
tar xvfz ${TARFILE} >& /dev/null

function checker() {
  local _title=$1
  local _makeTarget=$2
  local _binaryFile=$3
  local _label=$4

  cat<< CHECKTOP


------- check ${_title} -------
CHECKTOP

  cd BUILD
  echo "> make ${_makeTarget}"
  make ${_makeTarget}
  if [ ! -e ${_binaryFile} ]; then
    cat<< CHECKNG
********************************
 ${_label} ビルド失敗！
********************************
CHECKNG
    exit
  fi
  cd ../..
}

########## make rel
cd toscom
checker "${TARFILE}" "rel" "${TESTFILE}" "toscom"

########## com_testtos.c とともに make checkf (com_test.c はここで削除)
echo
echo
cd toscom
tar xvfz com_testtos.tar.gz
rm com_test.c
checker "${TARFILE} with test by checkf" "checkf" \
        "${CHECKPRE}${TESTFILE}" "toscom with test"

########## newenv.tar.gz をエキストラ機能追加してから make rel
echo
echo
tar xvfz toscom/newenv.tar.gz
echo
cd NEWENV
sed -i -e "/^USE_EXTRA=0/c USE_EXTRA=1" newenv
./newenv
checker "newenv.tar.gz" "rel" "${TESTFILE}" "newenv"

if [ -e toscom/${STAZ} ]; then
########## smplcomm.tar.gz の make rel (ver1.0)
  echo
  echo
  tar xvfz toscom/${STAZ}
  cd smplComm
  checker "${STAZ} (ver1.0)" "rel" "${SMPLFILE}" "smplcomm ver1"
  
########## smplcomm.tar.gz の make rel (ver2.0)
  echo
  echo
  cd smplComm
  ./s2
  checker "${STAZ} (ver2.0)" "rel" "${SMPLFILE}" "smplcomm ver2"
  
########## smplcomm.tar.gz の make checkf (ver2.0)
  cd smplComm
  checker "${STAZ} (ver2.0) by checkf" "checkf" \
          "${CHECKPRE}${SMPLFILE}" "smplcomm ver2 (checkf)"
fi

if [ -e toscom/${ATAZ} ]; then
########## analyzer.tar.gz の make rel
  echo
  echo
  tar xvfz toscom/${ATAZ}
  cd analyzer
  checker "${ATAZ}" "rel" "${ANLZFILE}" "analyzer"
  
########## analyzer.tar.gz の make checkf
  cd analyzer
  checker "${ATAZ} by checkf" "checkf" \
          "${CHECKPRE}${ANLZFILE}" "analyzer (checkf)"
fi

########## xconvを使ってEUC変換後、make rel
echo 
echo 
cd toscom
./xconv
checker "${TARFILE} (convert to EUC)" "rel" "${TESTFILE}" "toscom"

########## xnameを使ってモジュール名を tosに変換後、make lib
echo 
echo 
cd toscom
sed -i -e 's/NEWMODULE="com"/NEWMODULE="tos"/' xnameconf
./xname
tar xvfz tos_testtos.tar.gz >& /dev/null
checker "${TARFILE} (lib with module rename)" "lib" "libtoscom.a" "libtoscom"

########## NEWENVを tosモジュールに変更して make rel
echo
echo
cd NEWENV
./newenv
checker "NEWENV (module rename)" "rel" "${TESTFILE}" "newenv"

if [ -e toscom/${STAZ} ]; then
########## smplcomm.tar.gz を tosモジュールに変更して Ver2 を make rel
  echo
  echo
  rm -fr smplComm
  tar xvfz toscom/smplcomm.tar.gz >& /dev/null
  cd smplComm
  ./newenv
  echo
  echo
  ./s2
  checker "${STAZ} (ver2.0/module rename)" "rel" \
          "${SMPLFILE}" "smplcomm ver2"
fi

if [ -e toscom/${ATAZ} ]; then
########## analyzer.tar.gz を tosモジュールに変更して make rel
  echo
  echo
  rm -fr analyzer
  tar xvfz toscom/analyzer.tar.gz >& /dev/null
  cd analyzer
  ./newenv
  checker "${ATAZ} (module rename)" "rel" "${ANLZFILE}" "analyzer"
fi


# 全チェック終了
cat<< CHECKOK


===== check ${TARFILE} end =====

BUILD CHECK ALL GREEN!
checked directory: ${TESTDIR}

(HIT ENTER KEY)
CHECKOK

read

# 最後にチェック用ディレクトリを削除して終了
cd ..
rm -fr ${TESTDIR}
echo "removed ${TESTDIR}"


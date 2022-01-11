#!/bin/bash

# リリースファイル展開とビルド確認

testdir="newtest"
testfile="test"
smplfile="sample"
anlzfile="Analyzer"
checkpre="_CHECKF_"

STAZ="smplcomm.tar.gz.dummy"   # smplCommの確認は今は省略させる
ATAZ="analyzer.tar.gz"

# 一番日付の新しいファイルを処理対象とする
tarlist=(`ls -t toscom.*.tar.gz`)
tarfile=${tarlist[0]}

rm -fr $testdir
mkdir $testdir

cp $tarfile $testdir
cd $testdir
tar xvfz $tarfile >& /dev/null

function checker() {
  cat<< CHECKTOP


------- check $1 -------
CHECKTOP
  cd BUILD
  make $2
  if [ ! -e $3 ]; then
    cat<< CHECKNG
********************************
 $4 ビルド失敗！
********************************
CHECKNG
    exit
  fi
}

########## make rel
cd toscom
checker "$tarfile" "rel" "$testfile" "toscom"

########## com_testtos.c とともに make checkf (com_test.c はここで削除)
echo
echo
cd ..
tar xvfz com_testtos.tar.gz
rm com_test.c
checker "$tarfile with test by checkf" "checkf" \
        "$checkpre$testfile" "toscom with test"

########## newenv.tar.gz をエキストラ機能追加してから make rel
echo
echo
cd ../..
tar xvfz toscom/newenv.tar.gz
echo
cd NEWENV
sed -i -e "/^USE_EXTRA=0/c USE_EXTRA=1" newenv
./newenv
checker "newenv.tar.gz" "rel" "$testfile" "newenv"

if [ -e toscom/$STAZ ]; then
########## smplcomm.tar.gz の make rel (ver1.0)
  cd ../..
  tar xvfz toscom/$STAZ >& /dev/null
  cd smplComm
  checker "$STAZ (ver1.0)" "rel" "$smplfile" "smplcomm ver1"
  
########## smplcomm.tar.gz の make rel (ver2.0)
  cd ../
  ./xs2 >& /dev/null
  checker "$STAZ (ver2.0)" "rel" "$smplfile" "smplcomm ver2"
  
########## smplcomm.tar.gz の make checkf (ver2.0)
  cd ../
  checker "$STAZ (ver2.0) by checkf" "checkf" \
          "$checkpre$smplfile" "smplcomm ver2 (checkf)"
fi

if [ -e toscom/$ATAZ ]; then
########## analyzer.tar.gz の make rel
  cd ../../
  tar xvfz toscom/$ATAZ >& /dev/null
  cd analyzer
  checker "$ATAZ" "rel" "$anlzfile" "analyzer"
  
########## analyzer.tar.gz の make checkf
  cd ../
  checker "$ATAZ by checkf" "checkf" \
          "$checkpre$anlzfile" "analyzer (checkf)"
fi

########## xconvを使ってEUC変換後、make rel
echo 
echo 
cd ../../toscom
./xconv
checker "$tarfile (convert to EUC)" "rel" "$testfile" "toscom"

########## xnameを使ってモジュール名を tosに変換後、make lib
echo 
echo 
cd ../../toscom
./xname
tar xvfz tos_testtos.tar.gz >& /dev/null
checker "$tarfile (lib with module rename)" "lib" "libtoscom.a" "libtoscom.a"

if [ -e toscom/$STAZ ]; then
########## smplcomm.tar.gz を tosモジュールに変更して make rel
  echo
  echo
  cd ../../smplComm
  sed -i.bak -e 's/NEWMODULE="com"/NEWMODULE="tos"/' newenv
  ./newenv
  rm tos_window.*
  checker "$STAZ (ver2.0/module rename)" "rel" \
          "$smplfile" "smplcomm ver2"
fi

if [ -e toscom/$ATAZ ]; then
########## analyzer.tar.gz を tosモジュールに変更して make rel
  echo
  echo
  cd ../../analyzer
  sed -i.bak -e 's/NEWMODULE="com"/NEWMODULE="tos"/' newenv
  ./newenv
  checker "$ATAZ (module rename)" "rel" "$anlzfile" "analyzer"
fi


# 全チェック終了
cat<< CHECKOK


===== check $targile end =====

BUILD CHECK ALL GREEN!
checked directory: $testdir

(HIT ENTER KEY)
CHECKOK

read

# 最後にチェック用ディレクトリを削除して終了
cd ../../..
rm -fr $testdir
echo "removed $testdir"


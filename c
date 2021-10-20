#!/bin/bash

# リリースファイル展開とビルド確認

testdir="newtest"
testfile="test"
smplfile="sample"
anlzfile="Analyzer"
checkpre="_CHECKF_"

# 一番日付の新しいファイルを処理対象とする
tarlist=(`ls -t toscom.*.tar.gz`)
tarfile=${tarlist[0]}

rm -fr $tesfdir
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

cd toscom
checker "$tarfile" "rel" "$testfile" "toscom"

cd ..
tar xvfz com_testtos.tar.gz
rm com_test.c
checker "$tarfile with test by checkf" "checkf" \
        "$checkpre$testfile" "toscom with test"

cd ../..
tar xvfz toscom/newenv.tar.gz >& /dev/null
cd NEWENV
./newenv
checker "newenv.tar.gz" "rel" "$testfile" "newenv"

if [ -e toscom/smplcomm.tar.gz ]; then
  cd ../..
  tar xvfz toscom/smplcomm.tar.gz >& /dev/null
  cd smplComm
  checker "smplcomm.tar.gz (ver2.0)" "rel" "$smplfile" "smplcomm ver2"
  
  cd ../
  ./xs1 >& /dev/null
  checker "smplcomm.tar.gz (ver1.0)" "rel" "$smplfile" "smplcomm ver1"
  
  cd ../
  checker "smplcomm.tar.gz (ver1.0) by checkf" "checkf" \
          "$checkpre$smplfile" "smplcomm ver1 (checkf)"
fi

if [ -e toscom/analyzer.tar.gz ]; then
  cd ../../
  tar xvfz toscom/analyzer.tar.gz >& /dev/null
  cd analyzer
  checker "analyzer.tar.gz" "rel" "$anlzfile" "analyzer"
  
  cd ../
  checker "analyzer.tar.gz by checkf" "checkf" \
          "$checkpre$anlzfile" "analyzer (checkf)"
fi

echo 
echo 
cd ../../toscom
./xconv
checker "$tarfile (convert to EUC)" "rel" "$testfile" "toscom"

echo 
echo 
cd ../../toscom
./xname
checker "$tarfile (module rename)" "rel" "$testfile" "toscom"

if [ -e toscom/smplcomm.tar.gz ]; then
  echo
  echo
  cd ../../smplComm
  sed -i.bak -e 's/NEWMODULE="com"/NEWMODULE="tos"/' newenv
  ./newenv
  rm tos_window.*
  checker "smplcomm.tar.gz (ver1.0/module rename)" "rel" \
          "$smplfile" "smplcomm ver1"
fi

if [ -e toscom/analyzer.tar.gz ]; then
  echo
  echo
  cd ../../analyzer
  sed -i.bak -e 's/NEWMODULE="com"/NEWMODULE="tos"/' newenv
  ./newenv
  checker "analyzer.tar.gz (module rename)" "rel" "$anlzfile" "analyzer"
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


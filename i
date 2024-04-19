#!/bin/bash

#
# クローン作成直後、このシェルスクリプトを実行して、
# ・ファイルのパーミッションを適切なものに変更
# ・toscomの試験用ファイルのリンク作成
# ・smplComm,analyzer,resetTxt の newenv実行
# を行い、各環境でのビルドが可能な状態にする。
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


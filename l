#!/bin/bash

#
# 関数の平均ライン数を計算する
#

# CTRL+C で中断時も一時ファイルを削除するようにする。
# (.tmptotal が残っていると、結果出力のみのモードになるため)
trap intruptFinish sigint

TMPTOTAL=".tmptotal"

# .tmptotal を敢えて作成することで、それを読み込んで結果出力する試験動作可能。
# 動作確認後は手動で .tmptotal を削除すること。
if [ -e ${TMPTOTAL} ]; then
  ONLYRESULT="YES"
else
  cat /dev/null > ${TMPTOTAL}
fi

echo "check: $(date)"

function getFuncName() {
  local _line="$1"
  GETLINE=$(echo "${_line}" | awk -F ":" '{print $1}')
  #printf "%-5d : " "${GETLINE}"
  GETFUNC=$(echo "${_line}" | grep -o -E "[^[:space:]]+\(")
  #echo "${GETFUNC}"
  while [[ "${GETFUNC}" =~ \* ]]; do
    GETFUNC="${GETFUNC/\*/}"
  done
  GETFUNC="${GETFUNC/\(/}"
  #echo "${GETFUNC}()"
}

function searchLine() {
  local _target="$1"
  local _topLine=$2
  local _key=$3
  local _endLine=$((_topLine + TRIMLINE))
  local _grep=$(tail -n +${_topLine} ${_target} | grep -m 1 -n "^${_key}")
  echo "${_grep}" | awk -F ":" '{print $1}'
}

function getLineCount() {
  local _target="$1"
  local _topLine=$2
  local _start=$(searchLine ${_target} ${_topLine} "{")
  local _end=$(searchLine ${_target} ${_topLine} "}")
  ((FUNCLINE = _end - _start - 1))
}

function calcAvr() {
  local _lines=$1
  local _funcs=$2
  echo "scale=2; ${_lines} / ${_funcs}" | bc
}

GREPFUNC='[[:alnum:][:space:]_]+([*[:alnum:]_]+)\('

function getFunctions() {
  local _target=$1
  echo
  if [ -e ${_target} ]; then
    echo "${_target}"
  else
    echo "not exist... ${_target}"
    return
  fi
  grep -E -n "^([[:alnum:]_*]+[[:space:]]+)+([*[:alnum:]_]+)\(" ${_target} | {
    TOTALLINES=0
    FUNCCOUNT=0
    AVRLINES=0
    while read _grepline; do
      if [[ "${_grepline}" =~ typedef ]]; then continue; fi
      if [[ "${_grepline}" =~ \#define ]]; then continue; fi
      #echo "${_grepline}"
      getFuncName "${_grepline}"
      getLineCount "$_target" "${GETLINE}"
      printf "%5d : %s()\n" ${FUNCLINE} ${GETFUNC}
      ((FUNCCOUNT++))
      ((TOTALLINES += FUNCLINE))
    done
    echo "--------------------------------------------"
    AVRLINES=$(calcAvr ${TOTALLINES} ${FUNCCOUNT})
    printf "  total = %d lines (%d functions)\n" ${TOTALLINES} ${FUNCCOUNT}
    printf "  avrage = %5.2f\n" ${AVRLINES}
    echo "--------------------------------------------"
    # ここの処理は本シェルのプロセスとは別プロセスで処理されるため、
    # 本シェルプロセスの calcTotalResult()で集計が出来るように、
    # ファイルごとの結果を一時ファイルに書き込む。
    echo "${TOTALLINES} ${FUNCCOUNT} ${_target}" >> ${TMPTOTAL}
  }
}

function checkSource() {
  if [ "${ONLYRESULT}" == "YES" ]; then return; fi
  getFunctions toscom/com_proc.c
  getFunctions toscom/com_procThread.c
  getFunctions toscom/com_spec.c
  getFunctions toscom/com_debug.c
  getFunctions toscom/com_extra.c
  getFunctions toscom/com_select.c
  getFunctions toscom/com_window.c
  getFunctions toscom/com_signal.c
  getFunctions toscom/com_signalCom.c
  getFunctions toscom/com_signalSet1.c
  getFunctions toscom/com_signalSet2.c
  getFunctions toscom/com_signalSet3.c
}

function calcTotalResult() {
  echo 
  echo "============================================"
  echo " lines  funcs  average filename"
  cat ${TMPTOTAL} | {
    local _totalLines=0
    local _totalFuncs=0
    while read _lines _funcs _filename
    do
      local _avr=$(calcAvr ${_lines} ${_funcs})
      printf "%6d %6d  %6.2f   %s\n" ${_lines} ${_funcs} ${_avr} ${_filename}
      ((_totalLines += _lines))
      ((_totalFuncs += _funcs))
    done
    echo "--------------------------------------------"
    local _totalAvr=$(calcAvr ${_totalLines} ${_totalFuncs})
    printf "%6d %6d  %6.2f\n" ${_totalLines} ${_totalFuncs} ${_totalAvr}
  }

}

function removeTmpFiles() {
  if [ "${ONLYRESULT}" != "YES" ]; then rm -fr ${TMPTOTAL}; fi
}

function intruptFinish() {
  removeTmpFiles
  exit
}

checkSource
calcTotalResult
removeTmpFiles


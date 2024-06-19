#!/bin/bash

#
# 関数の平均ライン数を計算する
#

GREPFUNC='\([[:alnum:]_]\+\)[[:space:]*]\+\([[:alnum:]_]\+\)('
TMPPUBLIC=".tmpgrep1"
TMPSTATIC=".tmpgrep2"
TMPMERGE=".tmpgrep3"
TMPTOTAL=".tmpgrep4"

# .tmpgrep4 を敢えて作成することで、それを読み込んで結果出力する試験動作可能。
# 動作確認後は手動で .tmpgrep4 を削除すること。
if [ -e ${TMPTOTAL} ]; then
  ONLYRESULT="YES"
else
  cat /dev/null > ${TMPTOTAL}
fi

function getFuncName() {
  local _line="$1"
  GETLINE=$(echo "${_line}" | awk -F ":" '{print $1}')
  #printf "%-5d : " "${GETLINE}"
  # staticがあったら、その分 1つ後のトークンを見ないと関数名にならない
  if [[ "${_line}" =~ [0-9]+:static ]]; then
    GETFUNC=$(echo "${_line}" | awk '{print $3}')
    #echo -n "static "
  else
    GETFUNC=$(echo "${_line}" | awk '{print $2}')
  fi
  # 取得した関数名の冒頭に付いているかもしれない * と末尾の ( を削除
  while [[ "${GETFUNC}" =~ \* ]]; do
    GETFUNC=${GETFUNC/\*/}
  done
  GETFUNC=${GETFUNC/\(/}
  #echo "${GETFUNC}()"
}

function getLineCount() {
  local _target="$1"
  local _topLine=$2
  local _startLine=${_topLine}
  while
    ((_startLine++))
    local _line=$(sed -n "${_startLine}p" ${_target})
    [[ ! ${_line} =~ ^\{ ]]
  do true; done
  local _endLine=${_startLine}
  while
    ((_endLine++))
    local _line=$(sed -n "${_endLine}p" ${_target})
    [[ ! ${_line} =~ ^\} ]]
  do true; done
  ((FUNCLINE = _endLine - _startLine - 1))
}

function calcAvr() {
  local _lines=$1
  local _funcs=$2
  echo "scale=2; ${_lines} / ${_funcs}" | bc
}

function getFunctions() {
  local _target=$1
  echo
  if [ -e ${_target} ]; then
    echo "${_target}"
  else
    echo "not exist... ${_target}"
    return
  fi
  grep -n "^${GREPFUNC}" ${_target} > ${TMPPUBLIC}
  grep -n "^static[[:space:]]*${GREPFUNC}" ${_target} > ${TMPSTATIC}
  cat ${TMPPUBLIC} ${TMPSTATIC} | sort -g > ${TMPMERGE}
  cat ${TMPMERGE} | {
    TOTALLINES=0
    FUNCCOUNT=0
    AVRLINES=0
    while read _grepline; do
      if [[ "${_grepline}" =~ typedef ]]; then continue; fi
      if [[ "${_grepline}" =~ \#define ]]; then continue; fi
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
  rm -fr ${TMPPUBLIC}
  rm -fr ${TMPSTATIC}
  rm -fr ${TMPMERGE}
  if [ "${ONLYRESULT}" != "YES" ]; then rm -fr ${TMPTOTAL}; fi
}

checkSource
calcTotalResult
removeTmpFiles


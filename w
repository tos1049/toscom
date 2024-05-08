#!/bin/bash
WORKDIR=$1

# ワークディレクトリにアーカイブをコピーし展開する

if [ -z "${WORKDIR}" ]; then
  WORKDIR="work_toscom"
fi

echo "WORKDIR=${WORKDIR}"

if [ ! -e ${WORKDIR} ]; then
  mkdir ${WORKDIR}
fi

mapfile -t TARLIST < <(ls -t toscom.*.tar.gz)
TARFILE=${TARLIST[0]}

if [ -z "${TARFILE}" ]; then
  echo "no archive file..."
  exit
fi

cp ${TARFILE} ${WORKDIR}
cd ${WORKDIR} || exit

rm -fr toscom
tar xvfz ${TARFILE} >& /dev/null


#!/bin/bash
# build.sh
#
#   Copyright (C) 2017 Bruno Herrera. All rights reserved.
#   Author: Bruno Herrera <bruherrera@gmail.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

set -x

WD=`test -d ${0%/*} && cd ${0%/*}; pwd`
TOPDIR="${WD}/../.."
USAGE="

USAGE: ${0} <board-name>/<config-name>

Where:
  <board-name> is the name of the board in the configs directory
  <config-name> is the name of the board configuration sub-directory

"

while [ ! -z "$1" ]; do
  case "$1" in
    *)
      if [ ! -z "${boardconfig}" ]; then
        echo ""
        echo "<board/config> defined twice"
        echo "$USAGE"
        exit 1
      fi
      boardconfig=$1
      ;;
  esac
  shift
done

# Sanity checking

if [ -z "${boardconfig}" ]; then
  echo ""
  echo "Missing <board/config> argument"
  echo "$USAGE"
  exit 2
fi

if [ -n "$COVERITY" ];
then
  ./tools/ci/coverity.sh ${boardconfig};
  exit $?;
fi

cd ${TOPDIR}
make distclean 1>/dev/null 2>&1
pushd tools
if ! bash ./configure.sh -l ${boardconfig}; then
  printf '%s failed!' "configure ${boardconfig}" >&2
  exit 1
fi
popd

make -j$(nproc)
if test $? -ne 0; then
  printf '%s failed!' "make ${boardconfig}" >&2
  exit 1
fi
#!/bin/bash
#
# This script updates the Bettergram local repository,
# rebuilds the application,
# copies Bettergram executable file with legal documents to release directory
# and makes zip archive bettergram-linux.zip.
#
# This script should be run without sudo.
# We can use it at dockerfile.
#
# Maintainer: Ildar Gilmanov <dev@ddwarf.org>
# See also https://bettergram.io
#

# Params: 
# $1:	Success text
# $2:	Error text
check_result()
{
  res=$?

  if [ $res -eq 0 ]
  then    
    if [ ! -z "$1" ]
    then
      echo ""
      echo "SUCCESS: $1"
      echo ""
    fi
  else
    >&2 echo ""
    >&2 echo "ERROR: Script failed. Return value: '$res'. $2"
    exit 1
  fi
}

echo ""
echo "Trying to enter to bettergram directory..."
echo ""

cd bettergram

check_result "We are in the bettergram directory" "Unable to to enter to bettergram directory"

echo ""
echo "Trying to update local Bettergram git repository..."
echo ""

git pull --all && git submodule update --init --recursive

check_result "Local Bettergram git repository is up to date" "Unable to update Local Bettergram git repository"

echo ""
echo "Trying to rebuild Bettergram application..."
echo ""

rm -rf out/Release \
  && Telegram/gyp/refresh_bettergram.sh \
  && cd out/Release \
  && make $MAKE_THREADS_CNT \
  && strip Bettergram

check_result "Bettergram application is rebuilt" "Unable to rebuild Bettergram application"

echo ""
echo "Trying to create bettergram-linux.zip archive..."
echo ""

cd ../.. \
  && rm -rf ../release/unpacked \
  && mkdir ../release/unpacked \
  && cp out/Release/Bettergram ../release/unpacked/ \
  && cp docs/legal/* ../release/unpacked/ \
  && cd ../release \
  && zip -j bettergram-linux.zip ./unpacked/* \
  && cd ..

check_result "bettergram-linux.zip archive is created" "Unable to create bettergram-linux.zip archive"

exit 0
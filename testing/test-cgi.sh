#!/bin/bash

#
# Simple test script for CGI hacking, requires Python3, only tested on Ubuntu 16.x
#

testdir=/var/tmp/duc-cgi-test
port=8000

if [ `basename $PWD` != "testing" ]; then
    echo "Please run this script from within the testing directory only."
    exit 1
fi

if [ ! -d $testdir ]; then 
  ./rnd-tree.sh $testdir

  cp ../duc ${testdir}/duc
  mkdir ${testdir}/cgi-bin
  cp duc-test.cgi ${testdir}/cgi-bin
  cp footer.txt header.txt index.htm ${testdir}
  
  ${testdir}/duc index -v -d ${testdir}/duc.db $testdir

else
  echo "${testdir} already setup, just do rm -rf to purge and re-run for new setup."
fi

if [ ../duc -nt ${testdir}/duc ]; then
    cp ../duc ${testdir}/duc
    ${testdir}/duc index -v -d ${testdir}/duc.db $testdir
fi

# Serve up on a local port


echo
echo "Goto:  http://localhost:${port}/cgi-bin/duc-test.cgi"
echo 
cd $testdir
python3 -m http.server --cgi ${port}

#!/bin/bash

# Read value from BLE and send them to artemis
if [ "x$HOME" == "x" ]; then
  HOME=/home/pi
  cd $HOME/bt
fi

grep Raspbian /etc/os-release >/dev/null
if [ $? -eq 0 ]; then
  # build and install it in /usr/local/lib
  export LD_LIBRARY_PATH=/usr/local/lib
fi

# the BLE read might fail as the sendmess
failble=0
failartemis=0
while true
do
  # We get files like /tmp/dongle1.txt
  rm -f /tmp/dongle*.txt
  $HOME/bt/bluez_inc/examples/central/central dongle
  if [ $? -ne 0 ]; then
    echo "BLE stuff failed"
    failble=`expr $failble + 1`
  else
    failble=0
  fi
  if [ $failble -eq 0 ]; then
    for file in `ls /tmp/dongle*.txt`
    do
      $HOME/sendfileserver.py $file
      if [ $? -ne 0 ]; then
        echo "Send file  stuff failed"
        failartemis=`expr $failartemis + 1`
      else 
        failartemis=0
      fi
    done
  fi
  # 2 failures in a row, probably something wrong
  if [ $failble -gt 2  -o  $failartemis -gt 1 ]; then
    echo "BLE $failble Artemis $failartemis so failed"
    break 
  fi
done

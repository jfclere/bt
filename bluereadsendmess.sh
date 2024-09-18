#!/bin/bash

# Read value from BLE and send them to artemis

# the BLE read might fail as the sendmess
failble=0
failartemis=0
while true
do
  $HOME/bt/bluez_inc/examples/central/central
  if [ $? -ne 0 ]; then
    echo "BLE stuff failed"
    failble=`expr $failble + 1`
  else
    failble=0
  fi
  if [ $failble -eq 0 ]; then
    $HOME/bt/sendmess /tmp/data.txt
    if [ $? -ne 0 ]; then
      echo "Artemis stuff failed"
      failartemis=`expr $failartemis + 1`
    else 
      failartemis=0
    fi
  fi
  # 2 failures in a row, probably something wrong
  if [ $failble -gt 2  -o  $failartemis -gt 1 ]; then
    echo "BLE $failble Artemis $failartemis so failed"
    break 
  fi
done

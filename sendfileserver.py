#!/usr/bin/python3

import time
import math
import sys
import os
import requests
import socket

from nodeinfo import nodeinfo

#
# This report send a file to the server
#

def main():
  args = sys.argv[1:]
  if len(args) == 0:
    path = "/tmp/dongle1.txt"
    tmp = "/temp.txt"
  else:
    path = args[0]
    s = path.split('/')
    tmp = "/" + s[2]
    
  info = nodeinfo()
  print('server: ' + info.server)
  print('machine_id: ' + info.machine_id)
  if info.read():
    print("Failed no info!")
    exit()
  else:
    print(info.REMOTE_DIR)
    print(info.WAIT_TIME)
    print(info.BAT_LOW)
    print(info.GIT_VER)
    print(info.BATCHARGED)
    print(info.TIME_ACTIVE)

  headers = {'Content-type': 'text/plain'}
  url = "https://" + info.server + "/webdav/" + info.REMOTE_DIR + tmp
  print("sending: ", path, " to: ", url)
  requests.put(url, data=open(path, 'r'), headers=headers, auth=(info.login, info.password))

  print("Done")

if __name__ == "__main__":
  main()

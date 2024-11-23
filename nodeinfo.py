#!/usr/bin/python

import time
import math
import sys
import os
import requests

#
# This class read the info the server has for us
#
# REMOTE_DIR where to store the information in the server
# WAIT_TIME time to wait before restarting
# BAT_LOW low battery (won't start below the value)
# GIT_VER git version of the repo to use (auto update)
# BATCHARGED disconnect charging device once the battery reaches that value
# TIME_ACTIVE time the pump will run

class nodeinfo:
  REMOTE_DIR="pisolar"
  WAIT_TIME=500
  BAT_LOW=500
  GIT_VER="5fe4895"
  BATCHARGED=773
  TIME_ACTIVE=2
  MAINT_MODE=False

  # read the machine_id (/etc/machine-id)
  # read server info ($HOME/.netrc)
  def __init__(self):
    self.machine_id="a470a4070ed946d2ad6b98a9cf130f7b"
    try:
      text_file = open("/etc/machine-id");
      self.machine_id = text_file.readline().rstrip()
      text_file.close()
    except Exception as e:
      print('Exception: ' + str(e))

    self.server="jfclere.myddns.me"
    self.login="jfclere"
    self.password="changeit"
    home_directory = os.path.expanduser( '~' )
    try:
      text_file = open(home_directory + "/.netrc")
      for i in range(3): 
        txt = text_file.readline()
        x = txt.split(" ")
        if x[0] == "machine":
          self.machine=x[1].rstrip()
        if x[0] == "login":
          self.login=x[1].rstrip()
        if x[0] == "password":
          self.password=x[1].rstrip()
      text_file.close()
    except Exception as e:
      print('Exception: ' + str(e))
      return

  # get our configuration from server
  def read(self):
    try:
      r = requests.get('https://' + self.server + '/machines/' + self.machine_id)
      if (r.status_code == 200):
        # Read the information
        i = 0
        for l in r.iter_lines():
          l = l.decode('ascii')
          if i == 0:
            self.REMOTE_DIR=l
          if i == 1:
            self.WAIT_TIME=int(l)
          if i == 2:
            self.BAT_LOW=int(l)
          if i == 3:
            self.GIT_VER=l
          if i == 4:
            self.BATCHARGED=int(l)
          if i == 5:
            self.TIME_ACTIVE=int(l)
          i = i + 1
        return False
      if (r.status_code == 404):
        # 404 means mainteance
        self.MAINT_MODE=True
        return False
      return True
    except Exception as e:
      print('Exception: ' + str(e))
      return True

  # save configuration receive from server
  def saveconf(self):
    try:
      f = open("/home/pi/savedconfig.txt", "w")
      f.write(self.REMOTE_DIR)
      f.write("\n")
      f.write(str(self.WAIT_TIME))
      f.write("\n")
      f.write(str(self.BAT_LOW))
      f.write("\n")
      f.write(self.GIT_VER)
      f.write("\n")
      f.write(str(self.BATCHARGED))
      f.write("\n")
      f.write(str(self.TIME_ACTIVE))
      f.write("\n")
      f.close()
    except Exception as e:
      print('Exception: ' + str(e))
      return True
    return False

  # read save version id
  def readsavedversion(self):
    version=""
    try:
      f = open("/home/pi/savedconfig.txt", "r")
      i = 0
      for line in f:
        if i == 3:
          version=line.rstrip()
          break
        i = i + 1
      f.close()
    except Exception as e:
      print('Exception: ' + str(e))
      return version
    return version

if __name__=='__main__':

  info = nodeinfo()
  print('server: ' + info.server)
  print('machine_id: ' + info.machine_id)
  if info.read():
    print("Failed")
  else:
    print(info.REMOTE_DIR)
    print(info.WAIT_TIME)
    print(info.BAT_LOW)
    print(info.GIT_VER)
    print(info.BATCHARGED)
    print(info.TIME_ACTIVE)
    info.save_conf()
    print("saved version: " + info.read_saved_version())
    print("Success")

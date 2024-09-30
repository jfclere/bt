#!/usr/bin/python3
# 12 blue : blink once we have a dongle connected
# 22 green : network is running
# 27 red : blink once we send something to artemis

import RPi.GPIO as GPIO
import os
import requests
import time
import sys

BLUE=12
GREEN=22
RED=27
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
GPIO.setup(BLUE,GPIO.OUT)
GPIO.setup(GREEN,GPIO.OUT)
GPIO.setup(RED,GPIO.OUT)

# toogle the led we received
valled="RED"
args = sys.argv[1:]
if len(args) == 1:
  valled=sys.argv[1]


LED = RED
if valled == "blue":
  LED = BLUE
if valled == "green":
  LED = GREEN

if valled == "off":
  # switch off all leds
  GPIO.output(RED, GPIO.LOW)
  GPIO.output(BLUE, GPIO.LOW)
  GPIO.output(GREEN, GPIO.LOW)
else:
  val = GPIO.input(LED)
  if val == GPIO.HIGH:
    val = GPIO.LOW
  else:
    val = GPIO.HIGH
  GPIO.output(LED, val)

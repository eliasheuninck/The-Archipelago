#!/usr/bin/env python3

# The Archipelago - Code for Raspberry pi

from smbus2 import SMBus # I2C communication
from omxplayer.player import OMXPlayer # video player
from pathlib import Path
from time import sleep
import subprocess

import signal
import sys

address = 0x08

def signal_term_handler(signal, frame):
	print("SIGTERM received")
	bus.close()
	sleep(0.01)
	sys.exit(0)


with SMBus(1) as bus:
	bus.close()


# The Archipelago video
VIDEO_PATH = Path("/home/pi/Desktop/archipelago/WO_03_v2_LCR.mp4") # an absolute path is necesary
 
# Test video
# VIDEO_PATH = Path("/home/pi/Documents/surround_test/test8.mp4") # an absolute path is necesary

video_pause = 0 # Wait an additional amount of time (seconds) before the video loops
				# to give time for the motors to reset
				# or a longer break to make sure people don't go insane


#  Autostart exit information
print("--------------------------------------------")
print("Starting video The Archipelago in 20 seconds")
print("--------------------------------------------")
print("PRESS F11 TO STOP PYTHON SCRIPT (6 times)")
print("PRESS F12 TO STOP OMXPLAYER")
print("--------------------------------------------")
print("Right-click outside of this window")
print("Select 'Terminal Emulator'")
print("in ~, type: ./auto.py to toggle renaming the autostart file")
print("--------------------------------------------")
print("~/Desktop/autorun/run.py")
print("~/.config/lxsession/LXDE-pi/autostart")
print("--------------------------------------------")

sleep(0.1)

# Open a new terminal window so keyboard commands can be executed
subprocess.call('lxterminal')

sleep(0.1)

# show how much time is left before omxplayers starts
for x in range(1, 0, -1):
	print("{:02d}".format(x), " sec", end = '\r')
	sleep(1)

# Start omxplayer
player = OMXPlayer(VIDEO_PATH, args='--fps 25 --no-osd -o alsa --layout 3.0 --blank=0xaa8899ff')

# useful arguments information:
# --loop
# -b = black screen  or  --blank = black screen
#		--blank=0xAARRGGBB
#		--blank=0xff000000 (opaque black)
#		--blank=0xaa8899ff (semi transparent blue)
# -o alsa = use alsa as audio output (not the built in jack, or HDMI)
#     o alsa:hw:2,0   doesn't work
# -I = show stream info before playback
# --no-osd = no on screen interface-text
# --layout 5.1 = set output speaker layout (supported options: 2.0, 2.1, 3.0, 3.1, 4.0, 4.1, 5.0, 5.1, 7.0, 7.1)
# --no-boost-on-downmix = doesn't seem to work
# --win 1000,50,1640,410    (this seems to crash the raspberry pi regularly)

# Debug info
# print("player: " + str(player))
# print("List audio streams: " + str(player.list_audio()))
# print("current player volume: " + str(player.volume()))
# print("metadata: " + str(player.metadata()))


# Get the total video duration 
duration = player.duration() # duration in seconds (float)
duration = int(float(duration * 1000)) # convert to milliseconds (int)
print("duration in ms: ", duration)

player.pause()
sleep(0.5)
player.play()

while True: # GET PLAYHEAD TIME
	signal.signal(signal.SIGTERM, signal_term_handler)

	# Make a query to position() inside an infinite loop
	position_raw = int(float(player.position() * 1000)) # convert seconds to milliseconds
	
	# Ignore any negative values
	if position_raw >= 0:
		position = position_raw
   
    # print position (for debugging)
	print("pos: ", position)

	# Manual looping
	if position >= (duration - 300): # subtract a few milliseconds from the total duration for stability
		player.pause()
		sleep(0.3)
		sleep(video_pause)
		player.set_position(1) # set the playhead to 1 second into the video
		sleep(0.5)
		player.play()

	# send the position to the arduino
	with SMBus(1) as bus:
		data = [int(i) for i in str(position)] # cast the number to a string and iterate it into an array
		try:
			bus.write_i2c_block_data(address, 0, data) # send the data on I2C address 8, with an offset of 0
		except:
			print("bus write failed. closing and waiting")
			bus.close()
			sleep(0.02)


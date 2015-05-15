GPS GUI application for [OpenMoko](http://wiki.openmoko.org) mobile phone.

# Features #

  1. Map:
    * maps are configured with python script, thus can be easily modified by distributors or end users.
    * batch downloads
    * ruler and lat/lon grid
    * map overlay
    * in memory tile cache
  1. GPS:
    * U-BLOX binary protocol is used by default, efficient. Automatically switch to ogpsd upon conflict.
    * U-BLOX AGPS online, which cuts TTFF down to about 10s.
    * dump aiding data on close and after AGPS online, feed these data to GPS receiver on start.
    * sync GPS time to system and hardware clock.
    * auto-suspend is disabled, also you can reset GPS or configure "dynamic platform model" with "CFG-NAV2"
  1. Track recording and replay. Export track records as .gpx file.
  1. Scratch on map, and save as screen shot.
  1. Basic sounding for reporting speed and events (low power and weak signal). Config script is written in Python, for ease of extending.

# NOTE #

  1. only supports downloading via HTTP, without proxy. CURL was ever used but was dropped due to various problems.
  1. should run on any GTA01 or GTA02 with FSO framework.
  1. sound files are installed into /usr/share/sounds/omgps/
  1. data dir is $HOME/.omgps/. This directory and related subdirectories/files are created after first run. You can modify/add maps in file $HOME/.omgps/config\_VERSION/map.py
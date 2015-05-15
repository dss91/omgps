# 0.1\_svnr109 (20090719) #

  * bug fix: should load ALM even if EPH was loaded
  * bug fix: in main view, when full/unfull screen, heading redraw fails

# 0.1\_svnr105 (20090719) #

  * bug fix: sometimes unable to restore GPS from suspend
  * bug fix: sometimes block on GPS initializing
  * bug fix: incorrect file permission, should set with open(), don't use umask()
  * bug fix: [issue 11](https://code.google.com/p/omgps/issues/detail?id=11): incorrect satellite position in skymap, thanks to LudwigBrinckmann
  * defect: remove omgps.pid at exit
  * defect: [issue 9](https://code.google.com/p/omgps/issues/detail?id=9), don't popup warning dialog while no sound module found
  * defect: [issue 10](https://code.google.com/p/omgps/issues/detail?id=10), change font color for "dynamic platform model"
  * enhancement: faster GPS initializing

# 0.1\_svnr90 (20090625) #

  * bugfix: [bug #6](https://code.google.com/p/omgps/issues/detail?id=6)
  * minor UI update
  * explicitly enable SBAS again
  * source: can be built with openembedded.org bitbake (tested under SHR-unstable bb repository)

# 0.1\_20090618-3 #

  * bugfix: sometimes widgets does not display as expected: (1) start/stop gps button text (2) and gpsfix tab on main view
  * enhancement: make sure all GUI operations within non-main threads end with gdk\_flush()

## known issues: ##
  * "toggled state" of toggle button does not well displayed due to SHR theme.
  * [bug #6](https://code.google.com/p/omgps/issues/detail?id=6)

# 0.1\_20090618-2 #

  * bugfix: sound module does not read speed correctly when speed unit is not m/s
  * defect: theme related (on latest SHR):
    1. UI flush from non-main threads is extreme slow, force gdk\_flush().
    1. unable to set "gtk-font-name" by gtk\_rc\_parse\_string() any more, instead use gtk\_settings\_set\_string\_property(); slightly adjust UI layouts by adding scrolled window, etc.
    1. "keep cursor in view" mode, button font color (gray) is not suitable on black bg.
    1. fg/bg of scratch color buttons and center button broken, adjust and rewrite.

## known issues: ##
  * [bug #6](https://code.google.com/p/omgps/issues/detail?id=6)
  * occasionally gpsfix tab does not show on main view -- you have to restart
  * "toggled state" of toggle button does not well displayed due to SHR theme.

# 0.1\_20090618-1 #

  * bugfix: gpsfix tab does not show up
  * bugfix: too much records inserted into track file list while tracking
  * defect: track strange sorting, http://code.google.com/p/omgps/issues/detail?id=5&can=1
  * defect: scratch sorting

# 0.1\_20090529-2 #

  * bug fix: track replay, resume does not work

# 0.1\_20090529-1 #

  * bug fix: malformated .gpx file format
  * bug fix: choose lat/lon position on main view by clicking does not work well
  * defect: "keep cursor in view", refresh view before cursor moves out of view (1/4 width/height)
  * defect: "sky map", clean up UI, better support 640 x 480 -- by scaling background image.
  * new feature: configure "dynamic platform model" with "CFG-NAV2", you can choose from Pedestrian or Automotive, etc.
  * eye-friendly things: change text color; clear bottom label on main view by clicking.

# 0.1\_20090527-1 #

  * bug fix: "keep cursor in view" does not always behave as expected.
  * bug fix: full screen mode, popup message dialogs does not show on above
  * bug fix: SIGSEGV on stopping track
  * defect: add speed unit of km/h and mph, in addition to m/s
  * update UI, including sky map.
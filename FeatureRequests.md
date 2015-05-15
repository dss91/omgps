Collected from openmoko mailing list for v0.2 and later.

# Initial mail #

First, I must appreciate all omgps users, for your test and feed back.

Because version 0.1 gets stable for now, and it will be integrated into official openembeded repository, I'm planing start next major developing stage. Here is current plan:

1. better supports for track logging, nuk ask me to add altitude to track log, good point. I can remember complains about the lack of POI, and suggestion of voice recording to help post precess of track logging.

2. support dynamic layers, including POI, openbmap data, etc. my concerns are (1) performance (2) is it possible or necessary to support user defined layers as plugins?

3. and other big things related to routing.

Requirement of version 0.2 will be frozen due 07-07, the core task is to make it track-friendly for OSM map and JOMS application. Please feel free to comment or add new feature requests here.

mqy, 2009-06-30

# Replies summary #

  * OSM:
    1. kimaidou:
      * Combining the map navigation of omgps and the easy editing of osm tracker.
      * have a layer "osm poi" above the "slippy mapnkik map".
      * ref: Mokomapper (http://wiki.openmoko.org/wiki/Mokomapper)
      * ref: OSMtracker (http://wiki.openstreetmap.org/wiki/OSMtracker)
      * ref: osm2go (http://comiles.eu/~natanael/projects/, http://wiki.openstreetmap.org/wiki/OSM2Go)
    1. Risto H. Kurppa:
      * save POI's or something to load on JOSM to add it to OSM.
      * Big buttons to mark 'paved road starts', '80km/h speed limit starts', 'crossroads', 'bridge' etc.
    1. Davide:
      * upload data to OSM
    1. onen:
      * edit GPX track, cut private or garbage parts.
    1. Helge Hafting:
      * GPX comments
  * track:
    1. nuk (from opkg.org comment):
      * add altitude to track log
    1. jeremy jozwik:
      * configure length of track path history.
    1. ivvmm:
      * make track replay rewindable
    1. Risto H. Kurppa:
      * change the frequency of saving the location
      * analyze GPS track on-the-fly: show the speed vs. time plot.
    1. Christian Rüb:
      * split a track from the main screen. This makes it easier for later OSM editing.
  * UI:
    1. jeremy jozwik:
      * bigger buttons
  * Navigation mode:
    1. David Samblas Martinez:
      * "what's ahead" mode.
  * vector maps:
    1. Patryk Benderz:
      * less disk space,
      * ref: http://downloads.cloudmade.com/
    1. Marcel-2:
      * should avoid low render speed
    1. rusolis:
      * use the navit engine (for example) for routing using the vector data from OSM, and then display that on top of the png maps in omgps.
  * misc:
    1. Laszlo: archived repository
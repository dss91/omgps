############ resources ######################

confdir = $(sysconfdir)/$(PACKAGE)/$(VERSION)
dist_conf_DATA = data/etc/*.*

EXTRA_DIST = $(dist_conf_DATA)

desktopdir = $(datadir)/applications
dist_desktop_DATA = data/$(PACKAGE).desktop

appicondir = $(datadir)/pixmaps/$(PACKAGE)
dist_appicon_DATA = data/$(PACKAGE).png data/sky.png

############ source files ###################

#
# FIXME: detect version automatically
#

python_version = 2.6
python_include = $(LIBTOOL_SYSROOT_PATH)/usr/include/python$(python_version)/
python_link = python$(python_version)

bin_PROGRAMS = omgps omgps_debug

common_CFLAGS = -Wall -Werror       \ 
  -L$(LIBTOOL_SYSROOT_PATH)/usr/lib	\
  -I$(top_srcdir)/src       		\
  -I$(top_srcdir)/src/include  		\
  -I$(python_include)               \
  -DETCCONF_DIR=\"$(confdir)\"		\
  -DICONDIR=\"$(appicondir)\"       \
  -D_GNU_SOURCE                     \
  @DEPENDENCIES_CFLAGS@

omgps_CFLAGS = $(common_CFLAGS) -O2
omgps_LDADD = @DEPENDENCIES_LIBS@ -l$(python_link)

omgps_SOURCES =          \
  src/ctx_agps_online.c  \
  src/ctx_fixmap.c       \
  src/ctx_dl_tiles.c     \
  src/ctx_track_replay.c \
  src/cr_pixbuf.c        \
  src/customized.c       \
  src/dbus_intf.c        \
  src/gc_resource.c      \
  src/globals.c          \
  src/main.c             \ 
  src/map_repo.c         \
  src/map_meter.c        \
  src/mouse.c            \
  src/network.c          \
  src/poll.c             \
  src/poll_ui.c	         \
  src/py_ext.c	         \
  src/settings.c         \
  src/sound.c            \
  src/tab_gpscfg.c       \
  src/tab_menu.c         \
  src/tab_nav.c	         \
  src/tab_scratch.c      \
  src/tab_sound.c        \
  src/tab_tile.c         \
  src/tab_track.c        \
  src/tab_view.c         \
  src/tile_dl.c          \
  src/tile_cache.c       \
  src/ubx.c              \
  src/util.c             \
  src/usart.c            \
  src/wgs84.c            \
  src/xpm_image.c

omgps_debug_CFLAGS = $(common_CFLAGS) -g
omgps_debug_SOURCES = $(omgps_SOURCES)  
omgps_debug_LDADD = $(omgps_LDADD)

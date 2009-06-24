DESCRIPTION = "GPS application for openmoko freerunner"
HOMEPAGE = "http://omgps.googlecode.com"
SECTION = "openmoko/applications"
LICENSE = "GPLv2"
DEPENDS = "gtk+ python-pygobject dbus-glib"
PRIORITY = "optional"
PACKAGES = "${PN}-dbg ${PN}"
PV = "0.1+svnr${SRCREV}"
S = "${WORKDIR}/${PN}"
SRC_URI = "svn://omgps.googlecode.com/svn/trunk/;module=omgps;proto=http"

inherit autotools
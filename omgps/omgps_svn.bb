DESCRIPTION = "GPS application for openmoko freerunner"
HOMEPAGE = "http://omgps.googlecode.com"
SECTION = "openmoko/applications"
LICENSE = "GPLv2"
DEPENDS = "gtk+ python-pygobject dbus-glib"
PACKAGES = "${PN}-dbg ${PN}"
SRCREV = "${AUTOREV}"
PV = "0.1"
PR = "svnr${SRCREV}"
S = "${WORKDIR}/trunk/${PN}"
SRC_URI = "svn://omgps.googlecode.com/svn/;module=trunk;proto=http"

inherit autotools
DESCRIPTION = "GPS application for openmoko freerunner"
HOMEPAGE = "http://omgps.googlecode.com"
SECTION = "openmoko/applications"
LICENSE = "GPLv2"
DEPENDS = "gtk+ python-pygobject dbus-glib"
PRIORITY = "optional"
PACKAGES = "${PN}-dbg ${PN}"
S = "${WORKDIR}/${P}"
PR = "r0"
SRC_URI = "http://omgps.googlecode.com/files/${PF}.tar.gz"

inherit autotools
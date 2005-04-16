#!/bin/sh
#
# This file was automatically customized by debmake on Thu, 28 Aug 2003 12:10:11 +0000
#
# Written by Miquel van Smoorenburg <miquels@cistron.nl>.
# Modified for Debian GNU/Linux by Ian Murdock <imurdock@gnu.org>.
# Modified for Debian by Christoph Lameter <clameter@debian.org>

PATH=/bin:/usr/bin:/sbin:/usr/sbin
DAEMON=/usr/bin/irC6
# The following value is extracted by debstd to figure out how to generate
# the postinst script. Edit the field to change the way the script is
# registered through update-rc.d (see the manpage for update-rc.d!)
FLAGS="defaults 99"
CHROOT=""
EXTRA_ARGS=""
UNAME="nobody"

test -f $DAEMON || exit 0


_START_ARGS="--start -v --exec $DAEMON"
_STOP_ARGS="--stop -v --exec $DAEMON"

if test `whoami` = root; then
	_START_ARGS="$_START_ARGS --chuid $UNAME"

	test -d "$CHROOT" && _START_ARGS="$_START_ARGS --chroot $CHROOT"
fi
_START_ARGS="$_START_ARGS -- -d $EXTRA_ARGS"

case "$1" in
  start)
    start-stop-daemon $_START_ARGS
    ;;
  stop)
    start-stop-daemon $_STOP_ARGS
    ;;
  restart)
    start-stop-daemon $_STOP_ARGS
    sleep 1
    start-stop-daemon $_START_ARGS
    ;;
  *)
    echo "Usage: /etc/init.d/irci6 {start|stop|restart}"
    exit 1
    ;;
esac

exit 0

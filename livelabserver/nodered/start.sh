#!/bin/bash

rm -rf /var/run/avahi-daemon
rm -rf /var/run/dbus
mkdir -p /var/run/dbus
dbus-daemon --system
avahi-daemon --no-chroot -D
npm start -- --userDir /data
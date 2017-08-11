# Logi

The latest incarnation of my attempt to write a PVR with a decent UI.

## Status

At the moment it does very little except scan for terrestrial channels but,
unlike most other software, it has full support for the UK Freeview service. It
compromises between scanning the entire spectrum and using a set of
user-supplied tuning data, by starting a full scan but aiming to detect when
it's gathered data for all channels and stopping early.

To make it useful to other people at such an early stage I've included a script
which converts the database to channels.conf for VDR, or outputs a MySQL script
for setting the channels numbers in MythTV's database.

Logi uses sqlite3, the database's default location is
`${XDG_DATA_HOME:-~/.local/share}/logi/database.sqlite`). The table structure
is not finalised yet, so for now you should delete any existing database
whenever you update the source code.

## Roadmap

In expected order of implementation:

* Freesat scanning (much faster than existing scanners)

* Daemon with web interface

* Modern, responsive web UI

* EIT harvesting for EPG

* Multiple ways of viewing EPG in a browser

* Scheduled recordings

* Stream in a format compatible with browsers/mainstream video players

* Kodi plugin.

## Licence

GPL3. The standard `COPYING` file is included.

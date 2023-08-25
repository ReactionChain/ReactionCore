
Debian
====================
This directory contains files used to package reactiond/reaction-qt
for Debian-based Linux systems. If you compile reactiond/reaction-qt yourself, there are some useful files here.

## reaction: URI support ##


reaction-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install reaction-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your reaction-qt binary to `/usr/bin`
and the `../../share/pixmaps/reaction128.png` to `/usr/share/pixmaps`

reaction-qt.protocol (KDE)


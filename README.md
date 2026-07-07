# IMPORTANT: Linux rdesktop is no longer maintained

Please, beware that the parent project is no longer maintained and hadn't received
a patch in many years. There are reported security vulnerabilities, yet those
haven't been reviewed.

Use with caution and use it at your own risk.

# rdesktop - A Remote Desktop Protocol client

rdesktop is an open source client for Microsoft's RDP protocol. It is
known to work with Windows versions ranging from NT 4 Terminal Server
to Windows 2012 R2 RDS. rdesktop currently has implemented the RDP version 4
and 5 protocols.


## Installation

rdesktop uses a GNU-style build procedure.  Typically all that is necessary
to install rdesktop is the following:

	% ./configure
	% make
	% make install

The default is to install under `/usr/local`.  This can be changed by adding
`--prefix=<directory>` to the configure line.

The smart-card support module uses PCSC-lite. You should use PCSC-lite 1.2.9 or
later. To enable smart-card support in the rdesktop add `--enable-smartcard` to
the configure line.

## Usage (command line)

Connect to an RDP server with:

	% rdesktop server

where `server` is the name of the Terminal Services machine. If you receive
"Connection refused", this probably means that the server does not have
Terminal Services enabled, or there is a firewall blocking access.

You can also specify a number of options on the command line.  These are listed
in the rdesktop manual page (run `man rdesktop`).

## Usage macOS - Invoke the rdesktop-mac.app Application Bundle

---- Update for rdesktop-mac port ---
SVG Icon thanks to Remote Desktop SVG Vector
https://www.svgrepo.com/svg/366419/remote-desktop

Free Download Remote Desktop SVG vector file in monocolor and multicolor type for Sketch and Figma from Remote Desktop Vectors svg vector collection. Remote Desktop Vectors SVG vector illustration graphic art design format.

    COLLECTION: Puppylinux Interface Icons
    LICENSE: PD License
    AUTHOR: puppylinux


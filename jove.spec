# Redhat Package Manager .spec file for JOVE
# To create SRPM and RPM, copy distribution to /usr/src/redhat/SOURCES
# and execute: rpm -ba jove.spec
# This .spec file has the JOVE version number embedded in it several
# places so it needs to be adjusted every time the version changes!
# The SYSDEFS setting specified requires the system to suport UNIX98 PTYs.

Summary: Jonathan's Own Version of Emacs
Name: jove
Version: 4.16.0.48
Release: 1
Copyright: Copyright (C) 1986-1999 by Jonathan Payne, freely redistributable
Packager: jovehacks@cs.toronto.edu
Group: Applications/Editors/Emacs
Source: ftp://ftp.cs.toronto.edu/pub/hugh/jove-dev/4.16.0.48.tgz
%description

Jove is a compact, powerful Emacs-style text-editor. It provides the common
emacs keyboard bindings, together with a reasonable assortment of the most
popular advanced features (e.g. interactive shell windows, compile-it,
language specific modes) while weighing in with CPU, memory, and disk
requirements comparable to vi(1).

%prep
%setup -c

%build
make SYSDEFS="-DSYSVR4 -D_XOPEN_SOURCE=500" JOVEHOME=/usr PORTSRVINST=""
make SYSDEFS="-DSYSVR4 -D_XOPEN_SOURCE=500" JOVEHOME=/usr PORTSRVINST="" doc/jove.man
make SYSDEFS="-DSYSVR4 -D_XOPEN_SOURCE=500" JOVEHOME=/usr PORTSRVINST="" TROFF=groff TROFFPOST=">jove.man.ps" troff-man

%install
make SYSDEFS="-DSYSVR4 -D_XOPEN_SOURCE=500" JOVEHOME=/usr PORTSRVINST="" install

%files
/usr/lib/jove/
/usr/bin/jove
/usr/bin/teachjove
%doc /usr/man/man1/jove.1
%doc /usr/man/man1/teachjove.1
#%doc /usr/man/man1/xjove.1
%doc README doc/jove.man doc/jove.man.ps doc/jove.rc doc/example.rc doc/jove.qref

%changelog
* Wed Jul 12 2000 <hugh@mimosa.com>
  - 4.26.0.48: switched to using UNIX98 PTYs for security
* Wed Aug 18 1999 <hugh@mimosa.com>
  - Updated in prepartation for new release
* Tue Jul 28 1998 <markster@marko.net>
  - Updated to version 4.16.0.28
* Wed May 12 1997 <bet@fcmc.com>
  - Initial wrap


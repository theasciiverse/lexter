Name: lexter
Summary: A real-time word puzzle for text terminals
Version: 1.0.3
Release: 1
Source: http://www.kyne.com.au/~mark/%{name}-%{version}.tar.gz
Url: http://www.kyne.com.au/~mark/software.html
Group: Amusements/Games
Copyright: GPL
Packager: Mark Pulford <mark@kyne.com.au>
BuildRoot: /var/tmp/%{name}-root

%description
Lexter is a real-time word puzzle for text terminals. Arrange the falling
letters into words to score points. Lexter supports internationalization and
multiple dictionaries. This package contains English and French dictionaries,
but is missing a French gettext translation.

%prep
%setup

%build
./configure --prefix=/usr --bindir=/usr/games --datadir=/usr/share/games --localstatedir=/var/lib/games
make

%install
make install DESTDIR=$RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/var/lib/games
touch $RPM_BUILD_ROOT/var/lib/games/lexter.scores

%post
if [ ! -f /var/lib/games/lexter.scores ]
then
	touch /var/lib/games/lexter.scores
	chown games.games /var/lib/games/lexter.scores
	chmod 664 /var/lib/games/lexter.scores
fi

%clean
rm -rf $RPM_BUILD_ROOT

%files
%doc ABOUT-NLS AUTHORS COPYING NEWS README THANKS
%attr(2755,games,games) /usr/games/lexter
%doc /usr/man/man?/*
/usr/share/games/lexter

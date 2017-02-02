%define name libgmonopd
%define version 0.3.0
%define release 1

Summary: libgmonopd
Name: %{name}
Version: %{version}
Release: %{release}
License: GPL
Group: Development/Libraries
Source: %{name}-%{version}.tar.bz2
URL: http://sourceforge.net/projects/libgmonopd
Buildroot: %{_tmppath}/%{name}-buildroot

%description
libgmonopd is a library for embedding boardgame servers into monopd-compatible
clients.  It is protocol-compatible with monopd.

%prep
rm -rf $RPM_BULID_ROOT

%setup

%build

%configure

%make

%install
%makeinstall

%post

%postun

%clean
rm -rf $RPM_BULID_ROOT

%files
%defattr(-,root,root)
%doc AUTHORS COPYING NEWS README ChangeLog
/usr/include/*
/usr/lib/*

%changelog
* Sun Oct 28 2001 Erik Bourget <ebourg@po-box.mcgill.ca> 0.3.0
- made an rpm

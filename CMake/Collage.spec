Name:		Collage
Version:	1.1.2
Release:	1%{?dist}
Summary:	Cross-platform C++ network library

Group:		Development/Libraries
License:	LGPLv2
URL:		http://www.libcollage.net/
Source0:	http://www.equalizergraphics.com/downloads/%{name}-%{version}.tar.gz
Patch0:		Collage-1.1.2-build-fix.patch
BuildRequires:	cmake
BuildRequires:	boost-devel glew-devel lunchbox1

%description
Collage is a cross-platform C++ library for building heterogenous,
distributed applications. It is used as the cluster backend for the
Equalizer parallel rendering framework. Collage provides an abstraction
of different network connections, peer-to-peer messaging, discovery and
synchronization as well as high-performance, object-oriented, versioned
data distribution. Collage is designed for low-overhead multi-threaded
execution which allows applications to easily exploit multi-core
architectures.

%package devel
Summary:	Development files for Collage
Group:		Development/Libraries
Requires:	%{name} = %{version}-%{release}

%description devel
Development files for Collage.

%prep
%setup -q
%patch0 -p1 -b .build-fix

%build
%cmake

make %{?_smp_mflags}


%install
make install DESTDIR=%{buildroot}
mv %{buildroot}%{_datadir}/%{name}/doc _tmpdoc/


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%doc _tmpdoc/*
%{_bindir}/*
%{_libdir}/lib*.so.*
%{_datadir}/%{name}

%files devel
%{_includedir}/*
%{_libdir}/lib*.so
%{_libdir}/pkgconfig/*.pc

%changelog
* Mon Sep 19 2011 Richard Shaw <hobbes1069@gmail.com> - 1.0.1-1
- Initial Release

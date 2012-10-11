Summary: @SUMMARY@
Name: glite-lbjp-common-maildir
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: ASL 2.0
Vendor: EMI
Group: System Environment/Libraries
BuildRequires: chrpath
BuildRequires: libtool
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lbjp-common.maildir/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
@DESCRIPTION@


%package devel
Summary: Development files for gLite L&B/JP common maildir library
Group: Development/Libraries
Requires: %{name}%{?_isa} = %{version}-%{release}


%description devel
This package contains development libraries and header files for gLite L&B/JP
common maildir library.


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lbjp-common.maildir
make


%check
make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*.la' -exec rm -rf {} \;
find $RPM_BUILD_ROOT -name '*.a' -exec rm -rf {} \;
find $RPM_BUILD_ROOT -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'


%clean
rm -rf $RPM_BUILD_ROOT


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%defattr(-,root,root)
/usr/%{_lib}/libglite_lbu_maildir.so.@MAJOR@.@MINOR@.@REVISION@
/usr/%{_lib}/libglite_lbu_maildir.so.@MAJOR@


%files devel
%defattr(-,root,root)
%dir /usr/include/glite
%dir /usr/include/glite/lbu
/usr/include/glite/lbu/maildir.h
/usr/%{_lib}/libglite_lbu_maildir.so


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package

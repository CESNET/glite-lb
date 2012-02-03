Summary: @SUMMARY@
Name: glite-lb-types
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: ASL 2.0
Vendor: EMI
Group: Development/Tools
BuildArch: noarch
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.types/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lb.types
make


%check
make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%dir /usr/share/glite-lb/at3/
%dir /usr/share/perl/
%dir /usr/share/perl/gLite/
%dir /usr/share/perl/gLite/LB/
/usr/sbin/glite-lb-at3
/usr/sbin/glite-lb-check_version.pl
/usr/share/perl/gLite/LB/StructField.pm
/usr/share/perl/gLite/LB/MultiStruct.pm
/usr/share/glite-lb/at3/events.T
/usr/share/glite-lb/at3/status.T
/usr/share/glite-lb/at3/types.T


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package

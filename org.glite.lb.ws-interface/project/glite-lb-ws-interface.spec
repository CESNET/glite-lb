Summary: @SUMMARY@
Name: glite-lb-ws-interface
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: ASL 2.0
Vendor: EMI
Group: Development/Libraries
BuildArch: noarch
BuildRequires: glite-lb-types
BuildRequires: libxslt
BuildRequires: tidy
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.ws-interface/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lb.ws-interface
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
%dir /usr/include/glite/
%dir /usr/include/glite/lb/
%dir /usr/share/doc/%{name}-%{version}/
%dir /usr/share/wsdl/
%dir /usr/share/wsdl/glite-lb/
/usr/include/glite/lb/ws_interface_version.h
/usr/share/doc/%{name}-%{version}/LICENSE
/usr/share/doc/%{name}-%{version}/ChangeLog
/usr/share/doc/%{name}-%{version}/package.summary
/usr/share/doc/%{name}-%{version}/package.description
/usr/share/wsdl/glite-lb/LB.wsdl
/usr/share/wsdl/glite-lb/glue2.xsd
/usr/share/wsdl/glite-lb/lb4agu.wsdl
/usr/share/wsdl/glite-lb/LBTypes.wsdl


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package

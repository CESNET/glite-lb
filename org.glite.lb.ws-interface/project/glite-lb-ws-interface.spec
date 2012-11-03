Name:           glite-lb-ws-interface
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          Development/Libraries
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.ws-interface/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildArch:      noarch
BuildRequires:  glite-lb-types
BuildRequires:  libxslt
BuildRequires:  tidy

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
%doc LICENSE project/ChangeLog
%dir %{_includedir}/glite/
%dir %{_includedir}/glite/lb/
%dir /usr/share/wsdl/
%dir /usr/share/wsdl/glite-lb/
%{_includedir}/glite/lb/ws_interface_version.h
/usr/share/wsdl/glite-lb/LB.wsdl
/usr/share/wsdl/glite-lb/glue2.xsd
/usr/share/wsdl/glite-lb/lb4agu.wsdl
/usr/share/wsdl/glite-lb/LBTypes.wsdl


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package

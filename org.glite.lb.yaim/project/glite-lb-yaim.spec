Summary: @SUMMARY@
Name: glite-lb-yaim
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: ASL 2.0
Vendor: EMI
Group: Development/Tools
BuildArch: noarch
Requires: glite-yaim-bdii
Requires: perl-LDAP
Requires: glite-yaim-core
Obsoletes: glite-yaim-lb <= 4.2.1-1
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/@MODULE@/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lb.yaim
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
%dir /opt/glite/
%dir /opt/glite/release/
%dir /opt/glite/release/glite-LB/
%dir /opt/glite/yaim/
%dir /opt/glite/yaim/defaults/
%dir /opt/glite/yaim/etc/
%dir /opt/glite/yaim/etc/versions/
%dir /opt/glite/yaim/functions/
%dir /opt/glite/yaim/node-info.d/
/opt/glite/release/glite-LB/COPYRIGHT
/opt/glite/release/glite-LB/LICENSE
/opt/glite/release/glite-LB/arch
/opt/glite/release/glite-LB/node-version
/opt/glite/release/glite-LB/service
/opt/glite/release/glite-LB/update
/opt/glite/yaim/etc/versions/glite-lb-yaim
/opt/glite/yaim/defaults/glite-lb.pre
/opt/glite/yaim/functions/config_gip_lb
/opt/glite/yaim/functions/config_glite_lb
/opt/glite/yaim/functions/config_info_service_lb
/opt/glite/yaim/node-info.d/glite-lb


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package

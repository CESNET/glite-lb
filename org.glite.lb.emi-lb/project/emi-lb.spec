Summary: @SUMMARY@
Name: emi-lb
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: ASL 2.0
Vendor: EMI
Group: System Environment/Base
Requires: bdii
Requires: emi-version
Requires: fetch-crl
%if ! 0%{?fedora}
Requires: glite-lb-client-java
%endif
Requires: glite-lb-client-progs
Requires: glite-lb-doc
Requires: glite-lb-harvester
Requires: glite-lb-logger
Requires: glite-lb-logger-msg
Requires: glite-lb-server
Requires: glite-lb-utils
Requires: glite-lb-ws-test
Requires: glite-lb-yaim
#Requires: glue-service-provider
Requires: glite-info-provider-service
Requires: glue-schema
Obsoletes: glite-LB <= 3.3.3-3
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.emi-lb/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
@DESCRIPTION@


%prep
%setup -q


%build


%check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package

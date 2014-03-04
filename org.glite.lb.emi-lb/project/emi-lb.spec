# virtual package
%global debug_package %{nil}

Name:           emi-lb
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Base
License:        ASL 2.0
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.emi-lb/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

Requires:       bdii
Requires:       emi-version
Requires:       fetch-crl
Requires:       glite-lb-client-progs
Requires:       glite-lb-doc
Requires:       glite-lb-harvester
Requires:       glite-lb-logger
Requires:       glite-lb-logger-msg
Requires:       glite-lb-server
Requires:       glite-lb-utils
Requires:       glite-lb-ws-test
Requires:       glite-lb-yaim
#Requires: glue-service-provider
Requires:       glite-info-provider-service
Requires:       glue-schema
Obsoletes:      glite-LB <= 3.3.3-3

%description
@DESCRIPTION@


%prep
%setup -q


%build


%check


%install
rm -rf $RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package

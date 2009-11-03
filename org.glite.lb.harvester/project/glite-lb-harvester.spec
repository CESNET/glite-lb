Summary: Enhanced L&B notification client.
Name: glite-lb-harvester
Version: 1.0
Release: 1
License: Unknown
Group: Unknown
Source: harvester.tar.gz
URL: http://lindir.ics.muni.cz/dg_public/cvsweb.cgi/rtm/
Vendor: CESNET
Packager: František Dvořák <valtri@civ.zcu.cz>
Prefix: /opt/glite
BuildRoot: /tmp/rpm-harvester-root
Requires: postgresql-libs glite-jobid-api-c glite-lbjp-common-trio glite-lb-common glite-lb-client
BuildRequires: postgresql-devel

%description
L&B Harvester gathers information about jobs from L&B servers using effective L&B notification mechanism. It manages notifications and keeps them in a persistent storage (file or database table) to reuse later on next launch. It takes care about refreshing notifications and queries L&B servers back when some notification expires.

The tool was initially written for Real Time Monitor (project at Imperial College in London), later was extended with messaging mechanism for WLCG.

%prep
%setup -n harvester

%build
make PREFIX=%{prefix}

%install
make install PREFIX="${RPM_BUILD_ROOT}%{prefix}"

%clean
[ -n "${RPM_BUILD_ROOT}" -a "${RPM_BUILD_ROOT}" != "/" ] && rm -rf "${RPM_BUILD_ROOT}"

%files
%dir %{prefix}/doc/glite-lb-harvester
%{prefix}/doc/glite-lb-harvester/README
%{prefix}/bin/glite-lb-harvester

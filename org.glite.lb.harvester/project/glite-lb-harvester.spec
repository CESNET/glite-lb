Name:           glite-lb-harvester
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Daemons
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.harvester/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  chrpath
BuildRequires:  docbook-utils
BuildRequires:  glite-jobid-api-c-devel
BuildRequires:  glite-lb-client-devel
BuildRequires:  glite-lb-common-devel
BuildRequires:  glite-lbjp-common-gss-devel
BuildRequires:  glite-lbjp-common-db-devel
BuildRequires:  glite-lbjp-common-log-devel
BuildRequires:  glite-lbjp-common-trio-devel
BuildRequires:  libtool
BuildRequires:  pkgconfig
Requires(post): chkconfig
Requires(preun): chkconfig
Requires(preun): initscripts

%description
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lb.harvester
make


%check
make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
sed -i 's,\(lockfile=/var/lock\),\1/subsys,' $RPM_BUILD_ROOT/etc/init.d/glite-lb-harvester
find $RPM_BUILD_ROOT -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'


%clean
rm -rf $RPM_BUILD_ROOT


%pre
getent group glite >/dev/null || groupadd -r glite
getent passwd glite >/dev/null || useradd -r -g glite -d /var/glite -c "gLite user" glite
mkdir -p /var/glite /var/log/glite 2>/dev/null || :
chown glite:glite /var/glite /var/log/glite
exit 0


%post
/sbin/chkconfig --add glite-lb-harvester
if [ $1 -eq 1 ] ; then
	/sbin/chkconfig glite-lb-harvester off
fi


%preun
if [ $1 -eq 0 ] ; then
    /sbin/service glite-lb-harvester stop >/dev/null 2>&1
    /sbin/chkconfig --del glite-lb-harvester
fi


%postun
if [ "$1" -ge "1" ] ; then
    /sbin/service glite-lb-harvester condrestart >/dev/null 2>&1 || :
fi


%files
%defattr(-,root,root)
%dir /etc/glite-lb/
%dir /usr/%{_lib}/glite-lb/
%dir /usr/%{_lib}/glite-lb/examples/
%dir /usr/share/glite/
%dir /usr/share/doc/%{name}-%{version}/
/etc/init.d/glite-lb-harvester
%{_bindir}/glite-lb-harvester
%{_libdir}/glite-lb/examples/glite-lb-harvester-test.sh
%{_libdir}/glite-lb/examples/glite-lb-harvester-dbg
/usr/share/doc/%{name}-%{version}/README
/usr/share/glite/*
/usr/share/man/man1/glite-lb-harvester.1.gz


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package

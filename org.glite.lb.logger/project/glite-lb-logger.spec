Summary: @SUMMARY@
Name: glite-lb-logger
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: Apache Software License
Vendor: EMI
Group: System Environment/Daemons
BuildRequires: cppunit-devel
BuildRequires: chrpath
BuildRequires: glite-lb-common-devel
BuildRequires: glite-jobid-api-c-devel
BuildRequires: glite-lbjp-common-gss-devel
BuildRequires: glite-lbjp-common-trio-devel
BuildRequires: glite-lbjp-common-log-devel
BuildRequires: libtool
Requires(post): chkconfig
Requires(preun): chkconfig
Requires(preun): initscripts
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.logger/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lb.logger
make


%check
make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
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
/sbin/chkconfig --add glite-lb-locallogger
if [ $1 -eq 1 ] ; then
	# XXX: or rather to detect finalized set-up in the start-up scripts?
	/sbin/chkconfig glite-lb-locallogger off
fi


%preun
if [ $1 -eq 0 ] ; then
    /sbin/service glite-lb-locallogger stop >/dev/null 2>&1
    /sbin/chkconfig --del glite-lb-locallogger
fi


%postun
if [ "$1" -ge "1" ] ; then
    /sbin/service glite-lb-locallogger restart >/dev/null 2>&1 || :
fi


%files
%defattr(-,root,root)
%dir /usr/include/glite/
%dir /usr/include/glite/lb/
%dir /usr/share/doc/%{name}-%{version}/
/etc/init.d/glite-lb-locallogger
/usr/bin/glite-lb-notif-interlogd
/usr/bin/glite-lb-interlogd
/usr/bin/glite-lb-logd
/usr/include/glite/lb/*.h
/usr/share/doc/%{name}-%{version}/ChangeLog
/usr/share/doc/%{name}-%{version}/LICENSE
/usr/share/doc/%{name}-%{version}/package.description
/usr/share/doc/%{name}-%{version}/package.summary
/usr/share/man/man8/glite-lb-interlogd.8.gz
/usr/share/man/man8/glite-lb-logd.8.gz


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package

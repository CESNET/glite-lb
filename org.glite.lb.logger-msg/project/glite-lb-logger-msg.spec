Summary: @SUMMARY@
Name: glite-lb-logger-msg
Version: @MAJOR@.@MINOR@.@REVISION@
Release: @AGE@%{?dist}
Url: @URL@
License: ASL 2.0
Vendor: EMI
Group: System Environment/Daemons
%if 0%{?fedora}
BuildRequires: activemq-cpp-devel%{_isa}
%else
BuildRequires: activemq-cpp-library
%endif
BuildRequires: cppunit-devel%{?_isa}
BuildRequires: glite-lb-logger-devel%{?_isa}
BuildRequires: glite-lbjp-common-log-devel%{?_isa}
BuildRequires: glite-lbjp-common-trio-devel%{?_isa}
BuildRequires: libtool
BuildRequires: pkgconfig
Requires: perl-LDAP
Requires: glite-lb-logger
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
AutoReqProv: yes
Source: http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.logger-msg/%{version}/src/%{name}-@VERSION@.src.tar.gz


%description
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lb.logger-msg
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


%files
%defattr(-,root,root)
%dir /etc/glite-lb/
%dir /usr/%{_lib}/glite-lb/
%dir /usr/%{_lib}/glite-lb/examples/
%dir /usr/share/doc/%{name}-%{version}/
/etc/cron.d/glite-lb-logger-msg
/usr/%{_lib}/activemq_cpp_plugin.so
/usr/%{_lib}/activemq_cpp_plugin.so.0
/usr/%{_lib}/activemq_cpp_plugin.so.0.0.0
/usr/%{_lib}/glite-lb/examples/glite-lb-cmsclient
/usr/share/doc/%{name}-%{version}/ChangeLog
/usr/share/doc/%{name}-%{version}/LICENSE
/usr/share/doc/%{name}-%{version}/msg.conf.example
/usr/share/doc/%{name}-%{version}/package.summary
/usr/share/doc/%{name}-%{version}/package.description
/usr/sbin/glite-lb-msg-*


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package

Name:           glite-lb-logger-msg
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Daemons
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.logger-msg/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%if 0%{?fedora}
BuildRequires:  activemq-cpp-devel%{_isa}
%else
BuildRequires:  activemq-cpp-library
%endif
BuildRequires:  cppunit-devel%{?_isa}
BuildRequires:  glite-lb-logger-devel%{?_isa}
BuildRequires:  glite-lbjp-common-log-devel%{?_isa}
BuildRequires:  glite-lbjp-common-trio-devel%{?_isa}
BuildRequires:  libtool
BuildRequires:  pkgconfig
Requires:       perl-LDAP
Requires:       glite-lb-logger

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
%doc LICENSE project/ChangeLog config/msg.conf.example
%dir %{_sysconfdir}/glite-lb/
%dir %{_libdir}/glite-lb/
%dir %{_libdir}/glite-lb/examples/
%{_sysconfdir}/cron.d/glite-lb-logger-msg
%{_libdir}/activemq_cpp_plugin.so
%{_libdir}/activemq_cpp_plugin.so.0
%{_libdir}/activemq_cpp_plugin.so.0.0.0
%{_libdir}/glite-lb/examples/glite-lb-cmsclient
%{_sbindir}/glite-lb-msg-*


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package

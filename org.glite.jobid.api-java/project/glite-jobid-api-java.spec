%global distver %(rpm -q --quiet redhat-release && rpm -q --queryformat "%{VERSION}" redhat-release || rpm -q --quiet centos-release && rpm -q --queryformat "%{VERSION}" centos-release || rpm -q --quiet sl-release && rpm -q --queryformat "%{VERSION}" sl-release | sed 's/^\\([0-9]*\\).*/\\1/')

Name:           glite-jobid-api-java
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Libraries
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.jobid.api-java/%{version}/src/%{name}-@VERSION@.src.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildArch:      noarch
BuildRequires:  ant
BuildRequires:  jakarta-commons-codec
%if 0%{?distver} >= 6
BuildRequires:  java-1.6.0-openjdk-devel%{?_isa}
%else
BuildRequires:  java-devel
%endif
Requires:       jakarta-commons-codec

%description
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module jobid.api-java
make


%check
make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*.la' -exec rm -rf {} \;
find $RPM_BUILD_ROOT -name '*.a' -exec rm -rf {} \;


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
/usr/share/java/jobid-api-java.jar


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package

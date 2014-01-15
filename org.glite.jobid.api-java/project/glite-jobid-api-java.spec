%global groupId     org.glite
%global artifactId  jobid-api-java
%{!?_mavenpomdir: %global _mavenpomdir %{_datadir}/maven2/poms}

Name:           glite-jobid-api-java
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Libraries
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.jobid.api-java/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildArch:      noarch
BuildRequires:  ant
BuildRequires:  jakarta-commons-codec
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
%if 0%{?rhel} >= 7 || 0%{?fedora}
BuildRequires:  maven-local
%else
BuildRequires:  java-devel
BuildRequires:  jpackage-utils
%endif
Requires:       jakarta-commons-codec
Requires:       java
%if 0%{?rhel} <= 6 && ! 0%{?fedora}
Requires:       jpackage-utils
Requires(post): jpackage-utils
Requires(postun): jpackage-utils
%endif

%description
@DESCRIPTION@


%package        javadoc
Summary:        Java API documentation for %{name}
Group:          Documentation
Requires:       %{name} = %{version}-%{release}
%if 0%{?rhel} <= 6 && ! 0%{?fedora}
Requires:       jpackage-utils
%endif

%description    javadoc
This package contains java API documentation for java implementation of gLite
jobid.


%prep
%setup -q


%build
perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi --module jobid.api-java
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_javadocdir}
mv $RPM_BUILD_ROOT%{_docdir}/%{name}-%{version}/api $RPM_BUILD_ROOT%{_javadocdir}/%{name}
mkdir -p $RPM_BUILD_ROOT%{_mavenpomdir}
install -m 0644 JPP-%{name}.pom $RPM_BUILD_ROOT%{_mavenpomdir}
%if 0%{?add_maven_depmap:1}
%add_maven_depmap JPP-%{name}.pom %{name}.jar
%else
%add_to_maven_depmap %{groupId} %{artifactId} %{version} JPP %{name}
%endif


%clean
rm -rf $RPM_BUILD_ROOT


%if 0%{?rhel} <= 6 && ! 0%{?fedora}
%post
%update_maven_depmap
%endif


%if 0%{?rhel} <= 6 && ! 0%{?fedora}
%postun
%update_maven_depmap
%endif


%files
%defattr(-,root,root)
%doc LICENSE project/ChangeLog
%{_javadir}/%{name}.jar
%{_mavendepmapfragdir}/%{name}
%{_mavenpomdir}/JPP-%{name}.pom

%files javadoc
%defattr(-,root,root)
%{_javadocdir}/%{name}


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package

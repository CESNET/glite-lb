%global groupId     org.glite
%global artifactId  jobid-api-java
%{!?_mavenpomdir: %global _mavenpomdir %{_datadir}/maven2/poms}

Name:           glite-jobid-api-java
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Libraries
License:        ASL 2.0
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.jobid.api-java/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildArch:      noarch
BuildRequires:  ant
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
%if 0%{?rhel} >= 7 || 0%{?fedora}
BuildRequires:  apache-commons-codec
BuildRequires:  maven-local
%else
BuildRequires:  jakarta-commons-codec
BuildRequires:  java-devel
BuildRequires:  jpackage-utils
%endif
%if 0%{?rhel} >= 7 || 0%{?fedora}
Requires:       apache-commons-codec
%else
Requires:       jakarta-commons-codec
%endif
%if 0%{?rhel} >= 7 || 0%{?fedora} >= 20
Requires:       java-headless
%else
Requires:       java
%endif
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
./configure --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi
make %{?_smp_mflags}


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
mkdir -p %{buildroot}%{_javadocdir}
mv %{buildroot}%{_docdir}/%{name}-%{version}/api %{buildroot}%{_javadocdir}/%{name}
mkdir -p %{buildroot}%{_mavenpomdir}
install -m 0644 JPP-%{name}.pom %{buildroot}%{_mavenpomdir}
%if 0%{?add_maven_depmap:1}
%add_maven_depmap JPP-%{name}.pom %{name}.jar
%else
%add_to_maven_depmap %{groupId} %{artifactId} %{version} JPP %{name}
touch .mfiles
%endif


%clean
rm -rf %{buildroot}


%if 0%{?rhel} <= 6 && ! 0%{?fedora}
%post
%update_maven_depmap
%endif


%if 0%{?rhel} <= 6 && ! 0%{?fedora}
%postun
%update_maven_depmap
%endif


%files -f .mfiles
%{!?_licensedir:%global license %doc}
%defattr(-,root,root)
%doc ChangeLog
%license LICENSE
%if ! 0%{?add_maven_depmap:1}
%{_javadir}/%{name}.jar
%{_mavendepmapfragdir}/%{name}
%{_mavenpomdir}/JPP-%{name}.pom
%endif

%files javadoc
%defattr(-,root,root)
%{_javadocdir}/%{name}


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package

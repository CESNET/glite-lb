%global with_trustmanager 0
%global groupId     org.glite
%global artifactId  lb-client-java
%{!?_mavenpomdir: %global _mavenpomdir %{_datadir}/maven2/poms}

Name:           glite-lb-client-java
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          System Environment/Libraries
License:        ASL 2.0
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.client-java/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:  ant
%if 0%{?rhel} >= 7 || 0%{?fedora}
BuildRequires:  axis
%else
# only in EMI third-party repository
BuildRequires:  axis1.4
%endif
BuildRequires:  chrpath
%if %{with_trustmanager}
# only in EMI third-party repository
BuildRequires:  emi-trustmanager
BuildRequires:  emi-trustmanager-axis
%endif
BuildRequires:  glite-jobid-api-java
BuildRequires:  glite-lb-types
BuildRequires:  glite-lb-ws-interface
BuildRequires:  libtool
BuildRequires:  log4j
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
%if 0%{?rhel} >= 7 || 0%{?fedora}
BuildRequires:  apache-commons-lang
BuildRequires:  maven-local
%else
BuildRequires:  jakarta-commons-lang
BuildRequires:  java-devel
BuildRequires:  jpackage-utils
%endif
Requires:       glite-jobid-api-java
%if 0%{?rhel} >= 7 || 0%{?fedora}
Requires:       apache-commons-lang
%else
Requires:       jakarta-commons-lang
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


%package        axis
Summary:        Axis 1.4 flavor of Java L&B client
Group:          System Environment/Libraries
Requires:       %{name} = %{version}-%{release}
%if %{with_trustmanager}
Requires:       emi-trustmanager-axis
Requires:       emi-trustmanager
%endif
Requires:       glite-jobid-api-java
%if 0%{?rhel} >= 7 || 0%{?fedora}
Requires:       apache-commons-lang
%else
Requires:       jakarta-commons-lang
Requires:       jpackage-utils
%endif
%if 0%{?rhel} >= 6 || 0%{?fedora}
BuildArch:      noarch
%endif

%description    axis
This package contains java L&B client library based on Axis 1.4.


%if %{with_trustmanager}
%package        examples
Summary:        Java L&B client examples
Group:          Applications/Communications
Requires:       %{name} = %{version}-%{release}
Requires:       %{name}-axis
Requires:       emi-trustmanager-axis
Requires:       emi-trustmanager
Requires:       glite-jobid-api-java
%if 0%{?rhel} <= 6 && ! 0%{?fedora}
Requires:       jpackage-utils
%endif
Requires:       log4j
%if 0%{?rhel} >= 6 || 0%{?fedora}
BuildArch:      noarch
%endif

%description    examples
This package contains java L&B client examples for Axis 1.4. For the
communication is used trustmanager or pure SSL.
%endif


%package        javadoc
Summary:        Java API documentation for %{name}
Group:          Documentation
Requires:       %{name} = %{version}-%{release}
%if 0%{?rhel} <= 6 && ! 0%{?fedora}
Requires:       jpackage-utils
%endif
%if 0%{?rhel} >= 6 || 0%{?fedora}
BuildArch:      noarch
%endif

%description    javadoc
This package contains java API documentation for java implementation of gLite
L&B client.


%prep
%setup -q


%build
%if 0%{?rhel} <= 6 && ! 0%{?fedora}
# axis from EMI third-party repository
args="--with-axis=/usr/local/axis1.4"
%endif
perl ./configure --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi $args
if [ "%with_trustmanager" == "0" ]; then
    echo >> Makefile.inc
    echo "trustmanager_prefix=no" >> Makefile.inc
fi
# parallel build not supported
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make LOADER_SOURCES=JNIFedoraLoader.java


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
# move API docs
mkdir -p %{buildroot}%{_javadocdir}
mv %{buildroot}%{_docdir}/%{name}-%{version}/api %{buildroot}%{_javadocdir}/%{name}
rm -rf %{buildroot}%{_docdir}/%{name}-%{version}
# move JNI
mkdir -p %{buildroot}%{_libdir}/%{name}
mv %{buildroot}%{_libdir}/libglite_lb_sendviasocket.so* %{buildroot}%{_libdir}/%{name}
# move JAR (using JNI)
mkdir -p %{buildroot}%{_jnidir}
mv %{buildroot}%{_javadir}/%{name}.jar %{buildroot}%{_jnidir}
rm -f %{buildroot}%{_libdir}/*.a
rm -f %{buildroot}%{_libdir}/*.la
find %{buildroot} -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'
mkdir -p %{buildroot}%{_mavenpomdir}
install -m 0644 JPP-%{name}.pom JPP-%{name}-axis.pom %{buildroot}%{_mavenpomdir}
%if 0%{?add_maven_depmap:1}
%add_maven_depmap JPP-%{name}.pom %{name}.jar
%add_maven_depmap JPP-%{name}-axis.pom %{name}-axis.jar -f axis
%else
%add_to_maven_depmap %{groupId} %{artifactId} %{version} JPP %{name}
%add_to_maven_depmap %{groupId} %{artifactId}-axis %{version} JPP %{name}-axis
touch .mfiles .mfiles-axis
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
%defattr(-,root,root)
%doc ChangeLog LICENSE
%dir %{_libdir}/%{name}/
%{_libdir}/%{name}/libglite_lb_sendviasocket.so
%{_libdir}/%{name}/libglite_lb_sendviasocket.so.0
%{_libdir}/%{name}/libglite_lb_sendviasocket.so.0.0.0
%if ! 0%{?add_maven_depmap:1}
%{_jnidir}/%{name}.jar
%{_mavendepmapfragdir}/%{name}
%{_mavenpomdir}/JPP-%{name}.pom
%endif

%files axis -f .mfiles-axis
%defattr(-,root,root)
%if ! 0%{?add_maven_depmap:1}
%{_javadir}/%{name}-axis.jar
%{_mavenpomdir}/JPP-%{name}-axis.pom
%endif

%if %{with_trustmanager}
%files examples
%defattr(-,root,root)
%{_javadir}/%{name}-examples.jar
%endif

%files javadoc
%defattr(-,root,root)
%{_javadocdir}/%{name}


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package

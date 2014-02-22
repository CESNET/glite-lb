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
Url:            @URL@
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
BuildRequires:  jakarta-commons-lang
BuildRequires:  libtool
BuildRequires:  log4j
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
%if 0%{?rhel} >= 7 || 0%{?fedora}
BuildRequires:  maven-local
%else
BuildRequires:  java-devel
BuildRequires:  jpackage-utils
%endif
Requires:       glite-jobid-api-java
Requires:       jakarta-commons-lang
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
Requires:       jakarta-commons-lang
%if 0%{?rhel} <= 6 && ! 0%{?fedora}
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
perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi --module lb.client-java $args
if [ "%with_trustmanager" == "0" ]; then
    echo >> Makefile.inc
    echo "trustmanager_prefix=no" >> Makefile.inc
fi
# parallel build not supported
CFLAGS="%{?optflags}" LDFLAGS="%{?__global_ldflags}" make LOADER_SOURCES=JNIFedoraLoader.java


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
# move API docs
mkdir -p $RPM_BUILD_ROOT%{_javadocdir}
mv $RPM_BUILD_ROOT%{_docdir}/%{name}-%{version}/api $RPM_BUILD_ROOT%{_javadocdir}/%{name}
rm -rf $RPM_BUILD_ROOT%{_docdir}/%{name}-%{version}
# move JNI
mkdir -p $RPM_BUILD_ROOT%{_libdir}/%{name}
mv $RPM_BUILD_ROOT%{_libdir}/libglite_lb_sendviasocket.so* $RPM_BUILD_ROOT%{_libdir}/%{name}
# move JAR (using JNI)
mkdir -p $RPM_BUILD_ROOT%{_jnidir}
mv $RPM_BUILD_ROOT%{_javadir}/%{name}.jar $RPM_BUILD_ROOT%{_jnidir}
rm -f $RPM_BUILD_ROOT%{_libdir}/*.a
rm -f $RPM_BUILD_ROOT%{_libdir}/*.la
find $RPM_BUILD_ROOT -name '*' -print | xargs -I {} -i bash -c "chrpath -d {} > /dev/null 2>&1" || echo 'Stripped RPATH'
mkdir -p $RPM_BUILD_ROOT%{_mavenpomdir}
install -m 0644 JPP-%{name}.pom JPP-%{name}-axis.pom $RPM_BUILD_ROOT%{_mavenpomdir}
%if 0%{?add_maven_depmap:1}
%add_maven_depmap JPP-%{name}.pom %{name}.jar
%add_maven_depmap JPP-%{name}-axis.pom %{name}-axis.jar -f axis
%else
%add_to_maven_depmap %{groupId} %{artifactId} %{version} JPP %{name}
%add_to_maven_depmap %{groupId} %{artifactId}-axis %{version} JPP %{name}-axis
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
%dir %{_libdir}/%{name}/
%{_libdir}/%{name}/libglite_lb_sendviasocket.so
%{_libdir}/%{name}/libglite_lb_sendviasocket.so.0
%{_libdir}/%{name}/libglite_lb_sendviasocket.so.0.0.0
%{_jnidir}/%{name}.jar
%{_mavendepmapfragdir}/%{name}
%{_mavenpomdir}/JPP-%{name}.pom

%files axis
%defattr(-,root,root)
%{_javadir}/%{name}-axis.jar
%if 0%{?add_maven_depmap:1}
%{_mavendepmapfragdir}/%{name}-axis
%endif
%{_mavenpomdir}/JPP-%{name}-axis.pom

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

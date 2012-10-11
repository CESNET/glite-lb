%global is_fedora %(rpm -q --quiet fedora-release && echo 1 || echo 0)

Name:           glite-lb-doc
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          Development/Tools
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.doc/%{version}/src/%{name}-@VERSION@.src.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildArch:      noarch
%if %is_fedora
BuildRequires:  texlive-latex
%else
BuildRequires:  tetex-latex
%endif
BuildRequires:  glite-lb-types

%description
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=/usr --libdir=%{_lib} --project=emi --module lb.doc
make


%check
make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%dir /usr/share/doc/%{name}-%{version}/
%dir /usr/share/doc/%{name}-%{version}/examples/
/usr/share/doc/%{name}-%{version}/examples/*
/usr/share/doc/%{name}-%{version}/LICENSE
/usr/share/doc/%{name}-%{version}/README
/usr/share/doc/%{name}-%{version}/ChangeLog
/usr/share/doc/%{name}-%{version}/LBAG.pdf
/usr/share/doc/%{name}-%{version}/LBUG.pdf
/usr/share/doc/%{name}-%{version}/LBDG.pdf
/usr/share/doc/%{name}-%{version}/LBTG.pdf
/usr/share/doc/%{name}-%{version}/LBTP.pdf
/usr/share/doc/%{name}-%{version}/package.summary
/usr/share/doc/%{name}-%{version}/package.description


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@%{?dist}
- automatically generated package

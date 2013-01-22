Name:           glite-lb-doc
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          Development/Tools
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.doc/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildArch:      noarch
%if %{?fedora}%{!?fedora:0} >= 9 || %{?rhel}%{!?rhel:0} >= 6
BuildRequires:  tex(latex)
%else
BuildRequires:  tetex-latex
%endif
BuildRequires:  glite-lb-types

%description
@DESCRIPTION@


%prep
%setup -q


%build
/usr/bin/perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi --module lb.doc
make


%check
make check


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
install -m 0644 LICENSE project/ChangeLog $RPM_BUILD_ROOT%{_docdir}/%{name}-%{version}


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%dir %{_docdir}/%{name}-%{version}/
%dir %{_docdir}/%{name}-%{version}/examples/
%{_docdir}/%{name}-%{version}/examples/*
%{_docdir}/%{name}-%{version}/ChangeLog
%{_docdir}/%{name}-%{version}/LICENSE
%{_docdir}/%{name}-%{version}/README
%{_docdir}/%{name}-%{version}/LBAG.pdf
%{_docdir}/%{name}-%{version}/LBUG.pdf
%{_docdir}/%{name}-%{version}/LBDG.pdf
%{_docdir}/%{name}-%{version}/LBTG.pdf
%{_docdir}/%{name}-%{version}/LBTP.pdf


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package

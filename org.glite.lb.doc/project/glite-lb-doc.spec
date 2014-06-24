%{!?_pkgdocdir: %global _pkgdocdir %{_docdir}/%{name}-%{version}}

Name:           glite-lb-doc
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          Documentation
License:        ASL 2.0
URL:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.doc/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildArch:      noarch
BuildRequires:  glite-lb-types
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
%if 0%{?rhel} >= 6 || 0%{?fedora}
BuildRequires:  tex(latex)
%if 0%{?rhel} >= 7 || 0%{?fedora}
BuildRequires:  tex(lastpage.sty)
BuildRequires:  tex(multirow.sty)
%endif
%else
BuildRequires:  tetex-latex
%endif

%description
@DESCRIPTION@


%prep
%setup -q


%build
perl ./configure --root=/ --prefix=%{_prefix} --libdir=%{_lib} --docdir=%{_pkgdocdir} --project=emi
# parallel build not supported (problems with pdflatex)
make


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
install -m 0644 ChangeLog LICENSE %{buildroot}%{_pkgdocdir}


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root)
%dir %{_pkgdocdir}/
%dir %{_pkgdocdir}/examples/
%{_pkgdocdir}/examples/*
%{_pkgdocdir}/ChangeLog
%{_pkgdocdir}/LICENSE
%{_pkgdocdir}/README
%{_pkgdocdir}/LBAG.pdf
%{_pkgdocdir}/LBUG.pdf
%{_pkgdocdir}/LBDG.pdf
%{_pkgdocdir}/LBTG.pdf
%{_pkgdocdir}/LBTP.pdf


%changelog
* @SPEC_DATE@ @MAINTAINER@ - @MAJOR@.@MINOR@.@REVISION@-@AGE@
- automatically generated package

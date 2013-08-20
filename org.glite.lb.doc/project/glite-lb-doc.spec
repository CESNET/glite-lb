Name:           glite-lb-doc
Version:        @MAJOR@.@MINOR@.@REVISION@
Release:        @AGE@%{?dist}
Summary:        @SUMMARY@

Group:          Documentation
License:        ASL 2.0
Url:            @URL@
Vendor:         EMI
Source:         http://eticssoft.web.cern.ch/eticssoft/repository/emi/emi.lb.doc/%{version}/src/%{name}-%{version}.tar.gz
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildArch:      noarch
BuildRequires:  glite-lb-types
BuildRequires:  perl
BuildRequires:  perl(Getopt::Long)
BuildRequires:  perl(POSIX)
%if 0%{?fedora} >= 9 || 0%{?rhel} >= 6
BuildRequires:  tex(latex)
%else
BuildRequires:  tetex-latex
%endif
%if 0%{?fedora} >= 18
BuildRequires:  tex(comment.sty)
BuildRequires:  tex(lastpage.sty)
BuildRequires:  tex(multirow.sty)
%endif

%description
@DESCRIPTION@


%prep
%setup -q


%build
perl ./configure --thrflavour= --nothrflavour= --root=/ --prefix=%{_prefix} --libdir=%{_lib} --project=emi --module lb.doc
make


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

Name:		mbench
Version:	0.0.1
Release:	1%{?dist}
Summary:	Micro-benchmarking tool

Group:		Utilities/Benchmarking
License:	GPL
#URL:		
Source0:	mbench.tar.xz
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%description
mBench benchmarking tool is a tool to provide micro-benchmarking of CPU using
the Dhrystone, Whetstone and LinPack (former version of LAPACK) algorithms as
well as getting CPU count/speed and getting/setting up CPU affinity. It can also
get memory size, benchmark disk I/O for read, random read and write using the
direct memory access and network I/O for the various input buffer size. Both
server and client parts are implemented.

%prep
%setup -q

%build
%configure
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%{_bindir}/mbench
%doc



%changelog
* Wed Dec 01 2010 Michal Novotny <minovotn@redhat.com>
- Initial release (v0.0.1)

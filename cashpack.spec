Name:           cashpack
Version:        0.1
Release:        0.dev%{?dist}
Summary:        The C Anti-State HPACK library

License:        BSD
URL:            https://github.com/dridi/%{name}
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  bc
BuildRequires:  libnghttp2-devel
BuildRequires:  vim-common


%description
%{name} is a stateless event-driven HPACK codec written in C.


%package        devel
Summary:        Development files for %{name}
Requires:       %{name}%{?_isa} = %{version}-%{release}


%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.


%prep
%setup -q


%build
%configure
make %{?_smp_mflags}


%install
%make_install


%check
make %{?_smp_mflags} check


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%doc README.rst LICENSE
%{_libdir}/*.so.*
%{_libdir}/*.la


%files devel
%{_includedir}/*
%{_libdir}/*.so
%{_libdir}/*.a
%{_mandir}/man*/*


%changelog
* Mon Apr 11 2016 Dridi <dridi.boukelmoune@gmail.com> - 0.1
- Switch to autotools
- Disable memcheck

* Tue Feb  2 2016 Dridi <dridi.boukelmoune@gmail.com> - 0.1
- Initial spec

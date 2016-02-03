Name:           cashpack
Version:        0.1
Release:        0.dev%{?dist}
Summary:        The C Anti-State HPACK library

License:        BSD
URL:            https://github.com/dridi/%{name}
Source0:        %{name}-master.tar.gz

BuildRequires:  bc
BuildRequires:  cmake
BuildRequires:  python-docutils
BuildRequires:  uncrustify
BuildRequires:  valgrind
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
%setup -qn %{name}-master


%build
mkdir build
cd build
%cmake -DMEMCHECK=ON ..
make %{?_smp_mflags}


%install
cd build
%make_install


%check
cd build
make %{?_smp_mflags} check


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%doc README LICENSE
%{_libdir}/*.so.*


%files devel
%{_includedir}/*
%{_libdir}/*.so
%{_mandir}/man*/*


%changelog
* Tue Feb  2 2016 Dridi <dridi.boukelmoune@gmail.com> - 0.1
- Initial spec

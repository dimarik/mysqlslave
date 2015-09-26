Summary:	mysql replication developement library
Name:		libmysqlslave-devel
Version:	0.1.1
Release:	0%{?dist}

Group:		Developement/Libraries
License:	GPL
Source:		libmysqlslave-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
mysql replication

%prep
%setup -q -n libmysqlslave-%{version}


%build
%cmake .
make %{?_smp_mflags}


%install
rm -rf %{buildroot}
mkdir %{buildroot}
make install DESTDIR=%{buildroot}


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root,-)
%{_includedir}/mysqlslave/columndesc.hpp
%{_includedir}/mysqlslave/database.hpp
%{_includedir}/mysqlslave/exceptions.hpp
%{_includedir}/mysqlslave/logevent.hpp
%{_includedir}/mysqlslave/logparser.h
%{_includedir}/mysqlslave/value.hpp
%{_libdir}/libmysqlslave.a

%changelog

* Sat Sep 26 2015 Alexander Pankov <pianisteg@gmail.com> - 0.1.1
- MariaDB 5.5.x on CentOS 7


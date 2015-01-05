Summary:	mysql replication
Name:		mysqlslave
Version:	1.0
Release:	0%{?dist}

Group:		Networking/Daemons
License:	GPL
Source:		mysqlslave.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
mysql replication

%prep
%setup -q -n %{name}


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
%{_libdir}/libmysqlslave.so



%changelog


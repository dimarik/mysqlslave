BuildRoot: /home/rosmanov/projects/mysqlslave/_CPack_Packages/Linux/RPM/libmysqlslave-0.1.4.0-1.x86_64
Summary: mysql replication developement library
Name: libmysqlslave
Version: 0.1.4.0
Release: 1%{?dist}
Vendor: Waves Platform
Prefix: /usr/local
Group: Developement/Libraries
License: GPL
#BuildRequires:	cmake gcc-c++

%define _rpmfilename libmysqlslave-0.1.4.0-1.x86_64.rpm
%define _unpackaged_files_terminate_build 0
%define _topdir /home/rosmanov/projects/mysqlslave/_CPack_Packages/Linux/RPM
%define _rpmdir %_topdir
%define _rpmdir %_topdir/RPMS
%define _srcrpmdir %_topdir/SRPMS

%description
mysql replication

%prep
mv $RPM_BUILD_ROOT %_topdir/tmpBBroot
rm -rf "$RPM_BUILD_DIR/CMakeFiles"
rm -f "$RPM_BUILD_DIR/CMakeCache.txt"

%build

%install
if [ -e $RPM_BUILD_ROOT ];
then
  rm -Rf $RPM_BUILD_ROOT
fi
mv %_topdir/tmpBBroot $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/share

%post



%postun


%pre


%preun


%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
/usr/local/lib//libmysqlslave.a
/usr/local/include/mysqlslave/columndesc.hpp
/usr/local/include/mysqlslave/database.hpp
/usr/local/include/mysqlslave/exceptions.hpp
/usr/local/include/mysqlslave/logevent.hpp
/usr/local/include/mysqlslave/logparser.h
/usr/local/include/mysqlslave/value.hpp

%changelog

* Wed May 8 2019 Ruslan Osmanov <rrosmanov@gmail.com> - 0.1.2
- Synced spec file with CMakeLists.txt

* Sat Sep 26 2015 Alexander Pankov <pianisteg@gmail.com> - 0.1.1
- MariaDB 5.5.x and 10.0.x on CentOS 7


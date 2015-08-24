Name:           pts
Version:        <version>
Release:        1%{?dist}
Summary:        Design by DIPCC
Group:          Applications/Internet
License:        DLP
URL:            www.dipcc.com
Source0:        <sourcepath>
Requires:       glibc >= 2.0 mysql-libs >= 5.1.73

%description
PTS is the server side of the OK-RAS(OK-Remote Access System). PTS can management the ptcs, users, etc.
 
%post
if [ -f /etc/init.d/ptsd ]; then
    rm -rf /etc/init.d/ptsd
fi

if [ -f /etc/rc3.d/S90ptsd ]; then
    rm -rf /etc/rc3.d/S90ptsd
fi

if [ -f /etc/rc6.d/K90ptsd ]; then
    rm -rf /etc/rc6.d/K90ptsd
fi 

cp -rf /dipcc/var/ptsd /etc/init.d/ptsd
ln -s /etc/init.d/ptsd /etc/rc3.d/S90ptsd
ln -s /etc/init.d/ptsd /etc/rc3.d/K90ptsd
chkconfig --add ptsd
 
%build
 
%install
cd         ..
if [ ! -d BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/bin ]; then
    mkdir -p BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/bin 
fi

if [ ! -d BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/var ]; then
    mkdir -p BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/var
fi

if [ ! -d BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/html ]; then
    mkdir -p BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/html
fi

if [ ! -d BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc/lics/PTS/  ]; then
    mkdir -p BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc/lics/PTS
fi

if [ ! -f BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc/global.xml ]; then
    cp -rf <sourcepath>/conf/global.xml BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc/global.xml
fi

if [ ! -f /etc/hb-srv.xml ]; then
    cp -rf <sourcepath>/conf/hb-srv.xml BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc/hb-srv.xml
fi

cp -rf <sourcepath>/conf/license-default-data/data00.dat  BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc/lics/PTS/
cp -rf <sourcepath>/conf/license-default-data/data20.dat  BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc/lics/PTS/

cp -rf <sourcepath>/tools/apps/ptsd BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/bin
cp -rf <sourcepath>/html/pts BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/html/
cp -rf <sourcepath>/tools/tool/service/ptsd BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/var/
 
%files
/dipcc
/etc
 
%clean

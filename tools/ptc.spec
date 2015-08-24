Name:           ptc
Version:        <version>
Release:        1%{?dist}
Summary:        Design by DIPCC
Group:          Applications/Internet
License:        DLP
URL:            www.dipcc.com
Source0:        <sourcepath>
Requires:       glibc >= 2.0 mysql-libs >= 5.1.73

%description
PTC is the client side of the OK-RAS(OK-Remote Access System). PTC can help the OK-RAS access device in the LAN.
 
%post
if [ -f /etc/init.d/ptcd ]; then
    rm -rf /etc/init.d/ptcd
fi

if [ -f /etc/rc3.d/S91ptcd ]; then
    rm -rf /etc/rc3.d/S91ptcd
fi

if [ -f /etc/rc6.d/K91ptcd ]; then
    rm -rf /etc/rc6.d/K91ptcd
fi 

cp -rf /dipcc/var/ptcd /etc/init.d/ptcd
ln -s /etc/init.d/ptcd /etc/rc3.d/S91ptcd
ln -s /etc/init.d/ptcd /etc/rc3.d/K91ptcd
chkconfig --add ptcd
 
%build
 
%install
cd         ..
if [ ! -d BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/bin ]; then
    mkdir -p BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/bin 
fi

if [ ! -d BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/var ]; then
    mkdir -p BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/var
fi

if [ ! -d BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc  ]; then
    mkdir -p BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc
fi

if [ ! -f BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc/global.xml ]; then
    cp -rf <sourcepath>/conf/global.xml BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc/global.xml
fi

if [ ! -f /etc/hb-srv.xml ]; then
    cp -rf <sourcepath>/conf/hb-srv.xml BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc/hb-srv.xml
fi

cp -rf <sourcepath>/tools/apps/ptcd BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/bin
cp -rf <sourcepath>/tools/tool/service/ptcd BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/var/
 
%files
/dipcc
/etc
 
%clean

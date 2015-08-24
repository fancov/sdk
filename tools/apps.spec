Name:           ok-cc-apps
Version:        <version>
Release:        1%{?dist}
Summary:        Design by DIPCC
Group:          Applications/Internet
License:        GPL
URL:            www.dipcc.com
Source0:        <sourcepath>
Requires:       glibc >= 2.0 mysql-libs >= 5.1.73

%description
Program collection of the OK-CC IPCC system.
 
%post
if [ -f /etc/init.d/ipccd ]; then
    rm -rf /etc/init.d/ipccd
fi

if [ -f /etc/rc3.d/S90ipccd ]; then
    rm -rf /etc/rc3.d/S90ipccd
fi

if [ -f /etc/rc6.d/K90ipccd ]; then
    rm -rf /etc/rc6.d/K90ipccd
fi 

cp /dipcc/var/ipccd /etc/init.d/ipccd
if [ ! -f /etc/rc3.d/S90ipccd ]; then
    ln -s /etc/init.d/ipccd /etc/rc3.d/S90ipccd
fi
if [ ! -f /etc/rc6.d/K90ipccd ]; then
    ln -s /etc/init.d/ipccd /etc/rc6.d/K90ipccd
fi
chkconfig --add ipccd
 
%build
 
%install
cd         ..
mkdir -p   BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/bin
mkdir -p   BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/etc
mkdir -p   BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/scripts
mkdir -p   BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/html
mkdir -p   BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/run
mkdir -p   BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/var
mkdir -p   BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/var/run
mkdir -p   BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/var/run/pid
mkdir -p   BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/dipcc/var/run/socket
mkdir -p   BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc

if [ -f <sourcepath>/tools/apps/scd ]; then
    cp    -rf  <sourcepath>/tools/apps/scd BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/bin/
fi
if [ -f <sourcepath>/tools/apps/bsd ]; then
    cp    -rf  <sourcepath>/tools/apps/bsd BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/bin/
fi
if [ -f <sourcepath>/tools/apps/mcd ]; then
    cp    -rf  <sourcepath>/tools/apps/mcd BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/bin/
fi
if [ -f <sourcepath>/tools/apps/ctrl_paneld ]; then
    cp    -rf  <sourcepath>/tools/apps/ctrl_paneld BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/bin/
fi
if [ -f <sourcepath>/tools/apps/monitord ]; then
    cp    -rf  <sourcepath>/tools/apps/monitord BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/bin/
fi

if [ ! -f BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc/global.xml ]; then
    cp -rf <sourcepath>/conf/global.xml BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc/global.xml
fi

if [ ! -f /etc/hb-srv.xml ]; then
    cp -rf <sourcepath>/conf/hb-srv.xml BUILDROOT/%{name}-%{version}-1.el6.%{_target_cpu}/etc/hb-srv.xml
fi

cp    -rf  <sourcepath>/scripts/fs-conf    BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/scripts/
 
%files
/dipcc
/etc
 
%clean

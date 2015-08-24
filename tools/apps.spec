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
Design by DIPCC
 
%prep
 
%build
 
%install
cd         ..
mkdir -p   BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/bin
mkdir -p   BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/etc
mkdir -p   BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/scripts
mkdir -p   BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/html
mkdir -p   BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/run
mkdir -p   BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/var
mkdir -p   BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/var/run
mkdir -p   BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/var/run/pid
mkdir -p   BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/var/run/socket
cp    -rf  <sourcepath>/tools/apps/* BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/bin/
cp    -rf  <sourcepath>/conf/*       BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/etc/
cp    -rf  <sourcepath>/scripts/*    BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/scripts/
cp    -rf  <sourcepath>/html/*       BUILDROOT/ok-cc-apps-<version>-1.el6.x86_64/dipcc/html/
 
%files
/dipcc
 
%clean

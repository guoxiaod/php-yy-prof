%define php_config_opts %(php-config --configuration-options 2>/dev/null)
%define peclname php-yy-prof

%{?scl: %scl_package %peclname}

%define crondir %{?scl:%_root_sysconfdir}%{!?scl:%_sysconfdir}/cron.d/

Summary: Package for yy-prof
Name: %{?scl_prefix}%{peclname}
Version: 0.1.1
Release: 1
License: PHP
Group: Development/Languages
URL: http://pecl.php.net/package/%{name}
Source: http://pecl.php.net/get/%{peclname}-%{version}.tgz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: %{?scl_prefix}php-common
Requires: %{?scl_prefix}php-cli
Requires: %{?scl_prefix}php-ncurses
Requires: %{?scl_prefix}php-pecl-http
Requires: %{?scl_prefix}php-pecl-msgpack
Requires: mdbm >= 4.11 
Requires: libcurl, cronie
Requires: /bin/hostname 
BuildRequires: %{?scl_prefix}php-common
BuildRequires: %{?scl_prefix}php-cli
BuildRequires: %{?scl_prefix}php-devel
BuildRequires: %{?scl_prefix}php-pecl-http-devel
BuildRequires: %{?scl_prefix}php-pecl-raphf-devel
BuildRequires: %{?scl_prefix}php-pecl-propro-devel
BuildRequires: mdbm-devel >= 4.11, libcurl-devel
# Required by phpize
BuildRequires: autoconf, automake, libtool
BuildArch: x86_64

%description
prof php ext

%prep
%setup -n %{peclname}-%{version} -q

%build
# Workaround for broken old phpize on 64 bits
phpize
%{configure} 
%{__make} %{?_smp_mflags}
#%{__make} test

%install
%{__rm} -rf %{buildroot}
%{__make} install INSTALL_ROOT=%{buildroot}

# Drop in the bit of configuration
%{__mkdir_p} %{buildroot}%{php_inidir}
%{__mkdir_p} %{buildroot}%{crondir}

%{__cat} > %{buildroot}%{php_inidir}/yy_prof.ini << 'EOF'
; Enable yc prof extension
extension=yy_prof.so
; set if auto start tracking the function call
yy_prof.auto_enable = 0
; slow run function will log into %{_localstatedir}/log/yy_prof/slow.log
yy_prof.slow_run_time = 500000
; enable trace functions
yy_prof.enable_trace = 1
; enable trace functions in cli mode
yy_prof.enable_trace_cli = 0
; enable auto detect request in cli mode
yy_prof.enable_request_detector = 1
; auto disable tracking if load greater than yy_prof.protect_load
yy_prof.protect_load = 3
; auto reenable tracking if load lower than yy_prof.protect_restore_load
yy_prof.protect_restore_load = 2
; check load interval
yy_prof.protect_check_interval = 10
EOF

%{__cat} > %{buildroot}%{crondir}/%{?scl_prefix}yy_prof << 'EOF'
10 1 * * * root /bin/find %{_localstatedir}/log/yy_prof/ -type f -mtime +15 -delete 2>&1 | logger -t yy_prof
0 0 * * * root %{__php} -dyy_prof.auto_enable=0 %{_bindir}/yy_prof_stat.php --clear 2>&1 | logger -t yy_prof_clear
EOF

%{__mkdir_p} %{buildroot}%{_localstatedir}/cache/yy_prof/
%{__mkdir_p} %{buildroot}%{_localstatedir}/log/yy_prof/
%{__mkdir_p} %{buildroot}%{_bindir}
%{__mkdir_p} %{buildroot}%{_datadir}/yy_prof/
%{__install} -m 0755 bin/yy_prof_stat %{buildroot}%{_bindir}/yy_prof_stat.php

%{__cat} > %{buildroot}%{_bindir}/yy_prof_stat << 'EOF'
#!/bin/bash
%{__php} -dyy_prof.auto_enable=0 %{_bindir}/yy_prof_stat.php $@
EOF

%clean
%{__rm} -rf %{buildroot}

%post 

%{__php} -dextension=yy_prof.so -dyy_prof.auto_enable=1 -r 'echo "";' >/dev/null 2>&1

%files
%defattr(-, root, root, 0755)
%config(noreplace) %{php_inidir}/yy_prof.ini
%config(noreplace) %{crondir}/%{?scl_prefix}yy_prof
%{php_extdir}/yy_prof.so
%attr(0777, root, root) %{_localstatedir}/cache/yy_prof/
%attr(0777, root, root) %{_localstatedir}/log/yy_prof/
%attr(0755, root, root) %{_bindir}/yy_prof_stat
%attr(0644, root, root) %{_bindir}/yy_prof_stat.php

%changelog

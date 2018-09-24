%define php_config_opts %(php-config --configuration-options 2>/dev/null)
%define peclname php-pecl-ncurses

%{?scl: %scl_package %peclname}

%define crondir %{?scl:%_root_sysconfdir}%{!?scl:%_sysconfdir}/cron.d/

Summary: Package for ncurses
Name: %{?scl_prefix}%{peclname}
Version: 1.0.2
Release: 1
License: PHP
Group: Development/Languages
URL: https://github.com/marcinguy/php7-ncurses
Source: https://github.com/marcinguy/php7-ncurses/.../%{peclname}-%{version}.tgz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: %{?scl_prefix}php-common
Requires: ncurses
BuildRequires: %{?scl_prefix}php-devel
BuildRequires: ncurses-devel
BuildRequires: autoconf, automake, libtool
BuildArch: x86_64
Provides: php-ncurses

%description
ncurses php ext

%prep
%setup -n %{peclname}-%{version} -q

%build
phpize
%configure 
%{__make} %{?_smp_mflags}

%install
%{__rm} -rf %{buildroot}
%{__make} install INSTALL_ROOT=%{buildroot}

# Drop in the bit of configuration
%{__mkdir_p} %{buildroot}%{php_inidir}

%{__cat} > %{buildroot}%{php_inidir}/ncurses.ini << 'EOF'
; Enable ncurses extension
extension=ncurses.so
EOF

%clean
%{__rm} -rf %{buildroot}

%post 

%files
%defattr(-, root, root, 0755)
%config(noreplace) %{php_inidir}/ncurses.ini
%{php_extdir}/ncurses.so

%changelog

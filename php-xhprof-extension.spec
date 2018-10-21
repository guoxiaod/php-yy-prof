Name:		php-xhprof-extension
Version:	5.0
Release:	1%{?dist}
Summary:	php xhprof extension

Group:		Development/Languages
License:	MIT
URL:		https://github.com/tideways/php-xhprof-extension
Source0:	%{name}-%{version}.tgz

BuildRequires: php-devel
Requires:	php-common

%description
php xhprof extension

%prep
%setup -q


%build
phpize
%configure
make %{?_smp_mflags}


%install
make install DESTDIR=%{buildroot}


%files
%{php_extdir}/

%changelog


#  Copyright (c) 2020, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
%define compiler_name intel
%define compiler_version 19.0.4
%define mpi_name mvapich2
%define mpi_version 2.3

%define pname geopm
%define PNAME GEOPM

%define python_family python2
%define install_prefix %{install_parent_dir}/%{pname}/%{pname}-%{version}-%{compiler_name}-%{compiler_version}-%{mpi_name}-%{mpi_version}
%define module_prefix %{module_parent_dir}/%{compiler_name}/%{compiler_version}/%{mpi_name}/%{mpi_version}/%{pname}
%define module_name %{version}
%define python_family_lib_dir /lib/python%{expand:%{%{python_family}_version}}/site-packages

Name:          %{pname}-%{compiler_name}-%{mpi_name}-toss
Summary:       Global Extensible Open Power Manager
Version:       1.1.0
Release:       1
License:       BSD-3-Clause
Group:         System Environment/Libraries
URL:           https://geopm.github.io
Source0:       https://github.com/geopm/geopm/releases/download/v%{version}/geopm-%{version}.tar.gz
BuildRequires: autoconf
BuildRequires: automake
BuildRequires: libtool
BuildRequires: libtool-ltdl-devel
BuildRequires: %{python_family}
BuildRequires: %{python_family}-devel
BuildRequires: unzip
BuildRequires: zlib-devel
BuildRequires: openssh
BuildRequires: elfutils-libelf-devel


%description
The Global Extensible Open Power Manager (GEOPM) is a framework for
exploring power and energy optimizations targeting high performance
computing.  The GEOPM package provides many built-in features.  A
simple use case is reading hardware counters and setting hardware
controls with platform independant syntax using a command line tool on
a particular compute node.  An advanced use case is dynamically
coordinating hardware settings across all compute nodes used by an
application in response to the application's behavior and requests
from the resource manager.  The dynamic coordination is implemented as
a hierarchical control system for scalable communication and
decentralized control. The hierarchical control system can optimize
for various objective functions including maximizing global
application performance within a power bound or minimizing energy
consumption with marginal degradation of application performance.  The
root of the control hierarchy tree can communicate with the system
resource manager to extend the hierarchy above the individual MPI
application and enable the management of system power resources for
multiple MPI jobs and multiple users by the system resource manager.

%prep

%setup -q -n %{pname}-%{version}

%build
module load %{compiler_name}/%{compiler_version}
module load %{mpi_name}/%{mpi_version}
module load mkl
./autogen.sh
CC=icc CXX=icpc FC=ifort F77=ifort \
./configure --prefix=%{install_prefix} \
            --with-python=%{python_family} \
            || ( cat config.log && false )

%{__make} %{?_smp_mflags}

%install
%{__make} DESTDIR=%{buildroot} install
rm -f $(find %{buildroot}/%{install_prefix} -name '*.a'; \
        find %{buildroot}/%{install_prefix} -name '*.la'; \
        find %{buildroot}/%{install_prefix} -name 'geopmbench*')


# Module file
%{__mkdir_p} %{buildroot}%{module_prefix}
%{__cat} << EOF > %{buildroot}/%{module_prefix}/%{module_name}
#%Module1.0#####################################################################

proc ModulesHelp { } {

puts stderr " "
puts stderr "This module loads the %{pname} package built with the %{compiler_name} compiler"
puts stderr "toolchain and the %{mpi_name} MPI stack."
puts stderr "\nVersion %{version}\n"

}
module-whatis "Name: %{pname}"
module-whatis "Version: %{version}"
module-whatis "Category: perftools"
module-whatis "Description: %{summary}"
module-whatis "URL: %{url}"

set     version             %{version}

prepend-path    PATH                    %{install_prefix}/bin
prepend-path    PYTHONPATH              %{install_prefix}%{python_family_lib_dir}
prepend-path    INCLUDE                 %{install_prefix}/include
prepend-path    LD_LIBRARY_PATH         %{install_prefix}/lib
prepend-path    MANPATH                 %{install_prefix}/share/man

setenv          %{PNAME}_DIR            %{install_prefix}
setenv          %{PNAME}_BIN            %{install_prefix}/bin
setenv          %{PNAME}_LIB            %{install_prefix}/lib
setenv          %{PNAME}_INC            %{install_prefix}/include

EOF

%files
%defattr(-,root,root,-)
%{install_prefix}/*
%{module_prefix}/*

%changelog
* Fri Mar 27 2020 <christopher.m.cantalupo@intel.com> v1.1.0
- Initial commit.

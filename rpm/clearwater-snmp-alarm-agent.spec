Name:           clearwater-snmp-alarm-agent
Summary:        The SNMP subagent for Clearwater alarm reporting
BuildRequires:  python2-devel python-virtualenv net-snmp-devel = 1:5.7.2-24.el7.centos.1.clearwater1 boost-devel zeromq-devel
Requires:       redhat-lsb-core clearwater-snmpd boost-filesystem

%include %{rootdir}/build-infra/cw-rpm.spec.inc

%description
The SNMP subagent for Clearwater alarm reporting

%debug_package

%install
. %{rootdir}/build-infra/cw-rpm-utils clearwater-snmp-alarm-agent %{rootdir} %{buildroot}
setup_buildroot
install_to_buildroot < %{rootdir}/debian/clearwater-snmp-alarm-agent.install
build_files_list > clearwater-snmp-alarm-agent.files

%postun
# Trigger an snmpd restart to unload the sub-agent
systemctl stop snmpd || true

%files -f clearwater-snmp-alarm-agent.files

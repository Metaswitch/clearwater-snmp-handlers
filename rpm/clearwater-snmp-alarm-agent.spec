Name:           clearwater-snmp-alarm-agent
Summary:        The SNMP subagent for Clearwater alarm reporting
BuildRequires:  net-snmp-devel = 1:5.7.2-24.el7.centos.1.clearwater1 boost-devel zeromq-devel
Requires:       redhat-lsb-core clearwater-snmpd boost-filesystem net-snmp-libs = 1:5.7.2-24.el7.centos.1.clearwater1 net-snmp-agent-libs = 1:5.7.2-24.el7.centos.1.clearwater1 zeromq

%include %{rootdir}/build-infra/cw-rpm.spec.inc

%description
The SNMP subagent for Clearwater alarm reporting

%debug_package

%install
. %{rootdir}/build-infra/cw-rpm-utils clearwater-snmp-alarm-agent %{rootdir} %{buildroot}
setup_buildroot
install_to_buildroot < %{rootdir}/debian/clearwater-snmp-alarm-agent.install
copy_to_buildroot debian/clearwater-snmp-alarm-agent.service /etc/systemd/system
build_files_list > clearwater-snmp-alarm-agent.files

%post
systemctl enable clearwater-snmp-alarm-agent
systemctl start clearwater-snmp-alarm-agent

%preun
if [ "$1" == 0 ] ; then
  # Uninstall, not upgrade.
  systemctl stop clearwater-snmp-alarm-agent
  systemctl disable clearwater-snmp-alarm-agent
fi

%postun
# Trigger an snmpd restart to unload the sub-agent
systemctl stop snmpd || true

%files -f clearwater-snmp-alarm-agent.files

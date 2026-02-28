#!/usr/bin/env bash
set -euo pipefail

pi_interface=enp4s0
if nmcli connection show med-robot-pi-direct >/dev/null 2>&1; then
  sudo nmcli connection modify med-robot-pi-direct \
    connection.autoconnect yes connection.autoconnect-priority 100 \
    ipv4.method shared ipv4.addresses 192.168.50.1/24
  sudo nmcli connection up med-robot-pi-direct
fi
printf 'Ethernet state:\n'
ip -brief address show dev "$pi_interface"
printf '\nDHCP leases:\n'
sudo sh -c 'for f in /var/lib/NetworkManager/dnsmasq-enp4s0.leases /var/lib/misc/dnsmasq.leases; do if [ -f "$f" ]; then cat "$f"; fi; done'

if ! command -v tcpdump >/dev/null; then
  printf '\ntcpdump is missing. Run sudo apt install tcpdump, then rerun this script.\n'
  exit 1
fi
printf '\nCapturing for 40 seconds. Now unplug and reconnect only the Pi Ethernet cable.\n'
printf 'Keep the Pi powered. Copy the output into the chat when capture finishes.\n\n'
sudo timeout 40 tcpdump -l -n -e -i "$pi_interface" \
  'arp or (udp and (port 67 or port 68)) or icmp6' || {
  pi_capture_status=$?
  if [[ "$pi_capture_status" -ne 124 ]]; then
    exit "$pi_capture_status"
  fi
}

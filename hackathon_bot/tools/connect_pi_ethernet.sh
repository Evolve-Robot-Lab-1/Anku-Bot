#!/usr/bin/env bash
set -euo pipefail

pi_interface=enp4s0
pi_profile=med-robot-pi-direct

if ! command -v nmcli >/dev/null; then
  printf 'NetworkManager command nmcli is missing. Copy this result into the chat.\n'
  exit 1
fi
if [[ "$(cat "/sys/class/net/$pi_interface/carrier" 2>/dev/null || true)" != 1 ]]; then
  printf 'Connect the powered Pi directly to the computer with Ethernet.\n'
  exit 1
fi

sudo -v
if ! nmcli connection show "$pi_profile" >/dev/null 2>&1; then
  sudo nmcli connection add type ethernet ifname "$pi_interface" \
    con-name "$pi_profile" connection.autoconnect no \
    ipv4.method shared ipv4.addresses 192.168.50.1/24 ipv6.method auto
fi
sudo nmcli connection modify "$pi_profile" \
  connection.autoconnect yes connection.autoconnect-priority 100 \
  ipv4.method shared ipv4.addresses 192.168.50.1/24
sudo nmcli connection up "$pi_profile"
printf '\nThe computer is now providing Ethernet addresses to the Pi.\n'
printf 'Waiting 15 seconds for the Pi to request an address...\n'
sleep 15
printf 'Finding SSH on the direct Ethernet link...\n'
mapfile -t pi_hosts < <(
  seq 2 254 | xargs -P 32 -I '{}' bash -c \
    'timeout 2 bash -c "echo >/dev/tcp/192.168.50.{}/22" 2>/dev/null && echo 192.168.50.{}' \
    | sort -u
)
if [[ ${#pi_hosts[@]} -eq 1 ]]; then
  printf 'SSH found at %s. Connecting as ubuntu.\n' "${pi_hosts[0]}"
  ssh -o ConnectTimeout=5 "ubuntu@${pi_hosts[0]}"
elif [[ ${#pi_hosts[@]} -gt 1 ]]; then
  printf 'Multiple SSH addresses found:\n'
  printf '%s\n' "${pi_hosts[@]}"
  printf 'Copy these addresses into the chat.\n'
else
  printf '\nNo SSH service found yet. Neighbor status:\n'
  ip neighbor show dev "$pi_interface" | grep -v -E 'FAILED|INCOMPLETE' || true
  printf '\nUnplug and reconnect the Pi Ethernet cable, wait 30 seconds, then run this script again.\n'
fi

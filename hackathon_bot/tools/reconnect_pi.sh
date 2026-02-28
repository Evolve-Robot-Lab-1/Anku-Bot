#!/usr/bin/env bash
set -euo pipefail

pi_interface=enp4s0
pi_address=192.168.50.2
computer_address=192.168.50.1/24

if [[ ! -d "/sys/class/net/$pi_interface" ]]; then
  printf 'Ethernet interface %s was not found.\n' "$pi_interface"
  exit 1
fi

if [[ "$(cat "/sys/class/net/$pi_interface/carrier" 2>/dev/null || true)" != 1 ]]; then
  printf 'Connect the Ethernet cable between the computer and the powered Pi.\n'
  exit 1
fi

if ! ip -4 -o address show dev "$pi_interface" | grep -q 'inet 192\.168\.50\.1/24 '; then
  printf 'Adding temporary Ethernet address %s.\n' "$computer_address"
  sudo ip address add "$computer_address" dev "$pi_interface"
fi

ip -brief address show dev "$pi_interface"
printf 'Connecting to the Pi at %s. Enter the Pi ubuntu password if prompted.\n' "$pi_address"
if ssh -o ConnectTimeout=5 "ubuntu@$pi_address"; then
  exit 0
else
  pi_ssh_status=$?
  printf '\nSSH failed. Ethernet neighbor status:\n'
  ip neighbor show dev "$pi_interface"
  printf '\nCopy this output back into the chat.\n'
  exit "$pi_ssh_status"
fi

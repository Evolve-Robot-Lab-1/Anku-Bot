# Bot 2 SD card — SSH setup

Status: **pending; card was removed before any changes were made.**

## Identified card

- USB card reader: `Storage Device`, serial `121220160204`; 14.6 GiB removable disk, formerly `/dev/sdb`. Re-identify it after reinsertion; device names can change.
- Existing partitions: 512 MiB FAT `system-boot`, 14.1 GiB ext4 `writable`.
- Existing system: Ubuntu 24.04.5 LTS arm64, hostname `sanjeev-desktop`, user `sanjeev`, about 7.2 GiB used on the root filesystem.
- Boot `config.txt` already has `dtparam=i2c_arm=on`.
- `openssh-client` 1:9.6p1-3ubuntu13.19 is installed; `openssh-server` is not. Existing SSH host keys are present, but `/usr/sbin/sshd` and `ssh.service` are absent.
- The PC has an SSH public key at `/home/evolve/.ssh/id_ed25519.pub`.

## Prepared locally

- Matching Ubuntu arm64 `openssh-server` and `openssh-sftp-server` packages were downloaded from `ports.ubuntu.com` to `/tmp/bot2-openssh-server.deb` and `/tmp/bot2-openssh-sftp-server.deb`.
- SHA-256: server `66aa97ba542be76703764b7e311fc0b29f820f32e16d503a0d084b125acd094a`; SFTP `5776107878705a9c30f64c3d498d2f0deea374bb47cb8b9650f9e04069d311f8`.

## Complete after reinsertion

1. Recheck card serial, partitions, and mounts. Do not assume `/dev/sdb` remains its device name.
2. Confirm first-boot network choice (direct Ethernet, router Ethernet, or existing Wi-Fi). Inspect the existing netplan configuration without exposing credentials.
3. Add the PC's public key for `sanjeev`, set a distinct Bot 2 hostname, and install/enable `openssh-server` using a method compatible with the mounted arm64 card. Keep SSH host identity distinct from Bot 1.
4. Boot Bot 2 and verify key-based SSH, hostname, network route, and `i2cdetect` at address `0x5f`. Do not run motors as part of SSH setup.

The card's root partition is root-owned. This session's unsandboxed shell still requires host administrator authentication for direct root-filesystem edits. No write to the SD card has occurred.

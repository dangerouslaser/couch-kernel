# Bluetooth on the HA100 kernel

This file records the kernel-side implementation. Current user-facing behavior,
release notes, and pairing guidance live in the Couch repository.

## Current source and configuration

The HA100 vendor transport remains built in through `CONFIG_MTK_COMBO_BT=y` and
`CONFIG_MTK_BTIF=y`. It exposes the MediaTek STP transport used by both the
legacy Android character-device path and Couch's HCI driver.

The base 3.18 configuration deliberately leaves `CONFIG_BT` off. Couch ships a
matching backported 4.4 Bluetooth core as modules with the boot payload:
`compat.ko`, `bluetooth.ko`, `hci_vhci.ko`, and `hci_stp.ko`. `hci_stp` binds a
BlueZ `hci_dev` to the STP export API, powers the Bluetooth function while the
adapter is open, and owns H4 receive reassembly for the supported core versions.

The selected normal kernel source is commit `81d180fc`. The matching module
vermagic and source/configuration hashes are recorded by Couch's release pin
and source receipts. Build this tree through the documented generic local or
explicit remote builder interface; no particular workstation or host layout is
part of the source contract.

## Acceptance

The OTA kernel/boot payload based on this source was accepted on HA100 for
Bluetooth startup with Wi-Fi present, an existing bonded device after reboot,
undocked standby/wake with keys, and IR before and after wake. These observations
confirm the tested payload behavior; they do not establish battery calibration,
quantitative standby savings, clean-installer behavior, or recovery validation.

## Maintainer notes

The module build recipe and the Couch STP driver source are retained in the
Couch repository under `kernel/backports/`, alongside the backports source
receipt. Keep the backported modules tied to the exact base-kernel build:
`MODVERSIONS` and vermagic make them unsuitable for a different kernel commit.
Do not enable the in-tree 3.18 Bluetooth core as a substitute for these modules
without a separate source, module, and hardware validation round.

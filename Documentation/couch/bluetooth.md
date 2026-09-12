# Bluetooth on the HA100 kernel

Kernel-side tasks for Bluetooth Low Energy. The full findings, userland plan
and staging checklist live in the Couch repository at `docs/bluetooth.md` on
its `bluetooth` branch. This file tracks only what changes in this tree.

## State (2026-09-12)

- `CONFIG_MTK_COMBO_BT=y` and `CONFIG_MTK_BTIF=y` build the MediaTek CONSYS_6580
  Bluetooth transport in. `stp_chrdev_bt.c` creates `/dev/stpbt`; opening it
  calls `mtk_wcn_wmt_func_on(WMTDRV_TYPE_BT)` and reads/writes raw HCI
  packets through `mtk_wcn_stp_{send,receive}_data(..., BT_TASK_INDX)`.
- `CONFIG_BT` is not set. There is no HCI device, so BlueZ has nothing to
  attach to. Stock Android never needed it (Bluedroid talks to `/dev/stpbt`).
- Never exercised on this hardware, from-source or stock.

## Tasks

1. Config: `CONFIG_BT=y`, `CONFIG_BT_HCIVHCI=y` (the virtual HCI driver in
   `drivers/bluetooth/hci_vhci.c`). Leave `BT_RFCOMM`, `BT_BNEP`, `BT_HIDP`
   off. Linux 3.18 has no separate LE option; LE comes with `CONFIG_BT`.
2. Spike: a userland daemon in Couch shuttles packets between `/dev/vhci` and
   `/dev/stpbt`. Acceptance is `hci0` coming up with LE reported.
3. Proper driver, once the spike proves the radio: a small `hci_dev` on top of
   the STP BT interface, next to `stp_chrdev_bt.c`, gated by a new Kconfig
   symbol so the character device remains available. `hdev->open` powers the
   function on through WMT, `hdev->send` forwards to `mtk_wcn_stp_send_data`,
   and the STP event callback drains the RX queue into `hci_recv_frame`.
4. Address: find the CONSYS_6580 vendor HCI command that programs the BD
   address, so Couch can write the owner's recorded `bluetooth_mac` at
   bring-up. Stock reads it from NVRAM, which this tree does not carry.

## Constraints

- No `Add Advertising` management command in 3.18 (added in 4.2), so the
  BlueZ advertising D-Bus API is unavailable; advertising is raw HCI.
- No LE Secure Connections in 3.18; legacy LE pairing only.
- Builds run on Ollie (`~/couch-kernel/base`), not from the Mac copy.

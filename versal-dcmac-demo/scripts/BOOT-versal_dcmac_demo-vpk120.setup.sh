#!/bin/bash
#
# Script to prepare a workspace directory for running Versal DCMAC
# CoSim demo on virtual VPK120 board.
#
# Copyright (c) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: MIT
#
# Usage:
#   BOOT-versal_dcmac_demo-vpk120.setup.sh /PATH/TO/VPK120_DCMAC /PATH/TO/EDF_ARTIFACTS
#
# Pre-requisite:
# 1. The AMD-EDF SDK must have been activated for the invoking parent shell
# 2. /PATH/TO/EDF_ARTIFACTS contains all AMD-EDF prebuilt files obtained from
#    the AMD-EDF Download Center
#
set -euo pipefail
shopt -u nullglob

WDIR=${1:-""}
ADIR=${2:-""}
HERE=$(cd $(dirname -- $0) && pwd)

die() {
  echo >&2 ERROR: "$@"
  exit 1
}

abspath() {
  if [[ ! -e "$1" ]] ; then
    echo ""
  elif [[ -d "$1" ]] ; then
    (cd "$1" && pwd)
  else
    echo $(cd $(dirname -- "$1") && pwd)/$(basename -- "$1")
  fi
}

# ======================================================================
# Checking pre-requisites and arguments
# ======================================================================

# Check to make sure SDK is activated
XZ=$(type -p xz || true)
DTC=$(type -p dtc || true)
WIC=$(type -p wic || true)
BOOTGEN=$(type -p bootgen || true)
QEMUIMG=$(type -p qemu-img || true)
FDTOVLY=$(type -p fdtoverlay || true)

[[ -n $XZ ]]      || die "AMD-EDF SDK not activated: No cmd - xz"
[[ -n $DTC ]]     || die "AMD-EDF SDK not activated: No cmd - dtc"
[[ -n $WIC ]]     || die "AMD-EDF SDK not activated: No cmd - wic"
[[ -n $BOOTGEN ]] || die "AMD-EDF SDK not activated: No cmd - bootgen"
[[ -n $QEMUIMG ]] || die "AMD-EDF SDK not activated: No cmd - qemu-img"
[[ -n $FDTOVLY ]] || die "AMD-EDF SDK not activated: No cmd - fdtoverlay"

# BOOTGEN is the best anchor because it is unique to AMD-EDF
# Other commands are usable even if not coming from AMD-EDF
SDK=$(abspath $(dirname -- $BOOTGEN)/../..)

# Validate ADIR
DISK=edf-linux-disk-image-amd-cortexa72-common+versal-vpk120-sdt-seg.rootfs

if [[ -d $ADIR ]] ; then
  set -- "$ADIR/$DISK"*.wic.xz
  DISK=$1
  [[ -f $DISK ]] || die "$DISK: not found"
  DISK=$(abspath $DISK)
fi

if [[ -z $ADIR || ! -d $ADIR ]] ; then
  # Guess ADIR (for sites with pre-installed artifacts)
  set -- $(abspath $SDK/../../../..)
  set -- "$1/versal-vpk120-sdt-seg_edf-linux-disk-image/$DISK"*.wic.xz
  DISK="$1"
  if [[ ! -f $DISK ]] ; then
    if [[ -z $ADIR ]] ; then
      die "Unable to guess /PATH/TO/EDF_ARTIFACTS"
    else
      die "$ADIR: Not a directory"
    fi
  fi
fi

# Create the required WDIR and resolve it to abspath
[[ -n $WDIR ]] || die "Usage: $0 /PATH/TO/VPK120_DCMAC /PATH/TO/EDF_ARTIFACTS"

if [[ -d "$WDIR" ]] ; then
  WDIR=$(abspath $WDIR)
else
  [[ ! -e "$WDIR" ]] || die "$WDIR: Path exists but not as a directory"
  mkdir -p "$WDIR"
  WDIR=$(abspath $WDIR)
  echo >&2 "... Mkdir $WDIR"
fi

# ======================================================================
# What follows are the actual steps to prepare a DCMAC CoSim workspace
# ======================================================================

# Locations of most files have to be hard-coded into DEMO_QBC file.
#
# DEMO_DTB and DEMO_DTBO are meant to be injected into the disk image
# in subdir $PB_WIC:1/dtb.
QB_BB="$WDIR/bootbin"                                  # Prebuilt files for QEMU boot
QB_HW="$WDIR/qemu-hw-devicetrees/multiarch"

PB_WIC="$WDIR/BOOT-versal-vpk120-sdt-seg.wic.qemu-sd"  # Prebuilt files for real hardware
PB_BIN="$WDIR/BOOT-versal-vpk120-sdt-seg.bin"
PB_DTB="$WDIR/BOOT-versal-vpk120-sdt-seg.bin.dtb"
PB_QBC="$WDIR/BOOT-versal-vpk120-sdt-seg.qemuboot.conf"

DEMO_EFI="$WDIR/BOOT-versal_dcmac_demo-vpk120.efi.conf"
DEMO_QBC="$WDIR/BOOT-versal_dcmac_demo-vpk120.qemuboot.conf"
DEMO_DTS="$WDIR/BOOT-versal_dcmac_demo-vpk120.dtsi"
DEMO_DT="$WDIR/dtb"
DEMO_DTB="$DEMO_DT/dcmac-demo.dtb"
DEMO_DTBO="$DEMO_DT/dcmac-demo.dtbo"

msg() {
  echo >&2 "$@"
}

wks_init() {
  # This group of commands:
  # - Create all workspace directories
  # - Copy or extract prebuilt artifacts without any demo-specific modifications
  #
  # The origination source directories are:
  # - HERE : this DCMAC-demo repo
  # - ADIR : the artifacts fetched from AMD-EDF Download Center
  # - SDK  : the directory where the AMD-EDF SDK has been installed
  for n in \
    "$WDIR" \
    "$QB_BB" \
    "$QB_HW" \
    "$DEMO_DT" \
  ; do
    [[ -d $n ]] && continue
    [[ ! -e $n ]] || die "$n: exists but not as a directory"

    msg "... Mkdir $n"
    mkdir -p "$n"
  done

  msg "... Entering $WDIR"
  cd "$WDIR"

  msg "... Using AMD-EDF SDK from $SDK"
  msg "... Copying virtual VPK120 board files from SDK"
  for n in \
    $SDK/qemu-hw-devicetrees/multiarch/*vpk120.dtb \
    $SDK/qemu-hw-devicetrees/multiarch/board-versal-pmc-virt.dtb \
  ; do
    cp -f $n $QB_HW/
  done

  msg "... Copying $DISK"
  cp -f "$DISK" $PB_WIC.xz

  msg "... Expand disk image"
  [[ -e $PB_WIC ]] && rm -f $PB_WIC
  $XZ -d $PB_WIC.xz

  # Virtual VPK120 board requires the disk image size in power-of-2
  n=$(set -- $(wc -c $PB_WIC) && echo "$1")
  n=$((n | n >> 1))
  n=$((n | n >> 2))
  n=$((n | n >> 4))
  n=$((n | n >> 8))
  n=$((n | n >> 16))
  n=$((n | n >> 32))
  n=$((n + 1))
  $QEMUIMG resize -q -f raw $PB_WIC $n

  # Modify the original default EFI boot entry for CoSim boot
  # (see dt_merge below), after duplicating the original as a
  # backup boot.
  $WIC cp $PB_WIC:1/loader/entries/edf-linux.conf $DEMO_EFI
  $WIC cp $DEMO_EFI $PB_WIC:1/loader/entries/edf-linux-prebuilt.conf

  sed -i '/title/s=$= DCMAC CoSim=' $DEMO_EFI
  sed -i '/title/a devicetree /dtb/dcmac-demo.dtb' $DEMO_EFI

  # Extract prebuilt BOOT.bin for real vpk120 board, and from there,
  # extract prebuilt Linux system DTB.
  #
  # The .DTB is identified using the 'file' command.
  msg "... Extract prebuilt Linux system DTB from disk image"
  $WIC cp $PB_WIC:1/boot.bin $PB_BIN

  dt=$WDIR/dt-tmp
  rm -rf $dt $PB_DTB
  mkdir -p $dt
  $BOOTGEN -arch versal -dump_dir $dt -dump $PB_BIN
  for n in $dt/*.bin ; do
    (file $n | grep -q ': Device Tree Blob') || continue

    # Found the Linux dtb within the prebuilt BOOT.bin
    cp -f $n $PB_DTB
    break
  done
  if [[ ! -f $PB_DTB ]] ; then
    die "Failed to locate Linux System DTB from $PB_BIN"
  fi
  rm -rf $dt
  unset dt

  msg "... Extract VPK120 PLM files from disk image"
  $BOOTGEN -arch versal -dump_dir $QB_BB -dump $PB_BIN boot_files

  msg "... Copying DCMAC CoSim demo files"
  cp -f $HERE/BOOT-versal-vpk120-sdt-seg.qemuboot.conf $PB_QBC
  cp -f $HERE/BOOT-versal_dcmac_demo-vpk120.dtsi $DEMO_DTS

  # Create $DEMO_QBC, not using SDK's qemuboot-tool for merging so as
  # to prevent premature format-string interpolation
  (sed -e '/README/,/Note/{//!d}' \
       $HERE/BOOT-versal_dcmac_demo-vpk120.qemuboot.conf \
   && \
   sed -e '/^#/d' \
       -e '/^\[/d' \
       -e '/^qb_dtb/d' \
       -e '/^qb_cosim/d' \
       $HERE/BOOT-versal-vpk120-sdt-seg.qemuboot.conf) > $DEMO_QBC
}

dt_merge() {
  msg "... Merging CoSim .dtsi into prebuilt Linux System DTB"

  $DTC -@ -I dts -O dtb -o $DEMO_DTBO $DEMO_DTS
  $FDTOVLY -i $PB_DTB -o $DEMO_DTB $DEMO_DTBO

  # Inject the merged DTB into disk, .e.g, for EFI loader to fetch.
  #
  # Note: wic does not have 'mkdir' subcommand; instead,
  #   wic cp DIR_FOO IMAGE:1/DIR_BAR
  # is the equivalent of
  #   rsync -a DIR_FOO/ /DIR_BAR/
  $WIC cp $DEMO_DT $PB_WIC:1/
  $WIC cp $DEMO_EFI $PB_WIC:1/loader/entries/edf-linux.conf
}

wks_init
dt_merge

# Completed
cat >&2 <<EOF
... Setup completes successfully!
...
... It is strongly recommended that the setup be tested now, using commands:
...   runqemu $(abspath $PB_QBC) nographic slirp snapshot
...
... If the setup is correct, the linux login prompt will be shown.
...
... The default login username is amd-edf.
...
... To cleanly end the run:
... 1. sudo systemctl poweroff
... 2. Wait for 'reboot: Power down' message to appear
... 3. Press Ctrl+A then press letter key 'x'
EOF

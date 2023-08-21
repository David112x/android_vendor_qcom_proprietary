#! /vendor/bin/sh

#
# Copyright (c) 2019-2020, Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
# Copyright (c) 2019 The Linux Foundation. All rights reserved.
#

export PATH=/vendor/bin

if [ -f /sys/devices/soc0/soc_id ]; then
     soc_id=`cat /sys/devices/soc0/soc_id`
else
     soc_id=`cat /sys/devices/system/soc/soc0/id`
fi

if [ "$soc_id" -eq 441 ] || [ "$soc_id" -eq 471 ]; then
	#Scuba does not support usb-pd or charge pumps
	find /sys/class/power_supply/battery/ -type f | xargs chown system.system
	find /sys/class/power_supply/bms/ -type f | xargs chown system.system
	find /sys/class/power_supply/main/ -type f | xargs chown system.system
	find /sys/class/power_supply/usb/ -type f | xargs chown system.system
else
	find /sys/class/power_supply/battery/ -type f | xargs chown system.system
	find /sys/class/power_supply/bms/ -type f | xargs chown system.system
	find /sys/class/power_supply/main/ -type f | xargs chown system.system
	find /sys/class/power_supply/usb/ -type f | xargs chown system.system
	find /sys/class/power_supply/charge_pump_master/ -type f | xargs chown system.system
	find /sys/class/power_supply/pc_port/ -type f | xargs chown system.system
	find /sys/class/power_supply/dc/ -type f | xargs chown system.system
	find /sys/class/power_supply/parallel/ -type f | xargs chown system.system
	find /sys/class/usbpd/usbpd0/ -type f | xargs chown system.system
	find /sys/class/qc-vdm/ -type f | xargs chown system.system
	find /sys/class/charge_pump/ -type f | xargs chown system.system
	find /sys/class/qcom-battery/ -type f | xargs chown system.system
fi

setprop persist.vendor.hvdcp_opti.start 1

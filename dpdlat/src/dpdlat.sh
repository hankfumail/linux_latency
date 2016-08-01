#/bin/sh

#booti 0x80000 0x6000000 0x4000000
mount /dev/mmcblk0p1 /mnt
cd mnt
pwd
ls

# Bind all process to CPU 1&2
/mnt/taskdft.elf > /mnt/taskdft_log.txt

#Create several processes to make the CPU 1&2 busy.
/mnt/llat.elf > /mnt/llat_1_log.txt &
/mnt/llat.elf > /mnt/llat_2_log.txt &
/mnt/llat.elf > /mnt/llat_3_log.txt &
/mnt/llat.elf > /mnt/llat_4_log.txt &
/mnt/taskdft.elf
/mnt/llat.elf > /mnt/llat_5_log.txt &
/mnt/llat.elf > /mnt/llat_6_log.txt &
/mnt/llat.elf > /mnt/llat_7_log.txt &
/mnt/llat.elf > /mnt/llat_8_log.txt &
/mnt/taskdft.elf

# show process information
# ps shows the wrong scheduling class SCHED_OTHER
# ps -cLe | grep cyclic
# chrt -p 4766

# Bind all process to CPU 1&2
/mnt/taskdft.elf > /mnt/taskdft_2_log.txt

#Create DPD processes to make the CPU 3 busy.
/mnt/dpdlat.elf 100000 100 4 > /mnt/dpdlat_cpu3_log.txt &

#Create DPD processes to make the CPU 4 busy.
#dpdlat test-time-in-seconds loop-dealy-time-in-us cpu-affinity-bit-mask
/mnt/dpdlat.elf > /mnt/dpdlat_cpu4_log.txt &

ls *log.txt
cat  /mnt/dpdlat_cpu3_log.txt
cat  /mnt/dpdlat_cpu4_log.txt

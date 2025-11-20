# ku_copy Benchmark

This repository contains the **`ku_copy_bench`** kernel module used to benchmark kernel-to-user copy behavior on Linux.

## 1. Install Kernel Development Packages

### **Ubuntu / Debian**
```bash
sudo apt update
sudo apt install linux-headers-$(uname -r)
sudo apt install build-essential dkms
```

### **CentOS / RHEL**
```
sudo dnf install kernel-devel kernel-headers
sudo dnf install gcc make elfutils-libelf-devel
```
## 2. Build the module
```bash
make
```
## 3. Load the module and run the benchmark
```bash
sudo insmod ku_copy_bench.ko
sudo dmesg
sudo rmmod ku_copy_bench
```

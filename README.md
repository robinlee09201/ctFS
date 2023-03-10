# ctFS
ctFS is a file system that Replaces File Indexing with Hardware Memory Translation through Contiguous File Allocation for Persistent Memory.

Code author: [Robin Li](https://www.linkedin.com/in/robin-li-1bb259b8/)

Access to the [paper and presentation](https://www.usenix.org/conference/fast22/presentation/li)

It's a prototype and meant to prove the concept and demonstrate. I will maintain it and fix bugs progressively.
You are welcome to test it and leave me feedback. Any contribution is much appreciated. 
You are also free to change and reuse the code as you wish. 
## Requirement
* Hardware: Intel Xeon CPU equiped with Intel Optan NVDIMM
* Software: Ubuntu 20.04 with all Kernel compile essentials
## Tested environment
* System: ThinkSystem SR570
* CPU: Intel(R) Xeon(R) Silver 4215 CPU @ 2.50GHz
* DRAM: 6 x 16GB DDR4 RDIMM
* PMEM: 2 x 128GB Intel Optane DCPMM
* Kernel: 5.7.0-rc7+
* Distribution: Ubuntu Server 20.04 LTS
## How to use: 
### Kernel part source code
1. Clone the Linux kernel and checkout v5.7-rc7
2. Copy the files in **/kernel/*** to the kernel repo **kernel/drivers/dax/**
3. Add the following code in mm/memory.c:
    ```c
    EXPORT_SYMBOL_GPL(__pte_alloc);
    EXPORT_SYMBOL_GPL(__pmd_alloc);
    EXPORT_SYMBOL_GPL(__pud_alloc);
    EXPORT_SYMBOL_GPL(__p4d_alloc);
    ```
    After the end of these functions:
    ```c
    __pte_alloc
    __pmd_alloc
    __pud_alloc
    __p4d_alloc
    ```
### Compile 
1. Compile and install the kernel
2. Compile ctFS user part (ctU):
    ```sh
    mkdir bld
    make
    ```
3. Compile mkfs
    ```sh
    cd test
    make mkfs
    ```
4. Compile fstest
     ```sh
    make fstest
    cd ..
    ```
### Configure
1. Reboot to the compiled kernel and configure the PMEM:
    ```sh
    script/set_nvdimm.sh
    ```
2. Format
    ```sh
    test/mkfs 1
    ```
    Note that if you only need to format the ctU (user structure), run mkfs without any arguments.
    Running mkfs with any arguments will reset the ctK (kernel structure).
### Run
1. Run with TEST_PROGRAM: 
    ```sh
    script/run_ctfs.sh TEST_PROGRAM
    ```
2. Test with fstest: 
    ```sh
    cd test
    script/run_ctfs.sh test/fstest -a -p "\\/test"
    ```
    Run with -h to show the help of fstest.
    The path to ctFS must start with "\\", otherwise it will be bypassed to the regular file system.  
## Contact
Please feel free to reach me: robinlrb.li@mail.utoronto.ca.

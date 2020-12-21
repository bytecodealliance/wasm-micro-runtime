/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include <stdio.h>
#include <string.h>
#include "Enclave_t.h"
#include "wasm_export.h"
#include "bh_platform.h"

void ecall_iwasm_test()
{
    ocall_print(" Prepare to invoke sgx_open ... ==>\n\n");

    /* creat test.txt firstly */
    char file[] = "test.txt";
    char file_hd_ln[] = "test_hd_ln.txt";
    char file_sf_ln[] = "test_sf_ln.txt";
    char rlt_dir_path[] = "./tmp";
    char rlt_dir_path_new[] = "./tmp_new";
    char *str0 = (char *)"good luck, ";
    char *str1 = (char *)"have fun!";

    char buf0[4], buf1[4];
    struct iovec iov_w[2];
    struct iovec iov_r[2];

    char buf[2048];
    ssize_t total_size;
    ssize_t s_ret;
    int fd;
    int dird;
    int file_buf;
    int ret;
    long ret_l;
    DIR *dirp;
    struct dirent *p_dirent;
    struct stat *statbuf;
    struct pollfd fds[1];
    char *res;

    /** getopt value **/
    int argc = 3;
    char n_1[] = "./main";
    char n_2[] = "-a";
    char n_3[] = "test.txt";

    char *argv[3];
    argv[0] = n_1;
    argv[1] = n_2;
    argv[2] = n_3;

    /* time clock */
    struct timespec ts;
    struct timespec t_res;
    struct timespec times[2];
    struct stat statbuf_2;
    times[0] = statbuf_2.st_atim;
    times[1] = statbuf_2.st_mtim;
    struct timespec rqtp;
    struct timespec rmtp;
    rqtp.tv_sec = 0;
    rqtp.tv_nsec = 0;

    /** mkdirat **/
    /* mkdir tmp in current directory for test
     *     if the ./tmp directory has exits, mkdirat will fail
     */
    ret = mkdirat(AT_FDCWD, rlt_dir_path, 0755);
    if (ret == 0) {
        ocall_print("\tOperation mkdirat success.\n");
    }

    /* flags:
            0100:  - O_CREAT
            02 :  - O_RDWR */
    /** 1. open **/
    fd = open(file, O_RDWR);
    if (fd !=-1) {
        ocall_print("\tOperation open test_open.txt success.\n");
    }

    /** read **/
    total_size = read(fd, buf, 2048);
    if (total_size != -1) {
        ocall_print("\tOperation read test_open.txt success.\n");
        ocall_print("\t\tthe details of the file:  ");
        ocall_print(buf);
        ocall_print("\n");
    }

    /** lseek **/
    ret = lseek(fd, 1, SEEK_CUR);
    if (ret != -1) {
        ocall_print("\tOperation lseek success.\n");
    }

    /** ftruncate **/
    ret = ftruncate(fd, 0);
    if (ret == 0) {
        ocall_print("\tOperation ftruncate success.\n");
    }

    /** fdatasync **/
    ret = fdatasync(fd);
    if (ret == 0) {
        ocall_print("\tOperation fdatasync success.\n");
    }

     /** isatty **/
    ret = isatty(fd);
    if (ret == 0) {
        ocall_print("\tOperation fisatty success.\n");
    }

    /** fsync **/
    ret = fsync(fd);
    if (ret == 0) {
        ocall_print("\tOperation fsync success.\n");
    }

    /** 1. close **/
    ret = close(fd);
    if (ret != -1) {
        ocall_print("\tOperation close success.\n");
    }

    /*----------------------------------------------------------------*/

    /**-- DIR --**/
    /** fdopendir **/
    /* 2. open */
    dird = open(rlt_dir_path, O_RDONLY);
    dirp = fdopendir(dird);
    if (dirp != NULL) {
        ocall_print("\tOperation fdopendir success.\n");
    }

    /** readdir **/
    p_dirent = readdir(dirp);
    if (p_dirent != NULL) {
        ocall_print("\tOperation readdir success.\t");
        ocall_print(p_dirent -> d_name);
        ocall_print("\n");
    }

    /** rewinddir **/
    rewinddir(dirp);

    /** seekdir **/
    seekdir(dirp, 1);

    /** telldir **/
    ret_l = telldir(dirp);
    if (ret_l != -1) {
        ocall_print("\tOperation telldir success. \n");
    }

    /** closedir **/
    ret = closedir(dirp);
    if (ret == 0 ) {
        ocall_print("\tOperation closedir success. \n");
    }
    /* 2. close */
    close(dird);

    /*----------------------------------------------------------------*/

    /** fstat **/
    /** 3. open file firstly **/
    fd = open(file, O_RDWR);
    statbuf = (stat *)malloc(sizeof(stat));
    ret = fstat(fd, statbuf);
    if (ret == 0) {
        ocall_print("\tOperation fstat success. \n");
    }
    free(statbuf);
    /* 3. close */
    close(fd);

    /*----------------------------------------------------------------*/

    /** fstatat **/
    /* 4. open */
    dird = open(rlt_dir_path, O_RDONLY);
    ret = fstatat(AT_FDCWD, rlt_dir_path, statbuf, 0);
    if (ret == 0) {
        ocall_print("\tOperation fstatat success. \n");
    }

    /** renameat **/
    ret = renameat(AT_FDCWD, rlt_dir_path, AT_FDCWD, rlt_dir_path_new);
    if (ret == 0) {
        ocall_print("\tOperation renameat ./tmp to "
        "./tmp_new success. \n");
    }
    renameat(AT_FDCWD, rlt_dir_path_new, AT_FDCWD, rlt_dir_path);

    /** link **/
    ret = link(file, file_hd_ln);
    if (ret == 0) {
        ocall_print("\tOperation link success. \n");
    }

    /** unlinkat **/
    ret = unlinkat(AT_FDCWD, file_hd_ln, 0);
    if (ret == 0) {
        ocall_print("\tOperation unlinkat success. \n");
    }

    /** linkat **/
    ret = linkat(AT_FDCWD, file, AT_FDCWD, file_hd_ln, 0);
    if (ret == 0) {
        ocall_print("\tOperation linkat success. \n");
    }
    /* delete hard link file */
    unlinkat(AT_FDCWD, file_hd_ln, 0);

    /** symlinkat **/
    ret = symlinkat(file, AT_FDCWD, file_sf_ln);
    if (ret == 0) {
        ocall_print("\tOperation symlinkat from test.txt "
        "to text_sf_ln.txt success. \n");
    }
    /** readlinkat **/
    total_size = readlinkat(AT_FDCWD, file_sf_ln, buf, sizeof(buf));
    if (total_size != -1) {
        ocall_print("\tOperation readlinkat success. \n");
        ocall_print("\t\t the link details of the file is:  ");
        ocall_print(buf);
        ocall_print("\n");
    }
    /* delete soft link file */
    unlinkat(AT_FDCWD, file_sf_ln, 0);
    /* 4. close */
    close(dird);

    /*----------------------------------------------------------------*/

    /* 5. open */
    fd = open(file, O_RDWR);
    /** ioctl **/
    ret = ioctl(fd, FIONREAD, &file_buf);
    if (ret == 0) {
        ocall_print("\tOperation ioctl success. \n");
    }
    /** fcntl(fd, cmd) **/
    ret = fcntl(fd, F_GETFD);
    if (ret != 0 || ret != -1) {
        ocall_print("\tOperation fcntl_1 success. \n");
    }
    /** fcntl(fd, cmd, long) **/
    ret = fcntl(fd, F_SETFD, ret);
    if (ret != 0 || ret != -1) {
        ocall_print("\tOperation fcntl_2 success. \n");
    }

    /* 5. close */
    close(fd);

    /*----------------------------------------------------------------*/

    /** posix_fallocate **/
    /* 6. open */
    fd = open(file, O_RDWR);
    ret = posix_fallocate(fd, 1, 1);
    if (ret != 0 || ret != -1) {
        ocall_print("\tOperation posix_fallocate success. \n");
    }
    /* 6. close */
    close(fd);

    /** poll **/
    ret = poll(fds, 1, 10);
    if (ret != 0 || ret != -1) {
        ocall_print("\tOperation poll success. \n");
    }

    /** realpath **/
    res = realpath(file, res);
    if (res) {
        ocall_print("\tOperation realpath success. \n");
        ocall_print("\t\t the absolute path of the file is:  ");
        ocall_print(res);
        ocall_print("\n");
    }

    /** getrandom **/
    total_size = getrandom(buf, 1024, 0);
    if (ret != -1) {
        ocall_print("\tOperation getrandom success. \n");
    }

    /** writev **/
    /* 7. open */
    fd = open(file, O_RDWR);
    iov_w[0].iov_base = str0;
    iov_w[0].iov_len = strlen(str0);
    iov_w[1].iov_base = str1;
    iov_w[1].iov_len = strlen(str1);

    s_ret = writev(fd, iov_w, 2);
    if (s_ret != -1) {
        ocall_print("\tOperation writev success. \n");
    }

    /** readv **/
    iov_r[0].iov_base = buf0;
    iov_r[0].iov_len = sizeof(buf0) - 1;
    iov_r[1].iov_base = buf1;
    iov_r[1].iov_len = sizeof(buf1) - 1;

    s_ret = readv(fd, iov_r, 2);
    if (s_ret != -1) {
        ocall_print("\tOperation readv success. \n");
        ocall_print("\t\t");
        ocall_print(buf0);
        ocall_print(buf1);
        ocall_print("\n");
    }

    iov_r[0].iov_base = buf0;
    iov_r[0].iov_len = sizeof(buf0) - 1;
    iov_r[1].iov_base = buf1;
    iov_r[1].iov_len = sizeof(buf1) - 1;

    s_ret = preadv(fd, iov_r, 2, 2);
    if (s_ret != -1) {
        ocall_print("\tOperation readv success. \n");
        ocall_print("\t\t");
        ocall_print(buf0);
        ocall_print(buf1);
        ocall_print("\n");
    }
    /* 7. close */
    close(fd);

    /** getopt **/
    while((ret = getopt(argc, argv, "f:abc")) != -1){ //get option from the getopt() method
      switch(ret){
         //For option i, r, l, print that these are options
         case 'a':
         case 'b':
         case 'c':
            ocall_print("\tGiven Option operation success. \n");
            break;
         case 'f': //here f is used for some file name
            ocall_print("\tGiven File operation success.\n");
            break;
         case '?': //used for some unknown options
            ocall_print("\tunknown option trigger success.\n");
            break;
      }
   }

    /** sched_yield **/
    ret = sched_yield();
    if (ret == 0) {
        ocall_print("\tOperation sched_yield success. \n");
    }

    /** clock_gettime **/
    ret = clock_gettime(CLOCK_REALTIME, &ts);
    if (ret == 0) {
        ocall_print("\tOperation clock_gettime success. \n");
    }

    /** clock_getres **/
    ret = clock_getres(CLOCK_REALTIME, &t_res);
    if (ret == 0) {
        ocall_print("\tOperation clock_getres success. \n");
    }

    /** futimens **/
    /* 8. open */
    fd = open(file, O_RDWR);
    ret = futimens(fd, NULL);
    if (ret == 0) {
        ocall_print("\tOperation futimens NULL success. \n");
    }

    ret = futimens(fd, times);
    if (ret == 0) {
        ocall_print("\tOperation futimens times[2] success. \n");
    }
    /* 8. close */
    close(fd);

    /** utimensat **/
    /* 9. open */
    dird = open(rlt_dir_path, O_RDONLY);
    ret = utimensat(AT_FDCWD, file, NULL, AT_SYMLINK_NOFOLLOW);
    if (ret == 0) {
        ocall_print("\tOperation utimensat NULL success. \n");
    }

    ret = utimensat(AT_FDCWD, file, times, AT_SYMLINK_NOFOLLOW);
    if (ret == 0) {
        ocall_print("\tOperation utimensat times[2] success. \n");
    }
    /* 9. close */
    close(fd);

    /** clock_nanosleep **/
    ret = clock_nanosleep(CLOCK_REALTIME, 0, &rqtp, NULL);
    if (ret == 0) {
        ocall_print("\tOperation clock_nanosleep NULL success. \n");
    }

    ret = clock_nanosleep(CLOCK_REALTIME, 0, &rqtp, &rmtp);
    if (ret == 0) {
        ocall_print("\tOperation clock_nanosleep 2 success. \n");
    }

    ocall_print("\n<== ... End test\n");
}
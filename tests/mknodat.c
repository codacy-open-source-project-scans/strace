/*
 * Check decoding of mknodat syscall.
 *
 * Copyright (c) 2016-2023 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tests.h"
#include "scno.h"

#include <stdio.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

static const char sample[] = "mknodat_sample";
static const long int fd = (long int) 0xdeadbeefffffffffULL;

static long
call_mknodat(unsigned short mode, unsigned long dev)
{
	unsigned long lmode = (unsigned long) 0xffffffffffff0000ULL | mode;
	return syscall(__NR_mknodat, fd, sample, lmode, dev);
}

int
main(void)
{
	unsigned long dev = (unsigned long) 0xdeadbeefbadc0dedULL;

	long rc = call_mknodat(0, dev);
	printf("mknodat(-1, \"%s\", 000) = %s\n",
	       sample, sprintrc(rc));

	rc = call_mknodat(0xffff, dev);
	printf("mknodat(-1, \"%s\", %#03ho) = %s\n",
	       sample, (unsigned short) -1, sprintrc(rc));

	rc = call_mknodat(S_IFREG, 0);
	printf("mknodat(-1, \"%s\", S_IFREG|000) = %s\n",
	       sample, sprintrc(rc));

	rc = call_mknodat(S_IFDIR | 06, 0);
	printf("mknodat(-1, \"%s\", S_IFDIR|006) = %s\n",
	       sample, sprintrc(rc));

	rc = call_mknodat(S_IFLNK | 060, 0);
	printf("mknodat(-1, \"%s\", S_IFLNK|060) = %s\n",
	       sample, sprintrc(rc));

	rc = call_mknodat(S_IFIFO | 0600, 0);
	printf("mknodat(-1, \"%s\", S_IFIFO|0600) = %s\n",
	       sample, sprintrc(rc));

	dev = (unsigned long) 0xdeadbeef00000000ULL | makedev(1, 7);

	rc = call_mknodat(S_IFCHR | 024, dev);
	printf("mknodat(-1, \"%s\", S_IFCHR|024, makedev(0x1, 0x7)) = %s\n",
	       sample, sprintrc(rc));

	const unsigned short mode = (0xffff & ~S_IFMT) | S_IFBLK;
	dev = (unsigned long) 0xdeadbeefbadc0dedULL;

	rc = call_mknodat(mode, dev);
	printf("mknodat(-1, \"%s\", S_IFBLK|S_ISUID|S_ISGID|S_ISVTX|%#03ho"
	       ", makedev(%#x, %#x)) = %s\n",
	       sample, (short) (mode & ~(S_IFMT|S_ISUID|S_ISGID|S_ISVTX)),
	       major((unsigned) dev), minor((unsigned) dev),
	       sprintrc(rc));

	puts("+++ exited with 0 +++");
	return 0;
}

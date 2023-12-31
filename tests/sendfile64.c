/*
 * Copyright (c) 2015-2016 Dmitry V. Levin <ldv@strace.io>
 * Copyright (c) 2015-2023 The strace developers.
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tests.h"
#include "scno.h"

#ifdef __NR_sendfile64

# include <assert.h>
# include <errno.h>
# include <fcntl.h>
# include <stdio.h>
# include <stdint.h>
# include <stdlib.h>
# include <unistd.h>
# include <sys/socket.h>

int
main(void)
{
	(void) close(0);
	if (open("/dev/zero", O_RDONLY) != 0)
		perror_msg_and_skip("open: %s", "/dev/zero");

	int sv[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv))
		perror_msg_and_skip("socketpair");

	const unsigned int page_size = get_page_size();
	assert(syscall(__NR_sendfile64, 0, 1, NULL, page_size) == -1);
	if (EBADF != errno)
		perror_msg_and_skip("sendfile64");
	printf("sendfile64(0, 1, NULL, %u)" RVAL_EBADF, page_size);

	unsigned int file_size = 0;
	socklen_t optlen = sizeof(file_size);
	if (getsockopt(sv[1], SOL_SOCKET, SO_SNDBUF, &file_size, &optlen))
		perror_msg_and_fail("getsockopt");
	if (file_size < 1024)
		error_msg_and_skip("SO_SNDBUF too small: %u", file_size);

	file_size /= 4;
	if (file_size / 16 > page_size)
		file_size = page_size * 16;
	const unsigned int blen = file_size / 3;
	const unsigned int alen = file_size - blen;

	int reg_in = create_tmpfile(O_RDWR);
	if (ftruncate(reg_in, file_size))
		perror_msg_and_fail("ftruncate(%d, %u)", reg_in, file_size);

	TAIL_ALLOC_OBJECT_CONST_PTR(uint64_t, p_off);
	void *p = p_off + 1;
	*p_off = 0;

	assert(syscall(__NR_sendfile64, 0, 1, p, page_size) == -1);
	printf("sendfile64(0, 1, %p, %u)" RVAL_EFAULT, p, page_size);

	assert(syscall(__NR_sendfile64, sv[1], reg_in, NULL, alen)
	       == (long) alen);
	printf("sendfile64(%d, %d, NULL, %u) = %u\n",
	       sv[1], reg_in, alen, alen);

	assert(syscall(__NR_sendfile64, sv[1], reg_in, p_off, alen)
	       == (long) alen);
	printf("sendfile64(%d, %d, [0] => [%u], %u) = %u\n",
	       sv[1], reg_in, alen, alen, alen);

	assert(syscall(__NR_sendfile64, sv[1], reg_in, p_off, file_size + 1)
	       == (long) blen);
	printf("sendfile64(%d, %d, [%u] => [%u], %u) = %u\n",
	       sv[1], reg_in, alen, file_size, file_size + 1, blen);

	*p_off = 0xcafef00dfacefeedULL;
	assert(syscall(__NR_sendfile64, sv[1], reg_in, p_off, 1) == -1);
	printf("sendfile64(%d, %d, [14627392582579060461], 1)"
		RVAL_EINVAL, sv[1], reg_in);

	*p_off = 0xfacefeed;
	assert(syscall(__NR_sendfile64, sv[1], reg_in, p_off, 1) == 0);
	printf("sendfile64(%d, %d, [4207869677], 1) = 0\n", sv[1], reg_in);

	puts("+++ exited with 0 +++");
	return 0;
}

#else

SKIP_MAIN_UNDEFINED("__NR_sendfile64")

#endif

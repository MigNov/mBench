/*
 * disk_io.c: Disk I/O benchmarking functions
 *
 * Author: Michal Novotny <minovotn@redhat.com>
 *
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */

#include "utils.h"

char diskio_prefix[1024] = "/tmp";

void disk_set_temp(char *tmp)
{
	strncpy(diskio_prefix, tmp, sizeof(diskio_prefix));
}

int disk_drop_caches()
{
	int fd, rc = -EPERM;

	fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
	if (fd > 0) {
		write(fd, "3", 1);
		fsync(fd);
		close(fd);
		rc = 0;
	}

	return rc;
}

float disk_throughput_putc(long size, float *fcpu)
{
	FILE *fp;
	char filename[1024];
	unsigned long long start, words;
	float tm = 0.0, cpu_start = 0.0;

	snprintf(filename, sizeof(filename), "%s/benchmark.%d.tmp", diskio_prefix, getpid());
	unlink(filename);

	disk_drop_caches();
	fp = fopen(filename, "w");
	if (fp == NULL)
		return 0;

	cpu_start = cpu_time_get();
	start = nanotime();

	for (words = 0; words < size; words++)
		if (putc(words & 0x7f, fp) == EOF)
			return 0;

	tm = (nanotime() - start) / 1000000.0;
	if (fcpu != NULL)
		*fcpu = calc_cpu_usage(cpu_time_get() - cpu_start, tm);

	fclose(fp);

	return (size / tm);
}

float disk_throughput_write(int chunkSize, long size, float *fcpu)
{
	int fd;
	char filename[1024];
	unsigned long long start, words;
	float tm = 0.0, cpu_start = 0.0;
	int flags, direct = 1;
	void *buf;
	long pagesize = 1;

	snprintf(filename, sizeof(filename), "%s/benchmark.%d.tmp", diskio_prefix, getpid());
	unlink(filename);

	disk_drop_caches();

	flags = O_WRONLY | O_CREAT;
	if (direct)
		flags |= O_DIRECT;
	fd = open(filename, flags, 0777);
	if (fd == -1)
		return 0;

	if (direct) {
		pagesize = sysconf(_SC_PAGE_SIZE);
		if (pagesize < 0)
			return 0;

		if (posix_memalign(&buf, pagesize, chunkSize) != 0)
			return 0;
	}
	else
		buf = malloc( chunkSize );

	cpu_start = cpu_time_get();
	start = nanotime();

	for (words = 0; words < size; words += chunkSize) {
		if (write(fd, buf, chunkSize) < 0)
			return 0;
	}

	tm = (nanotime() - start) / 1000000.0;
	if (fcpu != NULL)
		*fcpu = calc_cpu_usage(cpu_time_get() - cpu_start, tm);

	free(buf);
	close(fd);

	return (size / tm);
}

float disk_benchmark_create(int numFiles, float *fcpu)
{
	int fd, i, created;
	char filename[1024];
	unsigned long long start;
	float tm = 0.0, cpu_start = 0.0;
	double time_total = 0.0, cpu_total = 0.0;

	disk_drop_caches();

	cpu_start = cpu_time_get();
	start = nanotime();

	created = 0;
	for (i = 0; i < numFiles; i++) {
			snprintf(filename, sizeof(filename), "%s/benchmark-create-%ld-%ld-%ld.tmp", diskio_prefix,
					(long)getpid(), (long)random(), (long)random() & 0xFF);
			cpu_start = cpu_time_get();
			start = nanotime();
			if ((fd = creat(filename, 0777)) > 0)
				created++;
			cpu_total += (cpu_time_get() - cpu_start);
			time_total += (nanotime() - start);
			close(fd);
	}

	tm = time_total / 1000000.0;
	if (fcpu != NULL)
		*fcpu = calc_cpu_usage(cpu_total, tm);

	return (created / tm);
}

float disk_benchmark_delete(long *numFiles, float *fcpu)
{
	long deleted;
	char filename[1024];
	unsigned long long start;
	float tm = 0.0, cpu_start = 0.0;
	double time_total = 0.0, cpu_total = 0.0;
	DIR *d;
	struct dirent *de;

	disk_drop_caches();

	deleted = 0;
	d = opendir(diskio_prefix);
	while ((de = readdir(d)) != NULL) {
		if (strstr(de->d_name, "benchmark-create") != NULL) {
			snprintf(filename, sizeof(filename), "%s/%s", diskio_prefix, de->d_name);
			cpu_start = cpu_time_get();
			start = nanotime();
			unlink(filename);
			cpu_total += (cpu_time_get() - cpu_start);
			time_total += (nanotime() - start);
			
			deleted++;
		}
	}
	tm = time_total / 1000000.0;
	if (fcpu != NULL)
		*fcpu = calc_cpu_usage(cpu_total, tm);
	closedir(d);
	if (numFiles != NULL)
		*numFiles = deleted;

	return (deleted / tm);
}

float disk_throughput_read(long chunkSize, float *fcpu)
{
	int fd;
	char filename[1024];
	float tm = 0.0, ret = 0.0, cpu_start = 0.0;
	unsigned long long num, start, total;
	void *aligned_buffer;
	long pagesize;

	disk_drop_caches();
	snprintf(filename, sizeof(filename), "%s/benchmark.%d.tmp", diskio_prefix, getpid());
	fd = open(filename, O_RDONLY | O_DIRECT);
	if (fd == -1)
		return 0;

	pagesize = sysconf(_SC_PAGE_SIZE);
	if (pagesize < 0)
		return 0;

	if (posix_memalign(&aligned_buffer, pagesize, chunkSize) != 0)
		return 0;

	total = 0;
	cpu_start = cpu_time_get();
	start = nanotime();
	while ((num = read(fd, aligned_buffer, chunkSize)) > 0) {
		if (num == -1) {
			printf("ERRNO: %d\n" ,errno);
			break;
		}
		total += num;
	}
	tm = (nanotime() - start) / 1000000.0;
	if (fcpu != NULL)
		*fcpu = calc_cpu_usage(cpu_time_get() - cpu_start, tm);

	ret = ((float)total / tm);

	free(aligned_buffer);

	close(fd);

	return ret;
}

float disk_throughput_read_random(long chunkSize, int numSequences, float *fcpu)
{
	int fd, flags;
	char filename[1024];
	float tm = 0.0, ret = 0.0, cpu_start = 0.0;
	unsigned long long start, ok;
	void *buf;
	long size, off, pagesize = 1;
	int direct = 1;

	disk_drop_caches();
	snprintf(filename, sizeof(filename), "%s/benchmark.%d.tmp", diskio_prefix, getpid());
	flags = O_RDONLY;
	if (direct)
		flags |= O_DIRECT;
	fd = open(filename, flags);
	if (fd == -1)
		return 0;

	size = lseek(fd, 0, SEEK_END);

	if (numSequences == -1)
		numSequences = (size / chunkSize);

	if (direct) {
		pagesize = sysconf(_SC_PAGE_SIZE);
		if (pagesize < 0)
			return 0;

		if (posix_memalign(&buf, pagesize, chunkSize) != 0)
			return 0;
	}
	else
		buf = malloc( chunkSize );

	ok = 0;
	cpu_start = cpu_time_get();
	start = nanotime();
	while (ok < numSequences) {
		int x;

		off = random() % size;
		if (off > size - chunkSize)
			off -= chunkSize;
		if (lseek(fd, off * pagesize, SEEK_SET) > -1) {
			if ((x = read(fd, buf, chunkSize)) > 0)
				ok++;
		}		
	}

	tm = (nanotime() - start) / 1000000.0;
	if (fcpu != NULL)
		*fcpu = calc_cpu_usage(cpu_time_get() - cpu_start, tm);

	ret = ((float)(ok * chunkSize) / tm);

	free(buf);

	close(fd);

	return ret;
}

void disk_temp_cleanup()
{
	char filename[1024];

	snprintf(filename, sizeof(filename), "%s/benchmark.%d.tmp", diskio_prefix, getpid());
	unlink(filename);
}


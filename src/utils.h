/*
 * utils.h: mBench headers
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

#define _GNU_SOURCE
#define _USE_GNU

#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <sched.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <netdb.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* Output format defines */
#define FORMAT_PLAIN					1
#define FORMAT_CSV						2
#define FORMAT_XML						4

/* CPU defines */
#define PROCESSOR_COUNT_TOTAL(mask)		(mask & 0xffff)
#define PROCESSOR_COUNT_ONLINE(mask)	(mask >> 16)

/* Memory defines */
#define MEMTYPE_B						1
#define MEMTYPE_KB						2
#define MEMTYPE_MB						4
#define MEMTYPE_GB						8

#define DISPLAY_VARMEM(var) memory_display((char*) &( var ), (sizeof( var )));
#define DISPLAY_PTRMEM(ptr) memory_display((char*) ( ptr ), (sizeof( *ptr )));

/* Disk and network operation defines */
#define DISK_OP_PUTC(type)				((type != FORMAT_PLAIN) ? "putc" : "Putc")
#define DISK_OP_WRITE(type)				((type != FORMAT_PLAIN) ? "write" : "Write")
#define DISK_OP_READ(type)				((type != FORMAT_PLAIN) ? "read" : "Read")
#define DISK_OP_READ_RANDOM(type)		((type != FORMAT_PLAIN) ? "read-random" : "Read random")
#define DISK_OP_FILE_CREATE(type)		((type != FORMAT_PLAIN) ? "file-create" : "File create")
#define DISK_OP_FILE_DELETE(type)		((type != FORMAT_PLAIN) ? "file-delete" : "File delete")
#define NET_OP_READ(type)				((type != FORMAT_PLAIN) ? "network-read" : "Network read")

/* Network defines */
#define	NET_IPV4						1
#define NET_IPV6						2

/* Common functions */
unsigned long long	nanotime(void);
int					io_get_size(unsigned long long size, int prec, char *sizestr, int maxlen);
int					io_get_size_double(double size, int prec, char *sizestr, int maxlen);
double				cpu_time_get();
float				calc_cpu_usage(float cpu, float tm);

/* CPU functions */
int 		cpu_affinity_set(pid_t pid, uint64_t cmask);
int 		cpu_affinity_get(pid_t pid, uint64_t *cmask);
float 		cpu_get_speed_mhz(void);
void 		cpu_dhrystone_get(unsigned long *dhry, unsigned long *loops, unsigned long *tm);
void	 	cpu_whetstone_get(unsigned long *loops, unsigned long *iter, unsigned long *btime, unsigned long *mips);
int 		get_linpack_score(int arsize, unsigned long *memory, float *minMFLOPS, float *maxMFLOPS, float *avgMFLOPS);
uint32_t 	cpu_get_count(void);

/* Memory functions */
float 		memory_size_get(int type);
void 		memory_display(char *address, int length);

/* Disk I/O functions */
void 		disk_set_temp(char *tmp);
int 		disk_drop_caches();
float 		disk_throughput_write(int chunkSize, long size, float *fcpu);
float 		disk_throughput_putc(long size, float *fcpu);
float		disk_throughput_read(long chunkSize, float *fcpu);
float 		disk_throughput_read_random(long chunkSize, int numSequences, float *fcpu);
void 		disk_temp_cleanup();
float		disk_benchmark_create(int numFiles, float *fcpu);
float		disk_benchmark_delete(long *numFiles, float *fcpu);

/* Network I/O function */
void		net_set_host(char *val, int port_only);
char*		net_get_bound_addr();
char*		net_get_connect_addr();
int 		net_listen(int port, int type);
int			net_connect(char *host, int port, int type);
int			net_write_command(int sock, unsigned long long size, unsigned long chunksize, unsigned long bufsize, float *otm, float *fcpu);
int			net_server_terminate(int sock);

/* Resultset types */
typedef struct {
	char operation[16];
	unsigned long long size;
	unsigned long chunk_size;
	double throughput;
	float cpu_usage;
} tIOResults;

typedef struct {
	float run_time;
	int cpus;
	uint64_t cpumask;
	int cpu_total;
	int cpu_online;
	float cpu_speed;
	float cpu_dhrystone;
	unsigned long cpu_whetstone;
	int cpu_linpack_size;
	long cpu_linpack_mem;
	float cpu_linpack_min;
	float cpu_linpack_max;
	float cpu_linpack_avg;
	float memory_size;
	int disk_drop_caches;
	int disk_res_size;
	tIOResults *disk;
	int net_res_size;
	tIOResults *net;
} tResults;

tResults *results;

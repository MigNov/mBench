/*
 * main.c: mBench benchmarking tool core functionality
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

//#define DEBUG
/* Experimental memory dump support */
//#define ENABLE_MEM_DUMP
#include "utils.h"

#ifdef DEBUG
#define DPRINTF(fmt, ...) \
do { fprintf(stderr, "debug: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
do {} while(0)
#endif

#define FLAG_CA_GET		0x01
#define FLAG_CA_SET		0x02
#define FLAG_CPU_GET		0x04
#define FLAG_CPU_SPEED		0x08
#define FLAG_MEM_GET		0x10
#define FLAG_MEM_DUMP		0x20
#define FLAG_CPU_DHRYSTONE	0x40
#define FLAG_CPU_WHETSTONE	0x80
#define FLAG_CPU_LINPACK	0x100
#define FLAG_DISK_STAT		0x200
#define FLAG_NETS_STAT		0x400
#define FLAG_NETC_STAT		0x800

int lpArrSize = 0;
unsigned long long dioBufSize = 0;
unsigned long long nioBufSize = 0;
int prec = 2;
char tempDir[1024] = { 0 };
int outType = FORMAT_PLAIN;

struct option options[] = {
	{ "cpu-affinity-get", 0, NULL, 'g' },
	{ "cpu-affinity-set", 1, NULL, 's' },
	{ "cpu-get", 0, NULL, 'c' },
	{ "cpu-get-speed", 0, NULL, 'p' },
	{ "cpu-get-dhrystone", 0, NULL, 'h' },
	{ "cpu-get-whetstone", 0, NULL, 'w' },
	{ "cpu-get-linpack", 1, NULL, 'l' },
	{ "memory-get", 0, NULL, 'm' },
#ifdef ENABLE_MEM_DUMP
	{ "memory-dump", 0, NULL, 'd' },
#endif
	{ "disk-get-benchmark", 1, NULL, 'i' },
	{ "net-benchmark-server", 1, NULL, 'n' },
	{ "net-benchmark-client", 1, NULL, 'e' },
	{ "temp-dir", 1, NULL, 't' },
	{ "precision", 1, NULL, 'r' },
	{ "cpu-all", 1, NULL, 'u' },
	{ "format", 1, NULL, 'f' },
	{ NULL, 0, NULL, 0 } };

void usage(char *name)
{
	printf(	"Syntax: %s [options]\n\n"
			"where option could be one of following:\n\n"
			"\t--cpu-affinity-get                     get the CPU affinity\n"
			"\t--cpu-affinity-set <mask>              set the CPU affinity to <mask>\n"
			"\t--cpu-get                              get the CPU information (total/online)\n"
			"\t--cpu-get-speed                        get (measure) the CPU speed\n"
			"\t--cpu-get-dhrystone                    get the CPU Dhrystone in DMIPS\n"
			"\t--cpu-get-whetstone                    get the CPU Whetstone in MIPS\n"
			"\t--cpu-get-linpack <size>               get the CPU Linpack score for specified array size (e.g. 200)\n"
			"\t--cpu-all <linpack-size>               perform all CPU-related tests and also get memory size\n"
			"\t--memory-get                           get the physical memory size\n"
#ifdef ENABLE_MEM_DUMP
			"\t--memory-dump                          dump the memory\n"
#endif
			"\t--precision <precision>                set precision value for floating point values (default value: 2)\n"
			"\t--temp-dir <dir>                       set the temporary directory to <dir>\n"
			"\t--disk-get-benchmark <size>            get the benchmark statistics for disk I/O for test data of <size> (supports k, M, G suffixes)\n"
			"\t--net-benchmark-server :<port>         create the server on local <port> for network I/O benchmarking\n"
			"\t--net-benchmark-client <host> <size>   connect to host[:port] defined by <host> to perform I/O benchmarking for <size> (k, M, G suffixed)\n"
			"\t--format <format>                      output results in <format> (can be plain, xml or csv)\n\n",
			name);
}

unsigned long long argvToSize(char *arg)
{
	int unit, multiplicator = 1, nodel = 0;

	if ((arg == NULL) || (strlen(arg) == 0))
		return 0;

	unit = arg[strlen(arg)-1];
	switch (arg[strlen(arg)-1]) {
		case 'k':
		case 'K':
			multiplicator = 1 << 10;
			break;
		case 'M':
			multiplicator = 1 << 20;
			break;
		case 'G':
			multiplicator = 1 << 30;
			break;
		default:
			nodel = 1;
	}

	if (nodel == 0)
		arg[strlen(arg) - 1] = 0;

	return atoi(arg) * multiplicator;
}

int parse_args(int argc, char *argv[])
{
	int opt, idx = 0, flags = 0;
	unsigned long long val = 0;
#ifdef ENABLE_MEM_DUMP
	unsigned long long cnt = 0;
#endif

	while ((opt = getopt_long(argc, argv, "s:gcpmdhl:", options, NULL)) != -1) {
		idx++;
		switch (opt) {
			case 't':
					strncpy(tempDir, optarg, sizeof(tempDir));
					break;
			case 'g':
					flags |= FLAG_CA_GET;
					break;
			case 's':
					if (strncmp(optarg, "0x", 2) == 0)
						val = strtoull(optarg, NULL, 16);
					else
						val = strtoull(optarg, NULL, 10);

					if (val > 0)
						DPRINTF("Setting up CPU mask %d: %s\n", (int)val,
							(cpu_affinity_set(getpid(), (int)val) == 0) ? "OK" : "FAIL");
					flags |= FLAG_CA_SET;
					break;
			case 'c':
					flags |= FLAG_CPU_GET;
					break;
			case 'p':
					flags |= FLAG_CPU_SPEED;
					break;
			case 'm':
					flags |= FLAG_MEM_GET;
					break;
			case 'h':
					flags |= FLAG_CPU_DHRYSTONE;
					break;
			case 'f':
					if (strcmp(optarg, "plain") == 0)
						outType = FORMAT_PLAIN;
					else
					if (strcmp(optarg, "xml") == 0)
						outType = FORMAT_XML;
					else
					if (strcmp(optarg, "csv") == 0)
						outType = FORMAT_CSV;
					else {
						fprintf(stderr, "Invalid output format type, valid formats are: plain, xml, csv\n");
						exit(1);
					}
					break;
			case 'w':
					flags |= FLAG_CPU_WHETSTONE;
					break;
			case 'l':
					flags |= FLAG_CPU_LINPACK;
					if (optarg != NULL)
						lpArrSize = atoi(optarg);
					break;
			case 'r':
					if (optarg != NULL)
						prec = atoi(optarg);
					break;
			case 'i':
					flags |= FLAG_DISK_STAT;
					if (optarg != NULL)
						dioBufSize = argvToSize(optarg);
					break;
			case 'n':
					flags |= FLAG_NETS_STAT;
					if (optarg != NULL)
						net_set_host(optarg, 1);
					break;
			case 'u':
					flags |= FLAG_CA_GET;
					flags |= FLAG_CPU_GET;
					flags |= FLAG_CPU_SPEED;
					flags |= FLAG_CPU_DHRYSTONE;
					flags |= FLAG_CPU_WHETSTONE;
					flags |= FLAG_MEM_GET;
					flags |= FLAG_CPU_LINPACK;
					if (optarg != NULL)
						lpArrSize = atoi(optarg);
					break;
			case 'e':
					if ((argc < idx + 2) || (argv[idx + 2] == NULL)) {
						fprintf(stderr, "Error: Network I/O buffer is missing.\n"
								"Syntax: %s --net-benchmark-client <host> <size>\n\n",
								argv[0]);
						break;
					}
					flags |= FLAG_NETC_STAT;
					if (optarg != NULL)
						net_set_host(optarg, 0);
					nioBufSize = argvToSize(argv[idx + 2]);
					break;
#ifdef ENABLE_MEM_DUMP
			case 'd':
					if (argc < idx + 3) {
						fprintf(stderr, "Error: Cannot dump memory since required arguments are not found.\n"
								"Syntax: %s --dump-memory start_memory memory_size_to_dump\n\n",
								argv[0]);
						break;
					}
					if (strncmp(argv[idx + 1], "0x", 2) == 0)
						val = strtoull(argv[idx + 1], NULL, 16);
					else
						val = strtoull(argv[idx + 1], NULL, 10);

					if (strncmp(argv[idx + 2], "0x", 2) == 0)
						cnt = strtoull(argv[idx + 2], NULL, 16);
					else
						cnt = strtoull(argv[idx + 2], NULL, 10);

					DPRINTF("Dumping memory from 0x%llx to 0x%llx\n", val, val + cnt);
					memory_display((char *)val, cnt);
					flags |= FLAG_MEM_DUMP;
					break;
#endif
		}
	}

	if (flags == 0) {
		usage(argv[0]);
		exit(1);
	}

	return flags;
}

void get_affinity(void)
{
	char out[1024] = { 0 }, tmp[32] = { 0 };
	int totalCpuCount, i, num = 0;
	uint64_t cmask;
	totalCpuCount = cpu_affinity_get(getpid(), &cmask);

	DPRINTF("This process is running on following cpu(s): ");

	for (i = 0; i < totalCpuCount; i++) {
		if (cmask & (1 << i)) {
			snprintf(tmp, sizeof(tmp), "CPU #%d, ", i);
			strncat(out, tmp, sizeof(out));
			num++;
		}
	}

	results->cpumask = cmask;
	results->cpus = num;

	out[strlen(out) - 2] = 0;
	DPRINTF("%s\n", out);
}

#define DISK_ACTION_ON_ARRAY(prec, msg, funcName) \
i = 0; \
num = 0; \
while ((num < size) && (i < dioBufNum)) { \
	float res = 0.0, fcpu = 0.0;	\
	char size_num[16] = { 0 }, size_res[16] = { 0 }; \
	num = dioBufferArray[i++] * (1 << 10);	\
	res = funcName(num, &fcpu); \
	\
	strncpy(results->disk[dIdx].operation, msg, sizeof(results->disk[dIdx].operation)); \
	results->disk[dIdx].size = size; \
	results->disk[dIdx].throughput = res; \
	results->disk[dIdx].chunk_size = num; \
	results->disk[dIdx++].cpu_usage = fcpu; \
	\
	io_get_size((unsigned long long)num, 0, size_num, 16); \
	io_get_size_double(res, prec, size_res, 16); \
	DPRINTF("%s (%s blocks): %s/s (CPU %.*f%%)\n", msg, size_num, size_res, prec, fcpu);	\
}

#define DISK_ACTION_ON_ARRAY_PARAM(prec, msg, funcName, param) \
i = 0; \
num = 0; \
while ((num < size) && (i < dioBufNum)) { \
	float res = 0.0, fcpu = 0.0;	\
	char size_num[16] = { 0 }, size_res[16] = { 0 }; \
	num = dioBufferArray[i++] * (1 << 10);	\
	res = funcName(num, param, &fcpu); \
	\
	strncpy(results->disk[dIdx].operation, msg, sizeof(results->disk[dIdx].operation)); \
	results->disk[dIdx].size = size; \
	results->disk[dIdx].throughput = res; \
	results->disk[dIdx].chunk_size = num; \
	results->disk[dIdx++].cpu_usage = fcpu; \
	\
	io_get_size((unsigned long long)num, 0, size_num, 16); \
	io_get_size_double(res, prec, size_res, 16); \
	DPRINTF("%s (%s blocks): %s/s (CPU %.*f%%)\n", msg, size_num, size_res, prec, fcpu);	\
}

void disk_io_process(unsigned long long size, long files)
{
	long dioBufferArray[6] = { 16, 128, 256, 512, 1024, 4096 };
	char size_fres[16] = { 0 };
	int i, err, dioBufNum, dIdx = 0;
	unsigned long long num = 0;
	float fres = 0.0, fcpu = 0.0;

	if (strlen(tempDir) > 0)
		disk_set_temp(tempDir);

	dioBufNum = sizeof(dioBufferArray) / sizeof(dioBufferArray[0]);

	if ((err = disk_drop_caches()) != 0)
		fprintf(stderr, "Warning: Cannot drop caches (%s). Values *may not* be reliable!\n", strerror(-err));
	else
		results->disk_drop_caches = 1;

	io_get_size(size, 0, size_fres, 16);
	DPRINTF("Running benchmark for %s pattern\n", size_fres);

	results->disk_res_size =  (3 * dioBufNum) + 3;
	results->disk = (tIOResults *)malloc( results->disk_res_size * sizeof(tIOResults) );
	memset(results->disk, 0, results->disk_res_size);

	fprintf(stderr, "Disk: Getting %d results, this may take some time\n", results->disk_res_size);
	fres = disk_throughput_putc(size, &fcpu);

	strncpy(results->disk[dIdx].operation, DISK_OP_PUTC(outType), sizeof(results->disk[dIdx].operation));
	results->disk[dIdx].size = size;
	results->disk[dIdx].throughput = fres;
	results->disk[dIdx].chunk_size = 1;
	results->disk[dIdx++].cpu_usage = fcpu;

	io_get_size_double(fres, prec, size_fres, 16);
	DPRINTF("Putc results: %s/s (CPU %.*f%%)\n", size_fres, prec, fcpu);

	DISK_ACTION_ON_ARRAY_PARAM(prec, DISK_OP_WRITE(outType), disk_throughput_write, size);
	DISK_ACTION_ON_ARRAY(prec, DISK_OP_READ(outType), disk_throughput_read);
	DISK_ACTION_ON_ARRAY_PARAM(prec, DISK_OP_READ_RANDOM(outType), disk_throughput_read_random, -1);

	fres = disk_benchmark_create(files, &fcpu);
	DPRINTF("Create benchmark: %.*f files/s (CPU %.*f%%)\n", prec, fres, prec, fcpu);

	strncpy(results->disk[dIdx].operation, DISK_OP_FILE_CREATE(outType), sizeof(results->disk[dIdx].operation));
	results->disk[dIdx].size = files;
	results->disk[dIdx].throughput = fres;
	results->disk[dIdx].chunk_size = 1;
	results->disk[dIdx++].cpu_usage = fcpu;

	fres = disk_benchmark_delete(&files, &fcpu);
	DPRINTF("Delete benchmark: %.*f files/s (CPU %.*f%%)\n", prec, fres, prec, fcpu);

	strncpy(results->disk[dIdx].operation, DISK_OP_FILE_DELETE(outType), sizeof(results->disk[dIdx].operation));
	results->disk[dIdx].size = files;
	results->disk[dIdx].throughput = fres;
	results->disk[dIdx].chunk_size = 1;
	results->disk[dIdx++].cpu_usage = fcpu;

	disk_temp_cleanup();
}

void net_io_process(int server)
{
	unsigned long nioBufferArray[6] = { 4, 128, 512, 1024, 2048, 4096 };				// in kB
	int err;

	if (server) {
		printf("Listening on %s\n", net_get_bound_addr());
		if ((err = net_listen(0, NET_IPV4)) < 0)
			DPRINTF("Error listening on %s: %s\n", net_get_bound_addr(), strerror(-err));

		DPRINTF("Server run complete...\n");
	}
	else {
		int sock;

		fprintf(stderr, "Trying connection to %s\n", net_get_connect_addr());
		sock = net_connect(NULL, 0, NET_IPV4);
		if (sock <= 0)
			fprintf(stderr, "Connection to %s failed: %s\n", net_get_connect_addr(), strerror(-sock));
		else {
			int ii, nioIdx;
			float tm = 0.0, cpu = 0.0;

			results->net_res_size = (sizeof(nioBufferArray) / sizeof(nioBufferArray[0]));

			fprintf(stderr, "Network: Getting %d results, this may take some time\n", results->net_res_size);

			results->net = (tIOResults *)malloc( results->net_res_size * sizeof(tIOResults) );
			nioIdx = 0;

			for (ii = 0; ii < (sizeof(nioBufferArray) / sizeof(nioBufferArray[0])); ii++) {
				unsigned long long total = 0, chunk = 0;
				char size_total[16] = { 0 }, size_chunk[16] = { 0 }, size_thp[16] = { 0 };

				total = nioBufSize;
				chunk = (unsigned long long)nioBufferArray[ii] * (1 << 10);
				net_write_command(sock, total, chunk, chunk, &tm, &cpu);

				strncpy(results->net[nioIdx].operation, NET_OP_READ(outType), sizeof(results->net[nioIdx].operation));
				results->net[nioIdx].size = total;
				results->net[nioIdx].throughput = total / tm;
				results->net[nioIdx].chunk_size = chunk;
				results->net[nioIdx++].cpu_usage = cpu;

				io_get_size(total, 0, size_total, 16);
				io_get_size(chunk, 0, size_chunk, 16);
				io_get_size_double(total / tm, prec, size_thp, 16);
				DPRINTF("Network benchmark on %s with buffer size %s: %s/s (CPU: %.*f%%)\n", size_total, size_chunk, size_thp, prec, cpu);
			}

			if (net_server_terminate(sock))
				DPRINTF("Server socket terminated\n");
		}
	}
}

void printResults(int flags, int type)
{
	char tmp[16];
	int i;

	if (type == FORMAT_PLAIN) {
		if (flags & FLAG_CA_GET) {
			printf("Processors used: %d\n", results->cpus);
			printf("Processor mask: 0x%" PRIx64 "\n", results->cpumask);
		}
		if (flags & FLAG_CPU_GET) {
			printf("Processors total: %d\n", results->cpu_total);
			printf("Processors online: %d\n", results->cpu_online);
		}
		if (flags & FLAG_CPU_SPEED)
			printf("Processor speed: %.*f MHz\n", prec, results->cpu_speed);
		if (flags & FLAG_CPU_DHRYSTONE)
			printf("Processor's Dhrystone: %.*f DMIPS\n", prec, results->cpu_dhrystone);
		if (flags & FLAG_CPU_WHETSTONE)
			printf("Processor's Whetstone: %ld MIPS\n", results->cpu_whetstone);
		if (flags & FLAG_CPU_LINPACK) {
			printf("Processor's LinPack 2D array score:\n");
			printf("\tArray size: %dx%d\n", results->cpu_linpack_size, results->cpu_linpack_size);
			io_get_size(results->cpu_linpack_mem, prec, tmp, 16);
			printf("\tMemory used: %s\n", tmp);
			printf("\tMinimum score: %.*f MFLOPS\n", prec, results->cpu_linpack_min);
			printf("\tMaximum score: %.*f MFLOPS\n", prec, results->cpu_linpack_max);
			printf("\tAverage score: %.*f MFLOPS\n", prec, results->cpu_linpack_avg);
		}
		if (flags & FLAG_MEM_GET) {
			io_get_size_double(results->memory_size, prec, tmp, 16);
			printf("Memory size: %s\n", tmp);
		}
		if (flags & FLAG_DISK_STAT) {
			printf("Disk:\n");
			printf("\tCan drop caches: %s\n", results->disk_drop_caches ? "True" : "False");
			printf("\tResults:\n");

			for (i = 0; i < results->disk_res_size; i++) {
				if (strncmp(results->disk[i].operation, "File", 4) != 0) {
					char tmp2[16] = { 0 }, tmpChunk[16] = { 0 };
					io_get_size(results->disk[i].size, 0, tmp, 16);
					io_get_size(results->disk[i].chunk_size, 0, tmpChunk, 16);
					io_get_size_double(results->disk[i].throughput, prec, tmp2, 16);
					printf("\t\t%s of %s with %s buffer: %s/s (CPU %.*f%%)\n", results->disk[i].operation, tmp,
							tmpChunk, tmp2, prec, results->disk[i].cpu_usage);
				}
				else {
					printf("\t\t%s of %lld files: %.*f files/s (CPU %.*f%%)\n", results->disk[i].operation,
							results->disk[i].size, prec, results->disk[i].throughput, prec, results->disk[i].cpu_usage);
				}
			}
		}
		if (flags & FLAG_NETC_STAT) {
			int i;

			printf("Network results:\n");

			for (i = 0; i < results->net_res_size; i++) {
					char tmp2[16] = { 0 }, tmpChunk[16] = { 0 };
					io_get_size(results->net[i].size, 0, tmp, 16);
					io_get_size(results->net[i].chunk_size, 0, tmpChunk, 16);
					io_get_size_double(results->net[i].throughput, prec, tmp2, 16);
					printf("\t%s of %s with %s buffer: %s/s (CPU %.*f%%)\n", results->net[i].operation, tmp,
							tmpChunk, tmp2, prec, results->net[i].cpu_usage);
			}
		}

		printf("Total run time: %.*f seconds\n", prec, results->run_time);
	}
	else
	if (type == FORMAT_CSV) {
		if ((flags & FLAG_CA_GET) || (flags & FLAG_CPU_GET) || (flags & FLAG_CPU_SPEED) || (flags & FLAG_CPU_DHRYSTONE)
				|| (flags & FLAG_CPU_WHETSTONE) || (flags & FLAG_CPU_LINPACK) || (flags & FLAG_MEM_GET)) {
			printf("cpus_used,cpu_mask,cpus_total,cpus_online,cpu_speed,cpu_dhrystone,cpu_whetstone,linpack_min,linpack_max,linpack_avg,memory_size,run_time\n");
			if (flags & FLAG_CA_GET)
				printf("%d,0x%" PRIx64",", results->cpus, results->cpumask);
			else
				printf("-,-,");

			if (flags & FLAG_CPU_GET)
				printf("%d,%d,", results->cpu_total, results->cpu_online);
			else
				printf("-,-,");

			if (flags & FLAG_CPU_SPEED)
				printf("%.*f,", prec, results->cpu_speed);
			else
				printf("-,");

			if (flags & FLAG_CPU_DHRYSTONE)
				printf("%.*f,", prec, results->cpu_dhrystone);
			else
				printf("-,");

			if (flags & FLAG_CPU_WHETSTONE)
				printf("%ld,", results->cpu_whetstone);
			else
				printf("-,");

			if (flags & FLAG_CPU_LINPACK)
				printf("%.*f,%.*f,%.*f,", prec, results->cpu_linpack_min, prec, results->cpu_linpack_max,
						prec, results->cpu_linpack_avg);
			else
				printf("-,-,-,");

			if (flags & FLAG_MEM_GET)
				printf("%.f,", results->memory_size);
			else
				printf("-,");

			printf("%.*f\n", prec, results->run_time);
		}

		if (flags & FLAG_DISK_STAT) {
			printf("operation,size,chunk_size,throughput,cpu_usage\n");

			for (i = 0; i < results->disk_res_size; i++) {
				if (strncmp(results->disk[i].operation, "File", 4) != 0)
					printf("%s,%lld,%ld,%.*f,%.*f\n", results->disk[i].operation, results->disk[i].size, results->disk[i].chunk_size,
							prec, results->disk[i].throughput, prec, results->disk[i].cpu_usage);
				else {
					printf("%s,%lld,-,%.*f,%.*f\n", results->disk[i].operation, results->disk[i].size,
							prec, results->disk[i].throughput, prec, results->disk[i].cpu_usage);
				}
			}
		}
		if (flags & FLAG_NETC_STAT) {
			printf("operation,size,chunk_size,throughput,cpu_usage\n");

			for (i = 0; i < results->net_res_size; i++)
					printf("%s,%lld,%ld,%.*f,%.*f\n", results->net[i].operation, results->net[i].size, results->net[i].chunk_size,
							prec, results->disk[i].throughput, prec, results->net[i].cpu_usage);
		}
	}
	else
	if (type == FORMAT_XML) {
		printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		printf("<resultset time=\"%.*f\">\n", prec, results->run_time);
		printf("\t<cpu>\n");
		if (flags & FLAG_CA_GET) {
			printf("\t\t<num_used>%d</num_used>\n", results->cpus);
			printf("\t\t<mask>0x%" PRIx64 "</mask>\n", results->cpumask);
		}
		else {
			printf("\t\t<num_used />\n");
			printf("\t\t<mask />\n");
		}
		if (flags & FLAG_CPU_GET) {
			printf("\t\t<total>%d</total>\n", results->cpu_total);
			printf("\t\t<online>%d</online>\n", results->cpu_online);
		}
		else {
			printf("\t\t<total />\n");
			printf("\t\t<online />\n");
		}

		if (flags & FLAG_CPU_SPEED)
			printf("\t\t<speed>%.*f</speed>\n", prec, results->cpu_speed);
		else
			printf("\t\t<speed />\n");

		if (flags & FLAG_CPU_DHRYSTONE)
			printf("\t\t<dhrystone>%.*f</dhrystone>\n", prec, results->cpu_dhrystone);
		else
			printf("\t\t<dhrystone />\n");

		if (flags & FLAG_CPU_WHETSTONE)
			printf("\t\t<whetstone>%ld</whetstone>\n", results->cpu_whetstone);
		else
			printf("\t\t<whetstone />\n");

		if (flags & FLAG_CPU_LINPACK) {
			printf("\t\t<linpack>\n");
			printf("\t\t\t<size>%d</size>\n", results->cpu_linpack_size);
			printf("\t\t\t<min unit=\"MFLOPS\">%.*f</min>\n", prec, results->cpu_linpack_min);
			printf("\t\t\t<max unit=\"MFLOPS\">%.*f</max>\n", prec, results->cpu_linpack_max);
			printf("\t\t\t<avg unit=\"MFLOPS\">%.*f</avg>\n", prec, results->cpu_linpack_avg);
			printf("\t\t</linpack>\n");
		}
		else
			printf("\t\t<linpack />\n");
		printf("\t</cpu>\n");

		if (flags & FLAG_MEM_GET)
			printf("\t<memory>%.f</memory>\n", results->memory_size);
		else
			printf("\t<memory />\n");

		if (flags & FLAG_DISK_STAT) {
			printf("\t<results type=\"disk\" can_drop_caches=\"%d\">\n", results->disk_drop_caches);

			for (i = 0; i < results->disk_res_size; i++) {
					printf("\t\t<result operation=\"%s\" size=\"%lld\" chunk_size=\"%ld\" throughput=\"%.*f\" cpu=\"%.*f\" />\n", results->disk[i].operation,
							results->disk[i].size, results->disk[i].chunk_size, prec, results->disk[i].throughput, prec, results->disk[i].cpu_usage);
			}

			printf("\t</results>\n");
		}
		if (flags & FLAG_NETC_STAT) {
			printf("\t<results type=\"net\">\n");

			for (i = 0; i < results->net_res_size; i++) {
					printf("\t\t<result operation=\"%s\" size=\"%lld\" chunk_size=\"%ld\" throughput=\"%.*f\" cpu=\"%.*f\" />\n", results->net[i].operation,
							results->net[i].size, results->net[i].chunk_size, prec, results->net[i].throughput, prec, results->net[i].cpu_usage);
			}

			printf("\t</results>\n");
		}

		printf("</resultset>\n");
	}
	else
		printf("Format %d not implemented\n", type);
}

int main(int argc, char *argv[])
{
	int flags;
	unsigned long long start = 0;

	start = nanotime();

	results = (tResults *)malloc( sizeof(tResults) );
	memset(results, 0, sizeof(tResults));

	flags = parse_args(argc, argv);
	if ((flags & FLAG_NETS_STAT) && (flags & FLAG_NETC_STAT)) {
		fprintf(stderr, "Error: Cannot use network client and network server together!\n");
		return 1;
	}

	if (flags & FLAG_CA_GET)
		get_affinity();
	if (flags & FLAG_CPU_GET) {
		uint32_t proc;
		proc = cpu_get_count();

		results->cpu_online = PROCESSOR_COUNT_ONLINE(proc);
		results->cpu_total  = PROCESSOR_COUNT_TOTAL(proc);

		DPRINTF("Processors: %d total / %d online\n", PROCESSOR_COUNT_TOTAL(proc),
				PROCESSOR_COUNT_ONLINE(proc));
	}
	if (flags & FLAG_CPU_SPEED) {
		results->cpu_speed = cpu_get_speed_mhz();

		DPRINTF("Measured CPU Speed is: %.*f Mhz\n", prec, cpu_get_speed_mhz());
	}
	if (flags & FLAG_CPU_DHRYSTONE) {
		unsigned long dhrystone, loops, btime;
		cpu_dhrystone_get(&dhrystone, &loops, &btime);

		results->cpu_dhrystone = (float)dhrystone / 1757;

		DPRINTF("Dhrystone: %.*f DMIPS(*), loops: %lu, benchmark time: %ld\n", prec, (float)dhrystone / 1757, loops, btime);
		DPRINTF("* Calculated as dhrystone store divided by 1757 (see: http://en.wikipedia.org/wiki/Dhrystone)\n");
	}
	if (flags & FLAG_CPU_WHETSTONE) {
		unsigned long loops, iter, btime, mips;

		cpu_whetstone_get(&loops, &iter, &btime, &mips);

		results->cpu_whetstone = mips;

		DPRINTF("Whetstone: %lu MIPS, loops: %lu, iteration count: %ld, benchmark time: %ld sec\n", mips, loops, iter, btime);
	}
	if (flags & FLAG_CPU_LINPACK) {
		unsigned long memory;
		float minMFLOPS, maxMFLOPS, avgMFLOPS;

		get_linpack_score(lpArrSize, &memory, &minMFLOPS, &maxMFLOPS, &avgMFLOPS);

		results->cpu_linpack_size = lpArrSize;
		results->cpu_linpack_mem = memory;
		results->cpu_linpack_min = minMFLOPS;
		results->cpu_linpack_max = maxMFLOPS;
		results->cpu_linpack_avg = avgMFLOPS;

		DPRINTF("Linpack array size: %dx%d, memory %ld KB, MFLOPS: min=%.*f, max=%.*f, average=%.*f\n", lpArrSize, lpArrSize,
				memory >> 10, prec, minMFLOPS, prec, maxMFLOPS, prec, avgMFLOPS);
		DPRINTF("* For more information about LINPACK benchmarking see http://en.wikipedia.org/wiki/Linpack\n");
	}
	if (flags & FLAG_MEM_GET) {
		results->memory_size = memory_size_get(MEMTYPE_B);

		DPRINTF("Physical memory size: %.*f MB\n", prec, memory_size_get(MEMTYPE_MB));
	}
	if (flags & FLAG_DISK_STAT)
		disk_io_process( dioBufSize, 131072 );
	if (flags & FLAG_NETS_STAT)
		net_io_process( 1 );
	if (flags & FLAG_NETC_STAT)
		net_io_process( 0 );

	results->run_time = (nanotime() - start) / 1000000.0;
	DPRINTF("Run finished in %.*f seconds\n", prec, (nanotime() - start) / 1000000.0);

	printResults(flags, outType);

	/* Free all the memory for results */
	if (results->disk_res_size > 0)
		free(results->disk);
	if (results->net_res_size > 0)
		free(results->net);
	free(results);

	return 0;
}


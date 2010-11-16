/*
 * cpu.c: CPU-related functions
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

/*
 * The MEASURE_TIMEVAL constant defines the time value between measuring is done.  The value is being
 * presented in nanoseconds and the value 1000000 (1s) is the most precise value, any higher value just
 * slows it down with no real effect. Setting up lower values than 10000 (10 ms) may produce inaccurate
 * and very fluctuating results!
 * If you don't need the precision to 1/100th of the Mhz the optimal value is 50ms (i.e. value 50000).
 */

#define MEASURE_TIMEVAL 50000

#include "utils.h"

uint32_t cpu_get_count(void)
{
	long conf, online;

	conf = sysconf(_SC_NPROCESSORS_CONF);
	online = sysconf(_SC_NPROCESSORS_ONLN);

	return (conf | (online << 16));
}

unsigned long long rdtsc(void)
{
	unsigned a, d;

	__asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

	return ((unsigned long long)a) | (((unsigned long long)d) << 32);;
}

float cpu_get_speed_mhz(void)
{
	unsigned long long diff1, diff2;
	unsigned long long val11, val12, val21, val22;

	val11 = nanotime();
	val21 = rdtsc();
	while ((val12 = nanotime()) < val11 + MEASURE_TIMEVAL)
		val22 = rdtsc();

	diff1 = (unsigned long long)( (unsigned long long)val22 - (unsigned long long)val21 );
	diff2 = (unsigned long long)( (unsigned long long)val12 - (unsigned long long)val11 );
	return ((float)diff1 / diff2);
}

int cpu_affinity_get(pid_t pid, uint64_t *cmask)
{
	cpu_set_t mask;
	int i;
	int cpucount = sysconf(_SC_NPROCESSORS_ONLN);
	int nrcpus = cpucount;

	if (cmask == NULL)
		return nrcpus;

	if ( sched_getaffinity(pid, sizeof(mask), &mask) == -1 )
		return -errno;

	*cmask = 0;
	for ( i = 0; i < nrcpus; i++ )
		if ( CPU_ISSET(i, &mask) )
			*cmask |= (1 << i);

	return nrcpus;
}

int cpu_affinity_set(pid_t pid, uint64_t cmask)
{
	cpu_set_t mask;
	int i, rc;
	int nrcpus;

	rc = 0;
	nrcpus = sysconf(_SC_NPROCESSORS_ONLN);

	CPU_ZERO(&mask);
	for (i = 0; i < nrcpus; i++)
		if (cmask & (1 << i))
			CPU_SET(i, &mask);

	if ( sched_setaffinity(pid, sizeof(mask), &mask) == -1 )
		rc = -errno;

	return rc;
}


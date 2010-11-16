/*
 * common.c: Common functions for mBench
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

unsigned long long nanotime(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (((unsigned long long)tv.tv_sec * 1000000) + tv.tv_usec);
}

float calc_cpu_usage(float cpu, float tm)
{
	return (cpu / tm) * 100.0;
}

double cpu_time_get()
{
	struct rusage rusage;

	getrusage(RUSAGE_SELF, &rusage);
	return
		((double) rusage.ru_utime.tv_sec) +
		(((double) rusage.ru_utime.tv_usec) / 1000000.0) +
		((double) rusage.ru_stime.tv_sec) +
		(((double) rusage.ru_stime.tv_usec) / 1000000.0);
}

int io_get_size(unsigned long long size, int prec, char *sizestr, int maxlen)
{
	char res[64] = { 0 };
	int len, div = 1, unit = ' ';

	if (size > (1 << 30)) {
		div = (1 << 30);
		unit = 'G';
	}
	else
	if (size > (1 << 20)) {
		div = (1 << 20);
		unit = 'M';
	}
	else
	if (size > (1 << 10)) {
		div = (1 << 10);
		unit = 'k';
	}
	else
		div = 1;

	if (prec <= 0)
		snprintf(res, sizeof(res), "%lld %cB", (unsigned long long)size / div, unit);
	else
		snprintf(res, sizeof(res), "%.*f %cB", prec, (double)((unsigned long long)size / div), unit);

	len = strlen(res);
	if (len > maxlen)
		len = maxlen;

	memset(sizestr, 0, maxlen);
	strncpy(sizestr, res, len);

	return len;
}

int io_get_size_double(double size, int prec, char *sizestr, int maxlen)
{
	char res[64] = { 0 };
	int len, div = 1, unit = ' ';

	if (size > (double)(1 << 30)) {
		div = (1 << 30);
		unit = 'G';
	}
	else
	if (size > (double)(1 << 20)) {
		div = (1 << 20);
		unit = 'M';
	}
	else
	if (size > (double)(1 << 10)) {
		div = (1 << 10);
		unit = 'k';
	}
	else
		div = 1;

	snprintf(res, sizeof(res), "%.*f %cB", prec, (double)size / div, unit);

	len = strlen(res);
	if (len > maxlen)
		len = maxlen;

	memset(sizestr, 0, maxlen);
	strncpy(sizestr, res, len);

	return len;
}


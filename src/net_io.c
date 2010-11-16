/*
 * net_io.c: Network I/O benchmarking functions
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

char netio_addr[256] = "0.0.0.0";	/* reserve some space for IPv6 */
int	 netio_addr_port = 8340;

char **net_get_tokens(char *value, int *numTokens, char *tokenizer)
{
	char *s, *s1, *token;
	char **tokens;
	int num = 0;

	if (value == NULL)
		return NULL;

	tokens = (char **)malloc( sizeof(char *) );
	for (s = value; ; s = NULL) {
		token = strtok_r(s, tokenizer, &s1);
		if (token == NULL)
			break;

		tokens = (char **)realloc( tokens, (num + 1) * sizeof(char *) );
		tokens[num] = (char *)malloc( (strlen(token) + 1) * sizeof(char) );
		strncpy(tokens[num], token, strlen(token) + 1);
		num++;
	}

	if (numTokens != NULL)
		*numTokens = num;

	return tokens;
}

void net_free_tokens(char **tokens, int numTokens)
{
	int i;

	if (tokens == NULL)
		return;

	for (i = 0; i < numTokens; i++)
		free(tokens[i]);

	free(tokens);
	tokens = NULL;
}

void net_set_host(char *val, int port_only)
{
	char **tokens;
	int num = 0;

	tokens = net_get_tokens(val, &num, ":");
	if (num == 0)
		return;

	if (num >= 1)
		strncpy(netio_addr, tokens[0], sizeof(netio_addr));
	if ((num == 2) && (!port_only))
		netio_addr_port = atoi(tokens[1]);
	if ((num == 1) && (port_only))
		netio_addr_port = atoi(tokens[0]);

	net_free_tokens(tokens, num);
}

char *net_get_bound_addr()
{
	char *res, addr[6] = { 0 };

	snprintf(addr, sizeof(addr), ":%d", netio_addr_port);
	res = (char *)malloc( (strlen(addr) + 1) * sizeof(char) );
	snprintf(res, strlen(addr) + 1, "%s", addr);
	return res;
}

char *net_get_connect_addr()
{
	char *res, addr[128 + 6] = { 0 };

	snprintf(addr, sizeof(addr), "%s:%d", netio_addr, netio_addr_port);
	res = (char *)malloc( (strlen(addr) + 1) * sizeof(char) );
	snprintf(res, strlen(addr) + 1, "%s", addr);
	return res;
}

int net_listen_fd(int port, int type)
{
	struct sockaddr_in l;
	int sock, family;
	const int true=1;

	if (type != NET_IPV4)
		return -EINVAL;

	family = (type == NET_IPV6) ? AF_INET6 : AF_INET;
	sock = socket(family, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
		return -errno;

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &true, 4);

	l.sin_family = family;
	l.sin_port = htons(port);
	l.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock, (struct sockaddr*)&l, sizeof(struct sockaddr)) < 0)
		return -errno;

	if (listen(sock, 5) < 0)
		return -errno;

	return sock;
}

unsigned long long net_get_value(char *value)
{
	int last_char = 0, multiplier = 1;

	if ((value[strlen(value) - 1] == '\n') || (value[strlen(value) - 1] == '\r'))
		last_char = value[strlen(value) - 2];
	else
		last_char = value[strlen(value) - 1];

	switch (last_char) {
		case 'k':
				multiplier = (1 << 10);
				break;
		case 'M':
				multiplier = (1 << 20);
				break;
		case 'G':
				multiplier = (1 << 30);
				break;
		default:
				multiplier = 1;
	}

	return (strtol(value, NULL, 10) * multiplier);
}

void net_generate_and_write(int sock, long chunk_size, unsigned long long size)
{
	int i, ii;
	char *str;
	unsigned long long num_calls = 0;

	num_calls = (size / chunk_size);
	if (num_calls * chunk_size < size)
		num_calls++;

	str = (char *)malloc( chunk_size * sizeof(char) );
	for (i = 0; i < num_calls; i++) {
		for (ii = 0; ii < chunk_size; ii++)
			str[ii] = (random() + getpid()) % 256;

		write(sock, str, chunk_size);
	}
	free(str);
}

int net_accept(int sock, struct sockaddr *addr, socklen_t clen)
{
	char clienthost[NI_MAXHOST];
	char clientservice[NI_MAXSERV];
	char buffer[1024] = { 0 };
	int len = 0;

	getnameinfo(addr, clen,
		clienthost, sizeof(clienthost),
		clientservice, sizeof(clientservice),
		NI_NUMERICHOST);

	snprintf(buffer, sizeof(buffer), "Hi %s!\n", clienthost);
	write(sock, buffer, strlen(buffer));
	while (1) {
		len = read(sock, buffer, 1024);
		if (len > 0) {
			buffer[len - 1] = 0;

			if (strlen(buffer) < 3)
				continue;

			if (strncmp(buffer, "TERM", 4) == 0) {
				close(sock);
				return 1;
			}

			/* Format: WRITE RANDOM 100M 4k */
			if (strncmp(buffer, "WRITE RANDOM ", 13) == 0) {
				char *data = buffer + 12;
				if (data != NULL) {
					char **tokens;
					int num;

					tokens = net_get_tokens(data, &num, " ");
					if (num > 0) {
						int chunk_size = 4096; /* 4 kB by default */
						long size = net_get_value(tokens[0]);
						if (num == 2)
							chunk_size = net_get_value(tokens[1]);
						net_generate_and_write(sock, chunk_size, size);
						fsync(sock);
					}
					net_free_tokens(tokens, num);
				}
			}
			else
			if (strncmp(buffer, "QUIT", 4) == 0) {
				close(sock);
				break;
			}
			else {
				char tmp[1088] = { 0 };
				snprintf(tmp, sizeof(tmp), "Unsupported command: %s\n", buffer);
				write(sock, tmp, strlen(tmp));
			}
		}
	}

	return 0;
}

int net_listen(int port, int type)
{
	struct sockaddr_in caddr;
	int sockfd, afsock;
	socklen_t clen;

	if ((type != NET_IPV4) && (type != NET_IPV6))
		return -EINVAL;

	if (port <= 0)
		port = netio_addr_port;

	sockfd = net_listen_fd(port, type);
	if (sockfd < 0)
		return (errno != 0) ? -errno : -EINVAL;

	clen = sizeof(caddr);
	while (1) {
		afsock = accept(sockfd, (struct sockaddr*)&caddr, &clen);
		if (afsock > 0) {
			/* Create a child for accepted connection */
			int pid = fork();
			if (pid == 0) {
				if (net_accept(afsock, (struct sockaddr *)&caddr, clen) == 1) {
					close(afsock);
					_exit(1);
				}
			}

			int status;
			waitpid(pid, &status, 0);
			if (WIFEXITED(status)) {
				if (WEXITSTATUS(status)) {
					close(afsock);
					return 1;
				}
			}
			close(afsock);
		}
	}
	close(sockfd);

	return 0;
}

int net_connect_fd(char *host, int port, sa_family_t family)
{
	struct addrinfo hints, *res, *res0;
	char ports[6] = { 0 };
	int err, s = -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	snprintf(ports, 6, "%d", port);
	err = getaddrinfo(host, ports, &hints, &res0);
	if (err)
		return -errno;

	for (res=res0 ; res; res = res->ai_next) {
		s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (s < 0) {
			s = -errno;
			continue;
		}

		if (connect(s, (struct sockaddr *)res->ai_addr, res->ai_addrlen) < 0) {
			s = -errno;
			continue;
		}

		break;
	}

	freeaddrinfo(res0);

	return s;
}

int net_connect(char *host, int port, int type)
{
	int sockfd;
	sa_family_t st;

	if ((type != NET_IPV4) && (type != NET_IPV6))
		return -EINVAL;

	if (host == NULL)
		host = strdup(netio_addr);
	if (port <= 0)
		port = netio_addr_port;

	switch (type) {
		case NET_IPV4:
					st = AF_INET;
					break;
		case NET_IPV6:
					st = AF_INET6;
					break;
		default:
					st = AF_UNSPEC;
					break;
	}

	sockfd = net_connect_fd(host, port, st);
	return sockfd;
}

int net_sock_have_data(int sock, int timeout)
{
		fd_set rfds;
		struct timeval tv;
		int retval;

		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);

		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		retval = select(sock + 1, &rfds, NULL, NULL, &tv);

		if (retval == -1)
			return -errno;
		else if (retval)
			return 1;
		else
			return 0;
}

int ixo_get_size(unsigned long long size, int prec, char *sizestr, int maxlen)
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
		snprintf(res, sizeof(res), "%.*f %cB", prec, (double)size / div, unit);

	len = strlen(res);
	if (len > maxlen)
		len = maxlen;

	memset(sizestr, 0, 16);
	strncpy(sizestr, res, len);

	return len;
}

int net_write_command(int sock, unsigned long long size, unsigned long chunksize, unsigned long bufsize, float *otm, float *fcpu)
{
	unsigned long long total;
	char *buf;
	char cmd[64] = { 0 };
	double cpu_start = 0.0, cpu = 0.0, tm = 0.0;
	unsigned long long start = 0;
	
	snprintf(cmd, sizeof(cmd), "WRITE RANDOM %lld %ld\n", size, chunksize);
	write(sock, cmd, strlen(cmd));

	buf = (char *)malloc( bufsize * sizeof(char) );

	cpu_start = cpu_time_get();
	start = nanotime();

	total = 0;
	while (net_sock_have_data(sock, 5) == 1)
		total += read(sock, buf, bufsize);

	tm = (nanotime() - start) / 1000000.0;
	cpu = calc_cpu_usage(cpu_time_get() - cpu_start, tm);

	if (otm != NULL)
		*otm = tm;
	if (fcpu != NULL)
		*fcpu = cpu;

	free(buf);

	return 0;
}

int net_server_terminate(int sock)
{
	int res;
	res = write(sock, "TERM\n", 5);

	close(sock);

	return (res == 5);
}


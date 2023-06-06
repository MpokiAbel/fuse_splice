// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
/* Copyright (c) 2022 Hengqi Chen */
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <bpf/libbpf.h>
#include <arpa/inet.h>
#include "xdp.skel.h"

#define LO_IFINDEX 1

static volatile sig_atomic_t exiting = 0;

static void sig_int(int signo)
{
	exiting = 1;
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
	return vfprintf(stderr, format, args);
}

int main(int argc, char **argv)
{
	// DECLARE_LIBBPF_OPTS(bpf_tc_hook, tc_hook, .ifindex = LO_IFINDEX,
	// 		    .attach_point = BPF_TC_INGRESS);
	DECLARE_LIBBPF_OPTS(bpf_xdp_attach_opts, xdp_opts);
	struct xdp_bpf *skel;
	int err;
	int prog_fd;

	libbpf_set_print(libbpf_print_fn);

	skel = xdp_bpf__open_and_load();
	if (!skel) {
		fprintf(stderr, "Failed to open BPF skeleton\n");
		return 1;
	}

	prog_fd = bpf_program__fd(skel->progs._xdp_ip_filter);
	err = bpf_xdp_attach(LO_IFINDEX, prog_fd, 0, &xdp_opts);
	if (err) {
		fprintf(stderr, "Failed to attach XDP: %d\n", err);
		return -err;
	}

	if (signal(SIGINT, sig_int) == SIG_ERR) {
		err = errno;
		fprintf(stderr, "Can't set signal handler: %s\n", strerror(errno));
		goto cleanup;
	}

	printf("Successfully started! Please run `sudo cat /sys/kernel/debug/tracing/trace_pipe` "
	       "to see output of the BPF program.\n");
	__u32 key = 0;
	char *ip_param = "127.0.0.1";

	struct sockaddr_in sa_param;
	inet_pton(AF_INET, ip_param, &(sa_param.sin_addr));
	__u32 ip = sa_param.sin_addr.s_addr;
	printf("the ip to filter is %s/%u\n", ip_param, ip);

	struct bpf_map *ip_map = skel->maps.ip_map;
	struct bpf_map *counter_map = skel->maps.counter_map;

	int result = bpf_map__update_elem(ip_map, &key, sizeof(__u32), &ip, sizeof(__u32), BPF_ANY);

	if (result != 0) {
		fprintf(stderr, "bpf_map_update_elem error %d %s \n", errno, strerror(errno));
		return 1;
	}
	// while (!exiting) {

	// 			sleep(2);
	// }

	int i, j;

	// get the number of cpus
	unsigned int nr_cpus = libbpf_num_possible_cpus();
	__u64 values[nr_cpus];

	// "infinite" loop
	for (i = 0; i < 1000; i++) {
		// get the values of the second map into values.
		assert(bpf_map__lookup_elem(counter_map, &key, sizeof(key), values, sizeof(values),
					    BPF_ANY) == 0);
		printf("%d\n", i);
		for (j = 0; j < nr_cpus; j++) {
			printf("cpu %d, value = %llu\n", j, values[j]);
		}
		printf("\n\n");
		sleep(2);
	}

cleanup:
	xdp_opts.sz = xdp_opts.old_prog_fd = 0;
	err = bpf_xdp_detach(LO_IFINDEX, 0, &xdp_opts);

	if (err) {
		fprintf(stderr, "Failed to detach xdp: %d\n", err);
		return -err;
	}

	return -err;
}

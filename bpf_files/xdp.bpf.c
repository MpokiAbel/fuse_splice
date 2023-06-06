#include <vmlinux.h>
#include <bpf/bpf_endian.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

#define ETH_P_IP 0x0800

char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, __u32);
} ip_map SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
	__type(key, __u32);
	__type(value, __u64);
	__uint(max_entries, 1);
} counter_map SEC(".maps");

SEC("xdp")
int _xdp_ip_filter(struct xdp_md *ctx)
{
	// key of the maps
	u32 key = 0;
	// the ip to filter
	u32 *ip;

	bpf_printk("starting xdp ip filter\n");

	// get the ip to filter from the ip_filtered map
	ip = bpf_map_lookup_elem(&ip_map, &key);
	if (!ip) {
		return XDP_PASS;
	}
	bpf_printk("the ip address to filter is %u\n", ip);

	void *data_end = (void *)(long)ctx->data_end;
	void *data = (void *)(long)ctx->data;
	struct ethhdr *eth = data;

	// check packet size
	if ((void*)(eth + 1) > data_end) {
		return XDP_PASS;
	}

	// check if the packet is an IP packet
	if (bpf_ntohs(eth->h_proto) != ETH_P_IP) {
		return XDP_PASS;
	}

	// get the source address of the packet
	struct iphdr *iph = data + sizeof(struct ethhdr);
	if ((void *)(iph + 1) > data_end) {
		return XDP_PASS;
	}
	u32 ip_src = iph->saddr;
	bpf_printk("source ip address is %u\n", ip_src);

	// drop the packet if the ip source address is equal to ip
	if (ip_src == *ip) {
		u64 *filtered_count;
		u64 *counter;
		counter = bpf_map_lookup_elem(&counter_map, &key);
		if (counter) {
			*counter += 1;
		}
		return XDP_DROP;
	}
	return XDP_PASS;
}
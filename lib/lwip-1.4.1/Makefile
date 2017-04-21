.PHONY: all kernel clean

TARGET = liblwip.a

DIR = obj obj/arch obj/core obj/core/ipv4 obj/core/snmp obj/netif obj/netif/ppp

OBJS =	obj/arch/sys_arch.o obj/arch/driver.o	\
	obj/core/tcp.o obj/core/sys.o obj/core/def.o obj/core/init.o obj/core/tcp_out.o obj/core/mem.o	\
	obj/core/dns.o obj/core/stats.o obj/core/dhcp.o obj/core/netif.o obj/core/memp.o	\
	obj/core/timers.o obj/core/raw.o obj/core/tcp_in.o obj/core/pbuf.o obj/core/udp.o	\
	obj/core/ipv4/inet.o obj/core/ipv4/autoip.o obj/core/ipv4/ip_frag.o obj/core/ipv4/icmp.o	\
	obj/core/ipv4/ip.o obj/core/ipv4/igmp.o obj/core/ipv4/inet_chksum.o obj/core/ipv4/ip_addr.o	\
	obj/core/snmp/mib2.o obj/core/snmp/msg_in.o obj/core/snmp/msg_out.o obj/core/snmp/mib_structs.o	\
	obj/core/snmp/asn1_enc.o obj/core/snmp/asn1_dec.o	\
	obj/netif/etharp.o obj/netif/slipif.o obj/netif/ethernetif.o	\
	obj/netif/ppp/auth.o obj/netif/ppp/md5.o obj/netif/ppp/chap.o obj/netif/ppp/randm.o	\
	obj/netif/ppp/lcp.o obj/netif/ppp/ppp_oe.o obj/netif/ppp/vj.o obj/netif/ppp/ppp.o	\
	obj/netif/ppp/fsm.o obj/netif/ppp/pap.o obj/netif/ppp/chpms.o obj/netif/ppp/magic.o	\
	obj/netif/ppp/ipcp.o

CFLAGS = -I ../core/include -I ../vnic/include -I src/include -I src/include/ipv4 -O2 -Wall -Werror -m64 -nostdlib -ffreestanding -std=gnu99 -mcmodel=kernel

all: $(DIR) $(TARGET)

$(DIR):
	mkdir -p $(DIR)

obj/arch/driver.o: src/arch/driver.c
	gcc $(CFLAGS) -I ../core/include -c -o $@ $<
	
obj/%.o: src/%.c
	gcc $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	ar rcs $@ $^

clean: 
	rm -rf obj
	rm -f $(TARGET)

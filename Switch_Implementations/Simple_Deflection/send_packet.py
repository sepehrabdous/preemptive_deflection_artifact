from scapy.all import *
# from zmq import IPV6

BEE_PORT = 9999

class BEE(Packet):
    name = "bee"
    fields_desc = [
        BitField("port_idx_in_reg", size=32, default=0),
        BitField("queue_occ_info", size=8, default=0)
    ]

class DIBS(Packet):
    name = "dibs"
    fields_desc = [
        BitField("port_idx_in_reg", size=32, default=0),
        BitField("queue_occ_info", size=8, default=0)
    ]


# p_bee = Ether()/IP()/UDP(dport=BEE_PORT)/BEE(queue_occ_info=3)/"Sepehr"
# hexdump(p_bee)
# p_bee.show()
# print('--------------------------------')
# sendp(p_bee, iface="veth1")

# p_bee = Ether()/IP(src='127.2.2.2', dst='127.2.2.2')/UDP(sport=13, dport=10)/DIBS(queue_occ_info=3)/"Sepehr"
# hexdump(p_bee)
# p_bee.show()
# print('--------------------------------')
# sendp(p_bee, iface="veth1")


# p_aifo = Ether()/IP()/UDP(dport=AIFO_PORT)/AIFO(rank=i)/"Sepehr"
# hexdump(p_aifo)
# p_aifo.show()
# print('--------------------------------')
# sendp(p_aifo, iface="veth5")

# for i in range(PORT_NUM):
#     p_bee = Ether()/IP()/UDP(dport=BEE_PORT)/BEE(ucast_egress_port=i, qid=0, queue_length=0)/"Sepehr"
#     hexdump(p_bee)
#     p_bee.show()
#     print('--------------------------------')
#     sendp(p_bee, iface="veth1")

# for i in range(5, 0, -1):
#     data = "Sepehr"+str(i)
#     p_aifo = Ether()/IP()/UDP(dport=AIFO_PORT)/AIFO(rank=i)/data
#     hexdump(p_aifo)
#     p_aifo.show()
#     print('--------------------------------')
#     sendp(p_aifo, iface="enp5s0f1")

# data = "Sepehr"
# p_aifo = Ether()/IP()/UDP(dport=AIFO_PORT)/data
# p_aifo = Ether()/IP(dst='127.2.2.2')/data
# # p_aifo = Ether()/IP(dst='127.2.2.2')/UDP(dport=AIFO_PORT)/AIFO(rank=0)/data
# # p_aifo = Ether(src="3c:fd:fe:ab:df:91", dst="3c:fd:fe:ab:de:f1")/IP(dst='127.2.2.2')/UDP(dport=1234)/AIFO(rank=0)/data
# hexdump(p_aifo)
# p_aifo.show()
# print('--------------------------------')
# sendp(p_aifo, iface="veth0")

# p_aifo = Ether(dst="33:33:00:00:00:02", src="E8:EB:D3:FD:DB:88", type=0x86DD)/Raw('\x60\x03\x88\x7A\x00\x10\x3A\xFF\xFE\x80\x00\x00\x00\x00\x00\x00\xEA\xEB\xD3\xFF\xFE\xFD\xDB\x88\xFF\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x85\x00\x4A\x49\x00\x00\x00\x00\x01\x01\xE8\xEB\xD3\xFD\xDB\x88')
# p_aifo = Ether(dst="33:33:00:00:00:02", src="E8:EB:D3:FD:DB:88")/IPv6(src="fe80::eaeb:d3ff:fefd:db88", dst="ff02::2", plen=16, nh=58, hlim=255, fl=0x3887a)/Raw(b'\x85\x00\x4A\x49\x00\x00\x00\x00\x01\x01\xE8\xEb\xD3\xFD\xDB\x88')
# hexdump(p_aifo)
# p_aifo.show()
# print('--------------------------------')
# sendp(p_aifo, iface="veth0")

PORT_NUM = 8
for i in range(PORT_NUM):
    p_dibs = Ether()/IP()/UDP(dport=BEE_PORT)/DIBS(port_idx_in_reg=i, queue_occ_info=0)/"Erfan"
    hexdump(p_dibs)
    p_dibs.show()
    print('--------------------------------')
    sendp(p_dibs, iface="enp5s0f1")
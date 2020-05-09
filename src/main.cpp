/* 
Process must have CAP_NET_RAW

Simple program to test AF-Packet

Linux kernel: 5.3.0-42-generic
*/

#include "AF_Packet.h"

int main (int argc, char *argv[]) { 
    // 1. A socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)) is created.
    socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if ( socket_fd < 0 ) {
		Error(errno ? strerror(errno) : "unable to create socket");
		return;
	}
}


// 2. The socket is bound to the eth0 interface.


// 3. Ring buffer version is set to TPACKET_V2 via the PACKET_VERSION socket option.


// 4. A ring buffer is created via the PACKET_RX_RING socket option.


// 5. The ring buffer is mmapped in the userspace
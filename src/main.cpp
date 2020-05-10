/* 
 * Copyright 2020 Berry den Hartog. All rights reserved.
 *
 * Process must have CAP_NET_RAW
 * 
 * AF-Packet test, based on google stenotype
 *
 * this test is a mechanism for quickly reading raw packets. 
 *
 * NIC -> RAM
 * this test uses MMAP'd AF_PACKET with 1MB blocks and a high timeout to offload
 * analyzing packets.  The kernel packs all the packets it can into 1MB, then 
 * lets the userspace process know there's a block available in the MMAP'd ring 
 * buffer.  Nicely, it guarantees no overruns (packets crossing the 1MB boundary) 
 * and good alignment to memory pages.
 *
 * tested on Linux kernel: 5.3.0-42-generic
 */

#include <errno.h>            // errno
#include <fcntl.h>            // O_*
#include <grp.h>              // getgrnam()
#include <linux/if_packet.h>  // AF_PACKET, sockaddr_ll
#include <poll.h>             // POLLIN
#include <pthread.h>          // pthread_sigmask()
#include <pwd.h>              // getpwnam()
#include <sched.h>            // sched_setaffinity()
#include <signal.h>           // sigaction(), SIGINT, SIGTERM
#include <string.h>           // strerror()
#include <sys/prctl.h>        // prctl(), PR_SET_*
#include <sys/resource.h>     // setpriority(), PRIO_PROCESS
#include <sys/socket.h>       // socket()
#include <sys/stat.h>         // umask()
#include <sys/syscall.h>      // syscall(), SYS_gettid
#include <unistd.h>           // setuid(), setgid(), getpagesize()
#include <assert.h>           // assert()
#include <map>
#include <vector>

#include <string>
#include <sstream>
#include <thread>
#include <iostream>

#include <argp.h>             // argp_parse()

#include "util.h"
#include "packets.h"

namespace afpackettest {

// define all options
std::string flag_iface = "wlp61s0";
int32_t flag_threads = 1;
uint16_t flag_fanout_type = PACKET_FANOUT_LB | PACKET_FANOUT_FLAG_ROLLOVER;
uint16_t flag_fanout_id = 0;
uint64_t flag_blocksize_kb = 1024;
int32_t flag_blocks = 2048;
int64_t flag_blockage_sec = 10;
bool flag_promisc = true;

Error SetAffinity(int cpu) {
  cpu_set_t cpus;
  CPU_ZERO(&cpus);
  CPU_SET(cpu, &cpus);
  return Errno(sched_setaffinity(0, sizeof(cpus), &cpus));
}

void RunThread(int thread, Packets* v3) {
  if (flag_threads > 1) {
    LOG_IF_ERROR(SetAffinity(thread), "set affinity");
  }

  LOG(INFO) << "Thread " << thread << " starting to process packets";

  Packet p;
  int64_t blocks = 0;
  int64_t block_offset = 0;
  for (int64_t remaining = -1; remaining != 0;) {
    // Read in a new block from AF_PACKET.
    Block b;
    CHECK_SUCCESS(v3->NextBlock(&b, 1000));
    if (b.Empty()) {
      continue;
    }

    // Index all packets if necessary.
    for (; remaining != 0 && b.Next(&p); remaining--) {
      //index->Process(p, block_offset * flag_blocksize_kb * 1024);
    }
    blocks++;
    block_offset++;
  }

  LOG(INFO) << "Finished thread " << thread << " successfully";
}

int Main(int argc, char** argv) {
  //std::cout << "Starting afpacket test" << std::endl;
  VLOG(1) << "Starting afpacket test";
 

  VLOG(1) << "afpacket test running with these arguments:";
  for (int i = 0; i < argc; i++) {
    VLOG(1) << i << ":\t\"" << argv[i] << "\"";
  }
  LOG(INFO) << "Starting, page size is " << getpagesize();

  CHECK(flag_threads >= 1);
  CHECK(flag_blockage_sec > 0);
  CHECK(flag_blocks >= 16);
  CHECK(flag_blocksize_kb >= 10);
  CHECK(flag_blocksize_kb * 1024 >= (uint64_t)(getpagesize()));
  CHECK(flag_blocksize_kb * 1024 % (uint64_t)(getpagesize()) == 0);

  std::vector<Packets*> sockets;
  for (int i = 0; i < flag_threads; i++) {
    LOG(INFO) << "Setting up AF_PACKET sockets for packet reading";
    int socktype = SOCK_RAW;
    struct tpacket_req3 options;
    memset(&options, 0, sizeof(options));
    options.tp_block_size = flag_blocksize_kb * 1024;
    options.tp_block_nr = flag_blocks;
    options.tp_frame_size = flag_blocksize_kb * 1024;  // doesn't matter
    options.tp_frame_nr = 0;                           // computed for us.
    options.tp_retire_blk_tov = flag_blockage_sec * 1000 - 1;

    PacketsV3::Builder builder;
    CHECK_SUCCESS(builder.SetUp(socktype, options));
    CHECK_SUCCESS(builder.SetPromisc(flag_promisc));
    int fanout_id = getpid();
    if (flag_fanout_id > 0) {
      fanout_id = flag_fanout_id;
    }
    if (flag_fanout_id > 0 || flag_threads > 1) {
      CHECK_SUCCESS(builder.SetFanout(flag_fanout_type, fanout_id));
    }

    Packets* v3;
    CHECK_SUCCESS(builder.Bind(flag_iface, &v3));
    sockets.push_back(v3);

  }

  VLOG(1) << "Starting writing threads";
  std::vector<std::thread*> threads;
  for (int i = 0; i < flag_threads; i++) {
    VLOG(1) << "Starting thread " << i;
    threads.push_back(new std::thread(&RunThread, i, sockets[i]));
  }

  for (auto thread : threads) {
    VLOG(1) << "===============Waiting for thread==============";
    CHECK(thread->joinable());
    thread->join();
    VLOG(1) << "Thread finished";
    delete thread;
  }

  LOG(INFO) << "Process exiting successfully";

  return 0;

}

} // end namespace


int main(int argc, char** argv) { return afpackettest::Main(argc, argv); }
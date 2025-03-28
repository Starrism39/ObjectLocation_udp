#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <ctime>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "pkg/modules/pkgProcess.h"

struct PacketHeader {
    uint32_t magic = 0xAA55CC33;
    uint16_t total_frags;
    uint16_t frag_num;
    uint32_t data_size;
};

struct ReassemblyBuffer {
    std::map<uint16_t, std::vector<uint8_t>> fragments;  // 分片存储
    uint32_t expected_data_size;                                  // 预期总数据大小
    uint16_t expected_total_frags;                      // 预期总包数
    time_t last_active;                                   // 最后活动时间
};

std::string get_src_key(const sockaddr_in& addr);

class FragmentReassembler {
    private:
        std::map<std::string, ReassemblyBuffer> buffers_;  // 源地址 -> 重组缓冲区
        constexpr static int REASSEMBLE_TIMEOUT = 5;       // 重组超时(秒)
    
    public:
        // 处理收到的分片数据包
        void process_packet(const std::string& src_key, 
                           const PacketHeader& header,
                           const uint8_t* payload, 
                           size_t payload_len);
    
        // 清理超时的缓冲区
        void cleanup_expired();
    
    private:
        // 组装完整数据包并处理
        void assemble_and_process(const std::string& src_key, 
                                 ReassemblyBuffer& buf);
    
        // 打印包信息
        void print_package_info(const OutPackage& pkg, const std::string& src);
};

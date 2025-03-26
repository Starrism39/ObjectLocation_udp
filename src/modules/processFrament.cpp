#include "modules/processFrament.h"

std::string get_src_key(const sockaddr_in& addr) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
}

void FragmentReassembler::process_packet(const std::string& src_key, 
                                         const PacketHeader& header,
                                         const uint8_t* payload, 
                                         size_t payload_len) 
{
    // 初始化或更新缓冲区
    auto& buf = buffers_[src_key];
    buf.total_size = header.data_size;
    buf.last_active = time(nullptr);

    // 存储分片（自动去重）
    buf.fragments[header.frag_num].assign(payload, payload + payload_len);

    // 检查是否完成重组
    if(buf.fragments.size() == header.total_frags) {
        assemble_and_process(src_key, buf);
        buffers_.erase(src_key);
    }
}

void FragmentReassembler::cleanup_expired() {
    auto now = time(nullptr);
    for(auto it = buffers_.begin(); it != buffers_.end();) {
        if(now - it->second.last_active > REASSEMBLE_TIMEOUT) {
            std::cerr << "清理超时缓冲区: " << it->first << std::endl;
            it = buffers_.erase(it);
        } else {
            ++it;
        }
    }
}

void FragmentReassembler::assemble_and_process(const std::string& src_key, 
                                               ReassemblyBuffer& buf) 
{
    // 按顺序组装数据
    std::vector<uint8_t> full_data;
    full_data.reserve(buf.total_size);

    for(uint16_t i=0; i<buf.fragments.size(); ++i) {
        auto& frag = buf.fragments[i];
        full_data.insert(full_data.end(), frag.begin(), frag.end());
    }

    // 验证数据大小
    if(full_data.size() != buf.total_size) {
        std::cerr << "数据大小不匹配! 期望:" << buf.total_size
                  << " 实际:" << full_data.size() << std::endl;
        return;
    }

    // 反序列化处理
    OutPackage pkg;
    if(deserializeOutPackage(full_data.data(), full_data.size(), pkg)) {
        print_package_info(pkg, src_key);
    } else {
        std::cerr << "反序列化失败: " << src_key << std::endl;
    }
}

void FragmentReassembler::print_package_info(const OutPackage& pkg, const std::string& src) {
    std::cout << "\n=== 收到完整数据包 [" << src << "] ===" << std::endl;
    std::cout << "时间戳: " << pkg.time << std::endl;
    std::cout << "时间片: " << pkg.time_slice << "秒" << std::endl;

    std::cout << "\n无人机姿态:" << std::endl;
    for(const auto& [id, pose] : pkg.uav_pose) {
        std::cout << "  UAV" << id << ": ";
        for(double val : pose) std::cout << val << " ";
        std::cout << std::endl;
    }

    std::cout << "\n检测目标 (" << pkg.objs.size() << "个):" << std::endl;
    for(const auto& obj : pkg.objs) {
        std::cout << "  目标ID:" << obj.global_id << " 位置("
                  << obj.location[0] << ", " << obj.location[1] 
                  << ", " << obj.location[2] << ")" << std::endl;
        
        std::cout << "  关联图像来源: ";
        for(const auto& [uav_id, img] : obj.uav_img) {
            std::cout << "UAV" << uav_id << "(" << img.cols << "x"
                      << img.rows << ") ";
        }
        std::cout << std::endl;
    }
}

#include "pkg/modules/processPkgFrament.h"

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
    // 尝试获取或创建缓冲区
    auto it = buffers_.find(src_key);
    if (it == buffers_.end()) {

        if (header.frag_num != 0) {
            // 非首分片到达但无缓冲区，忽略（或记录警告）
            std::cerr << "非首分片到达但无缓冲区: " << src_key << std::endl;
            return;
        }

        // 新src_key，创建缓冲区并初始化元数据
        ReassemblyBuffer new_buf;
        new_buf.expected_total_frags = header.total_frags;
        new_buf.expected_data_size = header.data_size;
        new_buf.last_active = time(nullptr);
        new_buf.fragments[header.frag_num].assign(payload, payload + payload_len);
        buffers_[src_key] = new_buf;
        return;
    }

    // 已有缓冲区，引用现有数据
    auto& buf = it->second;

    // 验证元数据一致性（非首分片时）
    if (header.total_frags != buf.expected_total_frags || 
        header.data_size != buf.expected_data_size) {
        std::cerr << "元数据不匹配，清理缓冲区: " << src_key << std::endl;
        buffers_.erase(it);  // 关键点：验证失败时清理
        return;
    }

    // 验证分片号合法性
    if (header.frag_num >= buf.expected_total_frags) {
        std::cerr << "非法分片号，清理缓冲区: " << header.frag_num 
                  << "/" << buf.expected_total_frags << std::endl;
        buffers_.erase(it);  // 关键点：非法分片号时清理
        return;
    }

    // 更新活动时间
    buf.last_active = time(nullptr);

    // 存储分片（自动去重）
    buf.fragments[header.frag_num].assign(payload, payload + payload_len);

    // 检查是否全部接收
    if(header.frag_num == buf.expected_total_frags - 1){
        bool all_received = true;
        for (uint16_t i = 0; i < buf.expected_total_frags; ++i) {
            if (!buf.fragments.count(i)) {
                all_received = false;
                buffers_.erase(it);
                break;
            }
        }
    
        // 完成重组并清理
        if (all_received) {
            assemble_and_process(src_key, buf);
            buffers_.erase(it);
        }
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
    full_data.reserve(buf.expected_data_size);

    for(uint16_t i=0; i<buf.fragments.size(); ++i) {
        auto& frag = buf.fragments[i];
        full_data.insert(full_data.end(), frag.begin(), frag.end());
    }

    // 验证数据大小
    if(full_data.size() != buf.expected_data_size) {
        std::cerr << "数据大小不匹配! 期望:" << buf.expected_data_size
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
        std::cout << "  UAV" << static_cast<int>(id) << ": ";
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
            std::cout << "UAV" << static_cast<int>(uav_id) << "(" << img.cols << "x"
                      << img.rows << ") ";
        }
        std::cout << std::endl;
    }
}

#include <iostream>
#include <atomic>
#include <vector>
#include <map>
#include <arpa/inet.h>
#include <thread>

#include "utils/udp_operation.h"
#include "utils/threadsafe_queue.h"
#include "pkg/modules/processPkgFrament.h"


struct PacketData {
    std::string src_key;
    PacketHeader header;
    std::vector<uint8_t> payload;
};

// 生产者函数 - 接收数据包并放入队列
void producer_thread_func(UDPOperation& receiver, 
                          ThreadSafeQueue<PacketData>& packet_queue,
                          std::atomic<bool>& running) {
    constexpr size_t MAX_PACKET_SIZE = 1500; // 标准MTU
    std::vector<char> buffer(MAX_PACKET_SIZE);
    
    while(running) {
        int received = receiver.recv_buffer(buffer.data(), buffer.size());

        if(received > 0) {
            // 解析包头
            if(static_cast<size_t>(received) < sizeof(PacketHeader)) {
                std::cerr << "收到无效小包(" << received << "字节)" << std::endl;
                continue;
            }

            PacketHeader header;
            memcpy(&header, buffer.data(), sizeof(header));

            // 验证魔术字
            if(header.magic != 0xAA55CC33) {
                std::cerr << "收到无效魔术字包头" << std::endl;
                continue;
            }

            // 获取源标识
            std::string src_key = get_src_key(*receiver.get_cliaddr());

            // 准备数据放入队列
            PacketData data;
            data.src_key = src_key;
            data.header = header;
            
            // 拷贝有效载荷
            const uint8_t* payload = 
                reinterpret_cast<const uint8_t*>(buffer.data()) + sizeof(header);
            size_t payload_len = received - sizeof(header);
            data.payload.assign(payload, payload + payload_len);
            
            // 放入队列
            packet_queue.push(std::move(data));
        }
    }
}

// 消费者函数 - 处理数据包
void consumer_thread_func(ThreadSafeQueue<PacketData>& packet_queue,
                          FragmentReassembler& reassembler,
                          std::atomic<bool>& running) {
    time_t last_clean = time(nullptr);
    
    while(running) {
        // 从队列中获取数据
        PacketData data;
        if(packet_queue.pop(data)) {
            // 处理分片
            reassembler.process_packet(
                data.src_key, 
                data.header, 
                data.payload.data(), 
                data.payload.size()
            );
            
            // 定期清理过期缓冲区
            if(time(nullptr) - last_clean > 1) {
                reassembler.cleanup_expired();
                last_clean = time(nullptr);
            }
        }
    }
}

int main() {
    UDPOperation receiver("127.0.0.1", 12345, "lo"); 
    if(!receiver.create_client()) {
        std::cerr << "创建接收端失败!" << std::endl;
        return 1;
    }

    // 创建线程安全队列
    ThreadSafeQueue<PacketData> packet_queue;
    FragmentReassembler reassembler;
    
    // 标志位用于控制线程退出
    std::atomic<bool> running(true);
    
    // 创建生产者线程
    std::thread producer(producer_thread_func, 
                        std::ref(receiver), 
                        std::ref(packet_queue), 
                        std::ref(running));
    
    // 创建消费者线程
    std::thread consumer(consumer_thread_func,
                        std::ref(packet_queue),
                        std::ref(reassembler),
                        std::ref(running));
    
    // 在主线程中等待用户输入退出
    std::cout << "按Enter键退出程序..." << std::endl;
    std::cin.get();
    
    // 通知线程退出
    running = false;
    
    // 等待线程结束
    producer.join();
    consumer.join();
    
    std::cout << "程序已退出" << std::endl;
    return 0;
}
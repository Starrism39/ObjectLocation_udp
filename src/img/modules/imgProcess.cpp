#include "img/modules/imgProcess.h"
#include "utils/protocol.h"
#include <cstring>
#include <arpa/inet.h>

bool serializeImgPackage(const imgPackage& pkg, std::vector<uint8_t>& buffer){
    using namespace ProtocolUtils;
    
    // 帧头
    buffer.insert(buffer.end(), {0xBE, 0x99, 0x90, 0x22});
    
    // 时间戳
    serialize<uint64_t>(buffer, static_cast<uint64_t>(pkg.time));
    
    // 无人机ID
    serialize<uint8_t>(buffer, pkg.uav_id);
    
    // 映射数
    serialize<uint8_t>(buffer, static_cast<uint8_t>(pkg.label_box.size()));
    
    // label_box数据
    for (const auto& [label, boxes] : pkg.label_box) {
        serialize<uint8_t>(buffer, label);
        serialize<uint8_t>(buffer, static_cast<uint8_t>(boxes.size()));
        
        for (const auto& box : boxes) {
            if (box.size() != 4) return false; // x,y,w,h必须4个元素
            for (const auto& coord : box) {
                serialize<int32_t>(buffer, coord);
            }
        }
    }
    
    // 图像数据
    serializeMat(buffer, pkg.img);
    
    // 帧尾
    buffer.insert(buffer.end(), {0xBA, 0xCB, 0xDC, 0xED});
    return true;

}


bool deserializeImgPackage(const uint8_t* data, size_t length, imgPackage& pkg){
    using namespace ProtocolUtils;
    const uint8_t* end = data + length;
    
    // 检查帧头
    if (length < 4 || memcmp(data, "\xBE\x99\x90\x22", 4) != 0) return false;
    data += 4;
    
    // 时间戳
    pkg.time = deserialize<uint64_t>(data);
    
    // 无人机ID
    pkg.uav_id = deserialize<uint8_t>(data);
    
    // 映射数
    const uint8_t map_count = deserialize<uint8_t>(data);
    pkg.label_box.clear();
    
    // 解析label_box
    for (uint8_t i = 0; i < map_count; ++i) {
        if (data + 2 > end) return false; // 至少需要label+count
        
        const uint8_t label = deserialize<uint8_t>(data);;
        const uint8_t box_count = deserialize<uint8_t>(data);;
        
        std::vector<std::vector<int>> boxes;
        for (uint8_t j = 0; j < box_count; ++j) {
            if (data + 16 > end) return false; // 每个框16字节
            std::vector<int> box(4);
            box[0] = deserialize<int32_t>(data); // x
            box[1] = deserialize<int32_t>(data); // y
            box[2] = deserialize<int32_t>(data); // w
            box[3] = deserialize<int32_t>(data); // h
            boxes.emplace_back(box);
        }
        pkg.label_box.emplace(label, boxes);
    }
    
    // 图像数据
    pkg.img = deserializeMat(data);
    
    // 检查帧尾
    if (end - data != 4 || memcmp(data, "\xBA\xCB\xDC\xED", 4) != 0) return false;
    return true;

}
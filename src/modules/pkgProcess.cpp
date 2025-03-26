#include "modules/pkgProcess.h"
#include <cstring>
#include <arpa/inet.h>

namespace ProtocolUtils {

// OpenCV矩阵序列化
void serializeMat(std::vector<uint8_t>& buffer, const cv::Mat& img) {
    serialize<uint32_t>(buffer, img.rows);
    serialize<uint32_t>(buffer, img.cols);
    serialize<int32_t>(buffer, img.type());
    
    if(img.isContinuous()) {
        buffer.insert(buffer.end(), img.data, img.data + img.total() * img.elemSize());
    } else {
        cv::Mat continuousImg = img.clone();
        buffer.insert(buffer.end(), continuousImg.data, 
                     continuousImg.data + continuousImg.total() * continuousImg.elemSize());
    }
}

// OpenCV矩阵反序列化
cv::Mat deserializeMat(const uint8_t*& data) {
    uint32_t rows = deserialize<uint32_t>(data);
    uint32_t cols = deserialize<uint32_t>(data);
    int type = deserialize<int32_t>(data);
    std::cout << "rows: " << rows << " cols: " << cols << " type: " << type << std::endl;
    
    cv::Mat img(rows, cols, type);
    size_t dataSize = rows * cols * img.elemSize();
    memcpy(img.data, data, dataSize);
    data += dataSize;
    return img;
}

} // namespace ProtocolUtils

// 协议序列化主函数
bool serializeOutPackage(const OutPackage& pkg, std::vector<uint8_t>& buffer) {
    using namespace ProtocolUtils;
    
    buffer.insert(buffer.end(), {0xEB, 0x22, 0x90, 0x99});
    
    uint64_t timestamp = pkg.time;
    serialize(buffer, timestamp);
    
    serialize<uint16_t>(buffer, static_cast<uint16_t>(pkg.time_slice));
    
    uint8_t poseCount = pkg.uav_pose.size();
    serialize<uint8_t>(buffer, poseCount);
    
    for (const auto& [uavId, pose] : pkg.uav_pose) {
        serialize<uint8_t>(buffer, uavId);
        for (const auto& val : pose) {
            serialize<double>(buffer, val);
        }
    }
    
    uint16_t objCount = pkg.objs.size();
    serialize<uint16_t>(buffer, objCount);
    
    for (const auto& obj : pkg.objs) {
        serialize<uint32_t>(buffer, obj.global_id);
        for (const auto& coord : obj.location) {
            serialize<double>(buffer, coord);
        }
        
        uint8_t imgCount = obj.uav_img.size();
        serialize<uint8_t>(buffer, imgCount);
        
        for (const auto& [uavId, img] : obj.uav_img) {
            serialize<uint8_t>(buffer, uavId);
            serializeMat(buffer, img);
        }
    }
    
    buffer.insert(buffer.end(), {0xED, 0xDC, 0xCB, 0xBA});
    return true;
}

// 协议反序列化主函数
bool deserializeOutPackage(const uint8_t* data, size_t length, OutPackage& pkg) {
    using namespace ProtocolUtils;
    const uint8_t* end = data + length;
    std::cout << "length:" << length << std::endl;
    
    if (length < 4 || memcmp(data, "\xEB\x22\x90\x99", 4) != 0) return false;
    data += 4;
    
    pkg.time = deserialize<uint64_t>(data);
    
    pkg.time_slice = deserialize<uint16_t>(data);
    
    uint8_t poseCount = deserialize<uint8_t>(data);
    
    for (int i = 0; i < poseCount; ++i) {
        int uavId = deserialize<uint8_t>(data);
        std::vector<double> pose(6); // yaw,pitch,roll,x,y,z
        for (auto& val : pose) {
            val = deserialize<double>(data);
        }
        pkg.uav_pose[uavId] = pose;
    }
    
    uint16_t objCount = deserialize<uint16_t>(data);
    pkg.objs.resize(objCount);
    
    for (auto& obj : pkg.objs) {
        obj.global_id = deserialize<uint32_t>(data);
        
        obj.location.resize(3);
        for (auto& coord : obj.location) {
            coord = deserialize<double>(data);
        }
        
        uint8_t imgCount = deserialize<uint8_t>(data);
        
        for (int i = 0; i < imgCount; ++i) {
            int uavId = deserialize<uint8_t>(data);
            cv::Mat img = deserializeMat(data);
            obj.uav_img.emplace(uavId, std::move(img));
        }
    }
    
    if (end - data != 4 || memcmp(data, "\xED\xDC\xCB\xBA", 4) != 0) return false;
    return true;
}

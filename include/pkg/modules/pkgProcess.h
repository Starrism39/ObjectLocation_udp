#include <iostream>
#include <vector>
#include <map>
#include <ctime>
#include <opencv2/opencv.hpp>

// 结构体定义
struct Object{
    int global_id;
    std::vector<double> location;
    std::map<uint8_t, cv::Mat> uav_img;
};

struct OutPackage{
    time_t time;
    uint16_t time_slice;
    std::map<uint8_t, std::vector<double>> uav_pose;
    std::vector<Object> objs;
};



// 协议序列化/反序列化函数声明
bool serializeOutPackage(const OutPackage& pkg, std::vector<uint8_t>& buffer);
bool deserializeOutPackage(const uint8_t* data, size_t length, OutPackage& pkg);




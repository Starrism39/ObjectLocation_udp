#include <iostream>
#include <vector>
#include <map>
#include <ctime>
#include <opencv2/opencv.hpp>

struct imgPackage{
    time_t time;
    uint8_t uav_id;
    std::map<uint8_t, std::vector<std::vector<int>>> label_box;  // std::vector<int>是目标框x,y,w,h，std::vector<std::vector<int>>代表同属于一个类别的所有目标框
    cv::Mat img;
};

// 协议序列化/反序列化函数声明
bool serializeImgPackage(const imgPackage& pkg, std::vector<uint8_t>& buffer);
bool deserializeImgPackage(const uint8_t* data, size_t length, imgPackage& pkg);


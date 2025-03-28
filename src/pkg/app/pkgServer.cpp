#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <thread>

#include "pkg/modules/pkgProcess.h"
#include "utils/sendFrament.hpp"


cv::Mat generateTestImage() {
    cv::Mat img(100, 100, CV_8UC3, cv::Scalar(0, 0, 255));
    cv::circle(img, cv::Point(50, 50), 30, cv::Scalar(0, 255, 0), -1);
    return img;
}

// 创建测试数据包
OutPackage createTestPackage() {
    OutPackage pkg;
    
    // 时间信息
    pkg.time = time(nullptr);
    pkg.time_slice = 1;
    
    // 无人机姿态信息
    pkg.uav_pose[1] = {0.1, 0.2, 0.3, 10.0, 20.0, 30.0};
    pkg.uav_pose[2] = {0.4, 0.5, 0.6, 40.0, 50.0, 60.0};
    
    // 创建测试目标
    Object obj1;
    obj1.global_id = 1001;
    obj1.location = {1.0, 2.0, 3.0};
    obj1.uav_img[1] = generateTestImage();
    obj1.uav_img[2] = generateTestImage();
    
    Object obj2;
    obj2.global_id = 1002;
    obj2.location = {4.0, 5.0, 6.0};
    obj2.uav_img[1] = generateTestImage();
    
    pkg.objs = {obj1, obj2};
    
    return pkg;
}


int main() {
    UDPOperation server("127.0.0.1", 12345, "lo");
    server.create_server();

    // 生成测试数据
    OutPackage testPkg = createTestPackage();
    
    // 序列化数据
    std::vector<uint8_t> buffer;
    if (!serializeOutPackage(testPkg, buffer)) {
        std::cerr << "Serialization failed!" << std::endl;
        return 1;
    }

    // 发送数据
    while(true){
        sendFragmented(server, buffer, 0xAA55CC33);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 测试丢包率和实际使用时注释掉
    }
    
    
    return 0;
}
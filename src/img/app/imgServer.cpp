#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <thread>

#include "img/modules/imgProcess.h"
#include "utils/sendFrament.h"


cv::Mat generateTestImage() {
    cv::Mat img(100, 100, CV_8UC3, cv::Scalar(0, 0, 255));
    cv::circle(img, cv::Point(50, 50), 30, cv::Scalar(0, 255, 0), -1);
    return img;
}

// 创建测试数据包
imgPackage createTestPackage() {
    imgPackage pkg;
    
    // 时间信息
    pkg.time = time(nullptr);
    pkg.uav_id = 1;
    

    pkg.label_box[0].push_back({1,2,3,4});
    pkg.label_box[0].push_back({2,3,4,5});
    pkg.label_box[0].push_back({3,4,5,6});
    pkg.label_box[1].push_back({4,3,2,1});
    pkg.label_box[1].push_back({5,4,3,2});
    
    pkg.img = generateTestImage();
    
    return pkg;
}


int main() {
    UDPOperation server("127.0.0.1", 12345, "lo");
    server.create_server();

    // 生成测试数据
    imgPackage testPkg = createTestPackage();  // 这个地方传入需要输入的包
    
    // 序列化数据
    std::vector<uint8_t> buffer;
    if (!serializeImgPackage(testPkg, buffer)) {
        std::cerr << "Serialization failed!" << std::endl;
        return 1;
    }

    // 发送数据
    while(true){
        sendFragmented(server, buffer, 0x33CC55AA);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 测试丢包率时注释掉
    }
    
    
    return 0;
}
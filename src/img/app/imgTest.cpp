#include "img/modules/imgProcess.h"
#include <iostream>
#include <iomanip>
#include <random>
#include <ctime>

// 随机生成工具类
class TestDataGenerator {
private:
    std::random_device rd;
    std::mt19937 gen;

public:
    TestDataGenerator() : gen(rd()) {}

    // 生成指定范围的整数
    int generateInt(int min, int max) {
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }

    // 生成指定范围的浮点数
    double generateDouble(double min, double max) {
        std::uniform_real_distribution<> dis(min, max);
        return dis(gen);
    }

    // 生成模拟图像
    cv::Mat generateMockImage(int width, int height) {
        cv::Mat img(height, width, CV_8UC3);
        cv::randu(img, cv::Scalar(0,0,0), cv::Scalar(255,255,255));
        return img;
    }
};

// 生成模拟协议数据包
std::vector<imgPackage> generateTestPackages(TestDataGenerator& gen, 
                                            int num_packages,
                                            time_t base_time) {
    std::vector<imgPackage> packages;

    for (int i = 0; i < num_packages; ++i) {
        imgPackage pkg;
        
        // 时间戳（毫秒级）
        pkg.time = base_time * 1000 + i * 100;
        
        // 无人机ID (0-255)
        pkg.uav_id = static_cast<uint8_t>(gen.generateInt(1, 5));
        
        // 生成随机label_box
        const int num_classes = gen.generateInt(1, 3);
        for (int c = 0; c < num_classes; ++c) {
            uint8_t class_id = static_cast<uint8_t>(gen.generateInt(0, 2));
            int num_boxes = gen.generateInt(1, 3);
            
            std::vector<std::vector<int>> boxes;
            for (int b = 0; b < num_boxes; ++b) {
                boxes.push_back({
                    gen.generateInt(0, 1920),  // x
                    gen.generateInt(0, 1080),  // y
                    gen.generateInt(10, 200),  // w
                    gen.generateInt(10, 200)   // h
                });
            }
            pkg.label_box[class_id] = boxes;
        }
        
        // 生成测试图像
        pkg.img = gen.generateMockImage(640, 480);
        
        packages.push_back(pkg);
    }
    return packages;
}

// 打印数据包信息
void printPackageInfo(const imgPackage& pkg) {
    std::cout << "=== Package Info ===" << std::endl;
    std::cout << "Timestamp: " << pkg.time << " ms\n";
    std::cout << "UAV ID: " << static_cast<int>(pkg.uav_id) << "\n";
    std::cout << "Object Classes: " << pkg.label_box.size() << "\n";
    
    for (const auto& [cls, boxes] : pkg.label_box) {
        std::cout << "  Class " << static_cast<int>(cls) 
                 << " has " << boxes.size() << " boxes\n";
    }
    
    std::cout << "Image Size: " << pkg.img.cols << "x" << pkg.img.rows 
             << " (Type: " << pkg.img.type() << ")\n";
    std::cout << "====================\n" << std::endl;
}

// 协议验证测试
void testProtocolConsistency(const imgPackage& origin, 
                           const imgPackage& decoded) {
    bool success = true;
    
    // 校验基本字段
    if (origin.time != decoded.time) {
        std::cerr << "Time mismatch: " << origin.time 
                 << " vs " << decoded.time << std::endl;
        success = false;
    }
    
    if (origin.uav_id != decoded.uav_id) {
        std::cerr << "UAV ID mismatch: " << static_cast<int>(origin.uav_id)
                 << " vs " << static_cast<int>(decoded.uav_id) << std::endl;
        success = false;
    }
    
    // 校验label_box结构
    if (origin.label_box.size() != decoded.label_box.size()) {
        std::cerr << "Label box size mismatch: " << origin.label_box.size()
                 << " vs " << decoded.label_box.size() << std::endl;
        success = false;
    }
    
    for (const auto& [cls, boxes] : origin.label_box) {
        if (!decoded.label_box.count(cls)) {
            std::cerr << "Missing class: " << static_cast<int>(cls) << std::endl;
            success = false;
            continue;
        }
        
        if (boxes.size() != decoded.label_box.at(cls).size()) {
            std::cerr << "Box count mismatch for class " << static_cast<int>(cls)
                     << ": " << boxes.size() << " vs " 
                     << decoded.label_box.at(cls).size() << std::endl;
            success = false;
        }
    }
    
    // 校验图像参数
    if (origin.img.size() != decoded.img.size() ||
        origin.img.type() != decoded.img.type()) {
        std::cerr << "Image parameter mismatch!\n";
        success = false;
    }
    
    // 校验图像数据
    if (!std::equal(origin.img.datastart, origin.img.dataend, decoded.img.datastart)) {
        std::cerr << "Image data mismatch!\n";
        success = false;
    }
    
    if (success) {
        std::cout << "Protocol validation PASSED\n";
    } else {
        std::cout << "Protocol validation FAILED\n";
    }
}

int main() {
    TestDataGenerator gen;
    
    // 生成测试数据
    const int NUM_PACKAGES = 3;
    time_t base_time = std::time(nullptr);
    auto packages = generateTestPackages(gen, NUM_PACKAGES, base_time);
    
    // 打印原始数据
    std::cout << "=== Original Packages ===" << std::endl;
    for (const auto& pkg : packages) {
        printPackageInfo(pkg);
    }
    
    // 序列化/反序列化测试
    std::cout << "\n=== Running Protocol Tests ===" << std::endl;
    for (const auto& origin_pkg : packages) {
        std::vector<uint8_t> buffer;
        if (!serializeImgPackage(origin_pkg, buffer)) {
            std::cerr << "Serialization failed!" << std::endl;
            continue;
        }
        
        imgPackage decoded_pkg;
        if (!deserializeImgPackage(buffer.data(), buffer.size(), decoded_pkg)) {
            std::cerr << "Deserialization failed!" << std::endl;
            continue;
        }
        
        std::cout << "\nTesting package with timestamp " << origin_pkg.time << "...\n";
        testProtocolConsistency(origin_pkg, decoded_pkg);
    }
    
    // 压力测试
    std::cout << "\n=== Stress Test ===" << std::endl;
    const int STRESS_TEST_COUNT = 100;
    int success_count = 0;
    
    for (int i = 0; i < STRESS_TEST_COUNT; ++i) {
        auto test_pkg = generateTestPackages(gen, 1, base_time)[0];
        std::vector<uint8_t> buffer;
        
        if (serializeImgPackage(test_pkg, buffer) && 
            deserializeImgPackage(buffer.data(), buffer.size(), test_pkg)) 
        {
            ++success_count;
        }
    }
    
    std::cout << "Stress test success rate: " 
             << (success_count * 100.0 / STRESS_TEST_COUNT) << "%\n";
    
    return 0;
}

#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

// 第一题：二进制编码（黑1白0）
Scalar encode_bin(int bit) {
    return (bit == 1) ? Scalar(0, 0, 0) : Scalar(255, 255, 255);
}

int decode_bin(Scalar color) {
    // 计算颜色亮度，黑色接近0，白色接近255
    int brightness = (color[0] + color[1] + color[2]) / 3;
    return (brightness < 128) ? 1 : 0;
}

// 第二题：八进制编码
Scalar encode_oct(int oct) {
    vector<Scalar> colors = {
        Scalar(0, 0, 0),       //黑
        Scalar(255, 255, 255), //白
        Scalar(0, 0, 255),     //红
        Scalar(255, 0, 0),     //蓝
        Scalar(0, 255, 0),     //绿
        Scalar(255, 0, 255),   //紫
        Scalar(0, 255, 255),   //黄
        Scalar(255, 255, 0)    //青
    };
    return colors[oct % 8];
}

int decode_oct(Scalar color) {
    vector<pair<Scalar, int>> colorMap = {
        {Scalar(0, 0, 0), 0},
        {Scalar(255, 255, 255), 1},   
        {Scalar(0, 0, 255), 2},     
        {Scalar(255, 0, 0), 3},     
        {Scalar(0, 255, 0), 4},   
        {Scalar(255, 0, 255), 5}, 
        {Scalar(0, 255, 255), 6},   
        {Scalar(255, 255, 0), 7} 
    };
    
    // 寻找最接近的颜色
    int minDist = INT_MAX;
    int result = 0;
    for (const auto& pair : colorMap) {
        int dist = norm(color - pair.first);
        if (dist < minDist) {
            minDist = dist;
            result = pair.second;
        }
    }
    return result;
}

// 发送接口
void send_bin(int bit, const string& windowName = "Sender") {
    Mat img(480, 640, CV_8UC3, encode_bin(bit));
    imshow(windowName, img);
    waitKey(1000);  // 显示1秒
}

void send_oct(int oct, const string& windowName = "Sender") {
    Mat img(480, 640, CV_8UC3, encode_oct(oct));
    imshow(windowName, img);
    waitKey(1000);
}

// 接收接口
int receive_bin(VideoCapture& cap) {
    Mat frame;
    if (cap.read(frame)) {
        // 取图像中心区域的平均颜色
        Rect centerRect(frame.cols/2-10, frame.rows/2-10, 20, 20);
        Mat center = frame(centerRect);
        Scalar avgColor = mean(center);
        return decode_bin(avgColor);
    }
    return -1;  // 读取失败
}

int receive_oct(VideoCapture& cap) {
    Mat frame;
    if (cap.read(frame)) {
        // 取图像中心区域的平均颜色
        Rect centerRect(frame.cols/2-10, frame.rows/2-10, 20, 20);
        Mat center = frame(centerRect);
        Scalar avgColor = mean(center);
        return decode_oct(avgColor);
    }
    return -1;  // 读取失败
}

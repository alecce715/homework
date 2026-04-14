#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <algorithm>

using namespace std;

// 定义π常量
const double PI = 3.14159265358979323846;

// 生成原始连续信号（多频率分量）
vector<double> generateSignal(double duration, double sample_rate, 
                             const vector<double>& frequencies, 
                             const vector<double>& amplitudes) {
    int num_samples = static_cast<int>(duration * sample_rate);
    vector<double> signal(num_samples, 0.0);
    double dt = 1.0 / sample_rate;
    
    for (int i = 0; i < num_samples; ++i) {
        double t = i * dt;
        double value = 0.0;
        for (size_t j = 0; j < frequencies.size(); ++j) {
            value += amplitudes[j] * sin(2.0 * PI * frequencies[j] * t);
        }
        signal[i] = value;
    }
    return signal;
}

// 采样函数
vector<pair<double, double>> sampleSignal(const vector<double>& signal, 
                                         double original_rate, 
                                         double sample_rate) {
    vector<pair<double, double>> samples;
    int step = static_cast<int>(original_rate / sample_rate);
    
    for (size_t i = 0; i < signal.size(); i += step) {
        double t = static_cast<double>(i) / original_rate;
        samples.push_back({t, signal[i]});
    }
    return samples;
}

// sinc插值函数
double sinc(double x) {
    if (fabs(x) < 1e-10) return 1.0;
    return sin(PI * x) / (PI * x);
}

// 重建信号（理想低通滤波器的sinc插值）
vector<double> reconstructSignal(const vector<pair<double, double>>& samples,
                                double sample_rate,
                                double reconstruct_rate,
                                double duration,
                                double cutoff_freq) {
    int num_points = static_cast<int>(duration * reconstruct_rate);
    vector<double> reconstructed(num_points, 0.0);
    double T = 1.0 / sample_rate;  // 采样间隔
    
    for (int i = 0; i < num_points; ++i) {
        double t = static_cast<double>(i) / reconstruct_rate;
        double value = 0.0;
        
        for (const auto& sample : samples) {
            double nT = sample.first;
            double sample_value = sample.second;
            value += sample_value * sinc((t - nT) / T) * 2.0 * cutoff_freq * T;
        }
        
        reconstructed[i] = value;
    }
    return reconstructed;
}

// 计算均方误差
double calculateMSE(const vector<double>& original, 
                   const vector<double>& reconstructed) {
    double mse = 0.0;
    int n = min(original.size(), reconstructed.size());
    
    for (int i = 0; i < n; ++i) {
        double diff = original[i] - reconstructed[i];
        mse += diff * diff;
    }
    return mse / n;
}

// 保存数据到文件（用于绘图）
void saveToFile(const string& filename, 
                const vector<double>& signal1, 
                const vector<double>& signal2,
                const vector<pair<double, double>>& samples) {
    ofstream file(filename);
    file << "Time,Original,Reconstructed,Sampled" << endl;
    
    double dt = 1.0 / 1000.0;  // 假设原始信号采样率为1000Hz
    
    // 写入原始信号和重建信号
    for (size_t i = 0; i < min(signal1.size(), signal2.size()); ++i) {
        double t = i * dt;
        file << t << "," << signal1[i] << "," << signal2[i] << ",";
        
        // 检查当前时间点是否有采样
        bool sampled = false;
        for (const auto& s : samples) {
            if (fabs(s.first - t) < dt/2) {
                file << s.second;
                sampled = true;
                break;
            }
        }
        if (!sampled) file << "0";
        file << endl;
    }
    file.close();
}

int main() {
    // 参数设置
    double duration = 0.1;  // 信号时长0.1秒
    double original_rate = 10000;  // 原始信号采样率10kHz（用于模拟连续信号）
    
    // 信号频率分量
    vector<double> frequencies = {50, 120, 300};  // 包含50Hz, 120Hz, 300Hz分量
    vector<double> amplitudes = {1.0, 0.5, 0.3};
    double max_freq = *max_element(frequencies.begin(), frequencies.end());
    
    cout << "信号最高频率: " << max_freq << " Hz" << endl;
    cout << "Nyquist频率: " << 2 * max_freq << " Hz" << endl;
    
    // 生成原始信号
    vector<double> original_signal = generateSignal(duration, original_rate, 
                                                   frequencies, amplitudes);
    
    // 情况1：采样频率大于2倍最高频率（满足Nyquist定理）
    double sample_rate1 = 2.2 * max_freq;  // 660Hz > 600Hz
    cout << "\n情况1：采样频率 = " << sample_rate1 << " Hz (> 2*" << max_freq << " Hz)" << endl;
    
    vector<pair<double, double>> samples1 = sampleSignal(original_signal, 
                                                        original_rate, 
                                                        sample_rate1);
    
    vector<double> reconstructed1 = reconstructSignal(samples1, sample_rate1,
                                                     original_rate, duration,
                                                     max_freq);
    
    double mse1 = calculateMSE(original_signal, reconstructed1);
    cout << "重建信号均方误差: " << mse1 << endl;
    
    // 情况2：采样频率小于2倍最高频率（不满足Nyquist定理）
    double sample_rate2 = 1.8 * max_freq;  // 540Hz < 600Hz
    cout << "\n情况2：采样频率 = " << sample_rate2 << " Hz (< 2*" << max_freq << " Hz)" << endl;
    
    vector<pair<double, double>> samples2 = sampleSignal(original_signal,
                                                        original_rate,
                                                        sample_rate2);
    
    vector<double> reconstructed2 = reconstructSignal(samples2, sample_rate2,
                                                     original_rate, duration,
                                                     max_freq);
    
    double mse2 = calculateMSE(original_signal, reconstructed2);
    cout << "重建信号均方误差: " << mse2 << endl;
    
    // 保存数据用于分析
    saveToFile("sampling_data_case1.csv", original_signal, reconstructed1, samples1);
    saveToFile("sampling_data_case2.csv", original_signal, reconstructed2, samples2);
    
    cout << "\n数据已保存到文件:" << endl;
    cout << "- sampling_data_case1.csv (满足Nyquist条件)" << endl;
    cout << "- sampling_data_case2.csv (不满足Nyquist条件)" << endl;
    cout << "\n使用Excel或Python matplotlib绘图可观察混叠效应" << endl;
    
    // 混叠现象演示
    cout << "\n混叠现象演示：" << endl;
    // 生成一个300Hz的单频信号
    vector<double> single_freq = {300};
    vector<double> single_amp = {1.0};
    vector<double> single_signal = generateSignal(duration, original_rate,
                                                 single_freq, single_amp);
    
    // 用500Hz采样（小于600Hz，将产生混叠）
    double alias_rate = 500;  // 500Hz < 600Hz
    vector<pair<double, double>> alias_samples = sampleSignal(single_signal,
                                                             original_rate,
                                                             alias_rate);
    
    // 计算混叠频率
    double alias_freq = abs(alias_rate - 300);
    cout << "原始频率: 300 Hz" << endl;
    cout << "采样频率: " << alias_rate << " Hz" << endl;
    cout << "混叠频率: " << alias_freq << " Hz" << endl;
    
    return 0;
}

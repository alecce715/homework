#include <iostream> 
#include <cmath>
#include <cstring>
using namespace std;
// 编码函数：将消息转换为RS-232C电压序列
int rs232c_encode(double *volts, int volts_size, const char *msg, int size) {
    if (!volts || !msg || volts_size <= 0 || size <= 0) {
        return -1;
    }
    
    int volt_idx = 0;
    
    // 空闲位（逻辑1），负电压
    if (volt_idx < volts_size) volts[volt_idx++] = -12.0;
    
    // 大端序：从消息的最后一个字符开始处理
    for (int msg_idx = size - 1; msg_idx >= 0; --msg_idx) {
        char ch = msg[msg_idx] & 0x7F;  // 只取低7位
        
        // 起始位（逻辑0），正电压
        if (volt_idx >= volts_size) return -1;
        volts[volt_idx++] = 12.0;
        
        // 位小端序：从最低位(LSB)到最高位(MSB)传输7个数据位
        for (int bit = 0; bit < 7; ++bit) {
            if (volt_idx >= volts_size) return -1;
            
            // 检查第bit位
            if (ch & (1 << bit)) {
                volts[volt_idx++] = -12.0;  // 逻辑1，负电压
            } else {
                volts[volt_idx++] = 12.0;   // 逻辑0，正电压
            }
        }
        
        // 停止位（逻辑1），负电压
        if (volt_idx >= volts_size) return -1;
        volts[volt_idx++] = -12.0;
    }
    
    return volt_idx;  // 返回生成的电压值个数
}

// 解码函数：从RS-232C电压序列恢复消息
int rs232c_decode(char *msg, int size, const double *volts, int volts_size) {
    if (!msg || !volts || size <= 0 || volts_size <= 0) {
        return -1;
    }
    
    int msg_len = 0;
    int volt_idx = 0;
    
    // 跳过一个空闲位
    if (volt_idx >= volts_size) return 0;
    volt_idx++;
    
    // 临时缓冲区，用于存储解码的字符（之后需要反转）
    char *temp_buffer = new char[size];
    int temp_idx = 0;
    
    while (volt_idx < volts_size && temp_idx < size) {
        // 检查起始位
        if (volt_idx >= volts_size) break;
        if (volts[volt_idx] < 0) {  // 应该是正电压（逻辑0）
            // 同步丢失，跳过直到找到起始位
            volt_idx++;
            continue;
        }
        volt_idx++;  // 跳过起始位
        
        // 解码7个数据位（位小端序）
        char ch = 0;
        for (int bit = 0; bit < 7; ++bit) {
            if (volt_idx >= volts_size) {
                delete[] temp_buffer;
                return -1;
            }
            
            if (volts[volt_idx] < 0) {  // 负电压，逻辑1
                ch |= (1 << bit);
            }
            // 逻辑0不设置位
            volt_idx++;
        }
        
        // 检查停止位
        if (volt_idx >= volts_size) {
            delete[] temp_buffer;
            return -1;
        }
        if (volts[volt_idx] < 0) {  // 应该是负电压（逻辑1）
            temp_buffer[temp_idx++] = ch;
        } else {
            // 帧错误
            delete[] temp_buffer;
            return -1;
        }
        volt_idx++;
        
        // 跳过帧间空闲位（如果有）
        while (volt_idx < volts_size && volts[volt_idx] < 0) {
            volt_idx++;
        }
    }
    
    // 字节大端序反转：将临时缓冲区反向复制到输出
    for (int i = 0; i < temp_idx; ++i) {
        msg[i] = temp_buffer[temp_idx - 1 - i];
    }
    msg_len = temp_idx;
    msg[msg_len] = '\0';  // 字符串结尾
    
    delete[] temp_buffer;
    return msg_len;
}

int main() {
    // 测试编解码一个字符串 
    {
        const char original_msg[] = "RS232";
        const int msg_size = 5;
        const int max_volts = 100;  // 每个字符10个电压值 × 5个字符 = 50
        
        double volts[max_volts] = {0};
        char decoded_msg[6] = {0};  // 5个字符 + '\0'
        
        // 编码
        int volts_count = rs232c_encode(volts, max_volts, original_msg, msg_size);
        
        if (volts_count <= 0) {
            cout << "编码失败" << endl;
        } else {
            cout << "编码成功 电压序列长度: " << volts_count 
                      << " (应为 " << (msg_size * 10) << ")" << endl;
            
            // 解码
            int decoded_size = rs232c_decode(decoded_msg, 6, volts, volts_count);
            
            if (decoded_size <= 0) {
                cout << "解码失败" <<endl;
            } else {
                cout << "解码成功 原始字符串: \"" << original_msg << "\"" <<endl;
                cout << "解码字符串: \"" << decoded_msg << "\"" <<endl;
                
                if (strcmp(decoded_msg, original_msg) == 0) {
                    cout << "测试通过" <<endl;
                } else {
                    cout << "测试失败: 解码字符串与原始字符串不匹配!" <<endl;
                }
            }
        }
        cout <<endl;
    }}

#include <stdio.h>

// 返回1校验通过返回0校验失败
int parity_check(const unsigned char *msg, const int msg_length)
{
    int cnt = 0;
    for (int i = 0; i < msg_length; i++)
    {
        if (msg[i] != 0)
            cnt++;
    }
    return (cnt % 2 == 0) ? 1 : 0;
}


// 测试
int main()
{
    unsigned char msg1[] = {0,1,0,1,0,1,0,1};
    unsigned char msg2[] = {0,1,0,1,1,1,0,1};
    printf("%d\n", parity_check(msg1, 8));
    printf("%d\n", parity_check(msg2, 8));
    return 0;
}

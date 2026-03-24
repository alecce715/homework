#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
int statistical_tdm_multiplex(unsigned char *c, const int c_size, const unsigned char *a, const int a_len, const unsigned char *b, const int b_len) {
    int idx = 0;
    if (c_size < 4) return -1;
    c[0] = (a_len >> 8) & 0xFF;
    c[1] = a_len & 0xFF;
    c[2] = (b_len >> 8) & 0xFF;
    c[3] = b_len & 0xFF;
    idx = 4;
    for (int i = 0; i < a_len; ++i) {
        if (a[i] != 0) {
            if (idx + 3 > c_size) return -1;
            c[idx++] = 0;
            c[idx++] = i & 0xFF;
            c[idx++] = 1;
        }
    }
    for (int i = 0; i < b_len; ++i) {
        if (b[i] != 0) {
            if (idx + 3 > c_size) return -1;
            c[idx++] = 1;
            c[idx++] = i & 0xFF;
            c[idx++] = 1;
        }
    }
    return idx;
}

int statistical_tdm_demultiplex(unsigned char *a, const int a_size, unsigned char *b, const int b_size, const unsigned char *c, const int c_len) {
    if (c_len < 4) return -1;
    int a_len = (c[0] << 8) | c[1];
    int b_len = (c[2] << 8) | c[3];
    if (a_len > a_size || b_len > b_size) return -1;
    for (int i = 0; i < a_len; ++i) a[i] = 0;
    for (int i = 0; i < b_len; ++i) b[i] = 0;
    int idx = 4;
    while (idx + 2 < c_len) {
        unsigned char stream = c[idx++];
        int pos = c[idx++];
        unsigned char val = c[idx++];
        if (stream == 0 && pos < a_len) a[pos] = val;
        else if (stream == 1 && pos < b_len) b[pos] = val;
    }
    return 0;
}


int synchronous_tdm_multiplex(unsigned char *c, const int c_size, const unsigned char *a, const int a_len, const unsigned char *b, const int b_len) {
    if (c_size < a_len + b_len) return -1;
    for (int i = 0; i < a_len && i < b_len; ++i) {
        c[2*i] = (a[i] != 0) ? 1 : 0;
        c[2*i+1] = (b[i] != 0) ? 1 : 0;
    }
    if (a_len > b_len) {
        for (int i = b_len; i < a_len; ++i) c[b_len + i] = (a[i] != 0) ? 1 : 0;
    } else if (b_len > a_len) {
        for (int i = a_len; i < b_len; ++i) c[a_len + i] = (b[i] != 0) ? 1 : 0;
    }
    return (a_len + b_len);
}

int synchronous_tdm_demultiplex(unsigned char *a, const int a_size, unsigned char *b, const int b_size, const unsigned char *c, const int c_len) {
    int total = 0;
    int ai = 0, bi = 0;
    for (int i = 0; i < c_len; ++i) {
        if (i % 2 == 0) {
            if (ai < a_size) a[ai++] = c[i];
        } else {
            if (bi < b_size) b[bi++] = c[i];
        }
        total++;
    }
    return total;
}


int fdm_multiplex(unsigned double *c, const int c_size, const unsigned char *a, const int a_len, const unsigned char *b, const int b_len, double f1, double f2, double fs, int samples_per_bit) {
    int total_samples = samples_per_bit * std::max(a_len, b_len);
    if (c_size < total_samples) return -1;
    for (int i = 0; i < total_samples; ++i) c[i] = 0.0;
    for (int i = 0; i < a_len; ++i) {
        double bit = (a[i] != 0) ? 1.0 : 0.0;
        for (int k = 0; k < samples_per_bit; ++k) {
            double t = (i * samples_per_bit + k) / fs;
            c[i * samples_per_bit + k] += bit * sin(2 * M_PI * f1 * t);
        }
    }
    for (int i = 0; i < b_len; ++i) {
        double bit = (b[i] != 0) ? 1.0 : 0.0;
        for (int k = 0; k < samples_per_bit; ++k) {
            double t = (i * samples_per_bit + k) / fs;
            c[i * samples_per_bit + k] += bit * sin(2 * M_PI * f2 * t);
        }
    }
    return total_samples;
}

int fdm_demultiplex(unsigned char *a, const int a_size, unsigned char *b, const int b_size, const unsigned double *c, const int c_len, double f1, double f2, double fs, int samples_per_bit) {
    int bits_a = std::min(a_size, c_len / samples_per_bit);
    int bits_b = std::min(b_size, c_len / samples_per_bit);
    for (int i = 0; i < bits_a; ++i) {
        double sum = 0.0;
        for (int k = 0; k < samples_per_bit; ++k) {
            double t = (i * samples_per_bit + k) / fs;
            sum += c[i * samples_per_bit + k] * sin(2 * M_PI * f1 * t);
        }
        a[i] = (sum > 0.5 * samples_per_bit) ? 1 : 0;
    }
    for (int i = 0; i < bits_b; ++i) {
        double sum = 0.0;
        for (int k = 0; k < samples_per_bit; ++k) {
            double t = (i * samples_per_bit + k) / fs;
            sum += c[i * samples_per_bit + k] * sin(2 * M_PI * f2 * t);
        }
        b[i] = (sum > 0.5 * samples_per_bit) ? 1 : 0;
    }
    return 0;
}

Âë·Ö
void generate_walsh_code(int *code1, int *code2, int len) {
    for (int i = 0; i < len; ++i) {
        code1[i] = 1;
        code2[i] = (i % 2 == 0) ? 1 : -1;
    }
}

int cdm_multiplex(unsigned double *c, const int c_size, const unsigned char *a, const int a_len, const unsigned char *b, const int b_len, int spread_factor) {
    int code1[8], code2[8];
    generate_walsh_code(code1, code2, spread_factor);
    int total_len = spread_factor * std::max(a_len, b_len);
    if (c_size < total_len) return -1;
    for (int i = 0; i < total_len; ++i) c[i] = 0.0;
    for (int i = 0; i < a_len; ++i) {
        double bit_a = (a[i] != 0) ? 1.0 : -1.0;
        for (int k = 0; k < spread_factor; ++k) c[i * spread_factor + k] += bit_a * code1[k];
    }
    for (int i = 0; i < b_len; ++i) {
        double bit_b = (b[i] != 0) ? 1.0 : -1.0;
        for (int k = 0; k < spread_factor; ++k) c[i * spread_factor + k] += bit_b * code2[k];
    }
    return total_len;
}

int cdm_demultiplex(unsigned char *a, const int a_size, unsigned char *b, const int b_size, const unsigned double *c, const int c_len, int spread_factor) {
    int code1[8], code2[8];
    generate_walsh_code(code1, code2, spread_factor);
    int bits_a = std::min(a_size, c_len / spread_factor);
    int bits_b = std::min(b_size, c_len / spread_factor);
    for (int i = 0; i < bits_a; ++i) {
        double sum = 0.0;
        for (int k = 0; k < spread_factor; ++k) sum += c[i * spread_factor + k] * code1[k];
        a[i] = (sum > 0) ? 1 : 0;
    }
    for (int i = 0; i < bits_b; ++i) {
        double sum = 0.0;
        for (int k = 0; k < spread_factor; ++k) sum += c[i * spread_factor + k] * code2[k];
        b[i] = (sum > 0) ? 1 : 0;
    }
    return 0;
}

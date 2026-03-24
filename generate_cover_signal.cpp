int generate_cover_signal(unsigned double *cover, const int size, double freq, double fs) {
    for (int i = 0; i < size; ++i) cover[i] = sin(2 * M_PI * freq * i / fs);
    return 0;
}

int simulate_digital_modulation_signal(unsigned char *message, const int size) {
    for (int i = 0; i < size; ++i) message[i] = rand() % 2;
    return 0;
}

int simulate_analog_modulation_signal(unsigned double *message, const int size) {
    for (int i = 0; i < size; ++i) message[i] = (sin(2 * M_PI * 0.01 * i) + 1.0) / 2.0;
    return 0;
}

int modulate_digital_frequency(unsigned double *cover, const int cover_len, const unsigned char *message, const int msg_len, double f0, double f1, double fs, int samples_per_bit) {
    if (cover_len < msg_len * samples_per_bit) return -1;
    for (int i = 0; i < msg_len; ++i) {
        double freq = (message[i] != 0) ? f1 : f0;
        for (int k = 0; k < samples_per_bit; ++k) {
            double t = (i * samples_per_bit + k) / fs;
            cover[i * samples_per_bit + k] = sin(2 * M_PI * freq * t);
        }
    }
    return 0;
}

int modulate_analog_frequency(unsigned double *cover, const int cover_len, const unsigned double *message, const int msg_len, double fc, double kf, double fs) {
    if (cover_len < msg_len) return -1;
    for (int i = 0; i < cover_len; ++i) {
        double phase = 2 * M_PI * fc * i / fs + 2 * M_PI * kf * message[i];
        cover[i] = sin(phase);
    }
    return 0;
}

int modulate_digital_amplitude(unsigned double *cover, const int cover_len, const unsigned char *message, const int msg_len, double fc, double fs, int samples_per_bit) {
    if (cover_len < msg_len * samples_per_bit) return -1;
    for (int i = 0; i < msg_len; ++i) {
        double amp = (message[i] != 0) ? 1.0 : 0.0;
        for (int k = 0; k < samples_per_bit; ++k) {
            double t = (i * samples_per_bit + k) / fs;
            cover[i * samples_per_bit + k] = amp * sin(2 * M_PI * fc * t);
        }
    }
    return 0;
}

int modulate_analog_amplitude(unsigned double *cover, const int cover_len, const unsigned double *message, const int msg_len, double fc, double fs) {
    if (cover_len < msg_len) return -1;
    for (int i = 0; i < cover_len; ++i) {
        cover[i] = message[i] * sin(2 * M_PI * fc * i / fs);
    }
    return 0;
}

int modulate_digital_phase(unsigned double *cover, const int cover_len, const unsigned char *message, const int msg_len, double fc, double fs, int samples_per_bit) {
    if (cover_len < msg_len * samples_per_bit) return -1;
    for (int i = 0; i < msg_len; ++i) {
        double phase = (message[i] != 0) ? M_PI : 0.0;
        for (int k = 0; k < samples_per_bit; ++k) {
            double t = (i * samples_per_bit + k) / fs;
            cover[i * samples_per_bit + k] = sin(2 * M_PI * fc * t + phase);
        }
    }
    return 0;
}

int modulate_analog_phase(unsigned double *cover, const int cover_len, const unsigned double *message, const int msg_len, double fc, double kp, double fs) {
    if (cover_len < msg_len) return -1;
    for (int i = 0; i < cover_len; ++i) {
        cover[i] = sin(2 * M_PI * fc * i / fs + kp * message[i]);
    }
    return 0;
}

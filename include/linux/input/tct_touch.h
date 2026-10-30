#define VERTICAL           1
#define HORIZONTAL_LEFT    2
#define HORIZONTAL_RIGHT   3

struct tct_touch_data {
    u8 tct_grip_mode;
    u8 tct_grip_level;
    struct mutex tct_touch_mutex;
};

extern struct tct_touch_data *tct_ts_data;
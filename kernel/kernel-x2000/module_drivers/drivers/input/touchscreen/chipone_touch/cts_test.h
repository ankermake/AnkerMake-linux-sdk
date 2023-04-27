#ifndef CTS_TEST_H
#define CTS_TEST_H

struct cts_device;


/*------------------------------------ CSVFILE PARA--------------------------------------*/
#define CTS_CSV_FILE_NAME_LENS                  64
#define CTS_CSV_FILE_PATH_LENS                  128

#define CTS_CSV_FILE_IN_PRODUCT                 0
#define CTS_CSV_FILE_IN_ODM                     1

#define CTS_CSV_PATH_PERF_PRODUCT        "/sdcard/"
#define CTS_CSV_PATH_PERF_ODM            "/sdcard/"

#define CTS_CSV_RESET_TEST_ENABLE        "reset_test_enable"
#define CTS_CSV_INT_TEST_ENABLE          "int_test_enable"

#define CTS_CSV_RAW_TEST_ENABLE          "raw_test_enable"
#define CTS_CSV_RAW_TEST_FRAME           "raw_test_frame"
#define CTS_CSV_MUTUAL_RAW_LIMIT_MAX     "mutual_raw_limit_max"
#define CTS_CSV_MUTUAL_RAW_LIMIT_MIN     "mutual_raw_limit_min"
#define CTS_CSV_SELF_RAW_LIMIT_MAX       "self_raw_limit_max"
#define CTS_CSV_SELF_RAW_LIMIT_MIN       "self_raw_limit_min"

#define CTS_CSV_OPEN_TEST_ENABLE      	 "open_test_enable"
#define CTS_CSV_OPEN_LIMIT_MAX     		 "open_limit_max"
#define CTS_CSV_OPEN_LIMIT_MIN     		 "open_limit_min"

#define CTS_CSV_DEVIATION_TEST_ENABLE    "deviation_test_enable"
#define CTS_CSV_DEVIATION_TEST_FRAME     "deviation_test_frame"
#define CTS_CSV_MUTUAL_DEVIATION_LIMIT   "mutual_deviation_limit"
#define CTS_CSV_SELF_DEVIATION_LIMIT     "self_deviation_limit"

#define CTS_CSV_NOISE_TEST_ENABLE        "noise_test_enable"
#define CTS_CSV_NOISE_TEST_FRAME         "noise_test_frame"
#define CTS_CSV_NOISE_TEST_FRAME         "noise_test_frame"
#define CTS_CSV_MUTUAL_NOISE_LIMIT       "mutual_noise_limit"
#define CTS_CSV_SELF_NOISE_LIMIT         "self_noise_limit"

#define CTS_CSV_SHORT_TEST_ENABLE        "short_test_enable"
#define CTS_CSV_SHORT_THRESHOLD          "short_threshold"
#define RX_MAX_CHANNEL_NUM                  8
#define TX_MAX_CHANNEL_NUM                  8
#define CTS_MAX_MUTUAL_NOTES           (RX_MAX_CHANNEL_NUM * TX_MAX_CHANNEL_NUM)
#define CTS_MAX_SELF_NOTES             (RX_MAX_CHANNEL_NUM + TX_MAX_CHANNEL_NUM)

#define CTS_MAX_DATA_NOTES             (CTS_MAX_MUTUAL_NOTES + CTS_MAX_SELF_NOTES)



#define CTS_TEST_FLAG_RESET_BEFORE_TEST             (1u << 0)
#define CTS_TEST_FLAG_RESET_AFTER_TEST              (1u << 1)
#define CTS_TEST_FLAG_DISPLAY_ON                    (1u << 2)
#define CTS_TEST_FLAG_DISABLE_GAS                   (1u << 3)
#define CTS_TEST_FLAG_DISABLE_LINESHIFT             (1u << 4)

#define CTS_TEST_FLAG_VALIDATE_DATA                 (1u << 8)
#define CTS_TEST_FLAG_VALIDATE_PER_NODE             (1u << 9)
#define CTS_TEST_FLAG_VALIDATE_MIN                  (1u << 10)
#define CTS_TEST_FLAG_VALIDATE_MAX                  (1u << 11)
#define CTS_TEST_FLAG_VALIDATE_SKIP_INVALID_NODE    (1u << 12)
#define CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED  (1u << 13)

#define CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE     (1u << 16)
#define CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE   (1u << 17)
#define CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE        (1u << 18)
#define CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE_APPEND (1u << 19)
#define CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE_CSV    (1u << 20)

#define CTS_TEST_FLAG_DRIVER_LOG_TO_USERSPACE       (1u << 24)
#define CTS_TEST_FLAG_DRIVER_LOG_TO_FILE            (1u << 25)
#define CTS_TEST_FLAG_DRIVER_LOG_TO_FILE_APPEND     (1u << 26)

#define MAKE_INVALID_NODE(r,c)      (((c) << 16) | (r))
#define INVALID_NODE_ROW(node)      ((u16)(node))
#define INVALID_NODE_COL(node)      ((u16)((node) >> 16))

enum cts_test_item {
    CTS_TEST_RESET_PIN = 1,
    CTS_TEST_INT_PIN,
    CTS_TEST_RAWDATA,
    CTS_TEST_DEVIATION,
    CTS_TEST_NOISE,
    CTS_TEST_OPEN,
    CTS_TEST_SHORT,
};


struct cts_test_param {
    int test_item;

    __u32 flags;

    __u32 num_invalid_node;
    __u32 *invalid_nodes;
    int *min;
    int *max;
    int *self_min;
    int *self_max;
	int max_limits[CTS_MAX_DATA_NOTES];
	int min_limits[CTS_MAX_DATA_NOTES];

    int	*test_result;

    void	*test_data_buf;
    int test_data_buf_size;
    int *test_data_wr_size;
    const char *test_data_filepath;

    int driver_log_level;
    char *driver_log_buf;
    int driver_log_buf_size;
    int *driver_log_wr_size;
    const char *driver_log_filepath;

    void *priv_param;
    int priv_param_size;
};

struct cts_rawdata_test_priv_param {
    __u32 frames;
    //__u8  work_mode;
};

struct cts_deviation_test_priv_param {
    __u32 frames;
};


struct cts_noise_test_priv_param {
    __u32 frames;
};

enum adc_data 
{
    CHANNEL_OFFSET = 0,
    INTERNAL_REFERENCE_RESISTOR,
    ODD_RX_TO_GROUND_SHORT_RESISTOR,
    ODD_TX_TO_GROUND_SHORT_RESISTOR,
};

extern const char *cts_test_item_str(int test_item);
extern int cts_write_file(struct file *filp, const void *data, size_t size);
extern int cts_mkdir_for_file(const char *filepath, umode_t mode);

extern int cts_start_dump_test_data_to_file(const char *filepath,
    bool append_to_file);
extern void cts_stop_dump_test_data_to_file(void);

extern int cts_test_reset_pin(struct cts_device *cts_dev,
    struct cts_test_param *param);
extern int cts_test_int_pin(struct cts_device *cts_dev,
    struct cts_test_param *param);
extern int cts_test_rawdata(struct cts_device *cts_dev,
    struct cts_test_param *param);
extern int cts_test_noise(struct cts_device *cts_dev,
    struct cts_test_param *param);
extern int cts_test_open(struct cts_device *cts_dev,
    struct cts_test_param *param);
extern int cts_test_short(struct cts_device *cts_dev,
    struct cts_test_param *param);


extern int cts_get_shortdata(const struct cts_device *cts_dev, void *buf);

extern int cts_plat_get_int_pin( struct cts_device *cts_dev);
extern int ts_kit_parse_csvfile(char *file_path, char *target_name,
			 int32_t *data, int rows, int columns);
extern int cts_init_testlimits_from_csvfile(struct cts_device *cts_dev);
extern int cts_test_deviation(struct cts_device *cts_dev,
	struct cts_test_param *param);
extern int cts_test_single_drive(struct cts_device *cts_dev,
	 struct cts_test_param *param);

 extern int init_factory_test_param(void);
 extern void print_factory_test_result_to_console(void);
extern  void do_factory_test(void);

#endif /* CTS_TEST_H */


#define LOG_TAG         "Test"

#include "cts_config.h"
#include "cts_platform.h"
#include "cts_core.h"
#include "cts_strerror.h"
#include "cts_test.h"
#include <linux/syscalls.h>

const char *cts_test_item_str(int test_item)
{
#define case_test_item(item) \
    case CTS_TEST_ ## item: return #item "-TEST"

	switch (test_item) {
		case_test_item(RESET_PIN);
		case_test_item(INT_PIN);
		case_test_item(RAWDATA);
		case_test_item(DEVIATION);
		case_test_item(NOISE);
		case_test_item(OPEN);
		case_test_item(SHORT);

	default:
		return "INVALID";
	}
#undef case_test_item
}

#define CTS_FIRMWARE_WORK_MODE_NORMAL   (0x00)
#define CTS_FIRMWARE_WORK_MODE_FACTORY  (0x01)
#define CTS_FIRMWARE_WORK_MODE_CONFIG   (0x02)
#define CTS_FIRMWARE_WORK_MODE_TEST     (0x03)

#define CTS_TEST_SHORT                  (0x01)
#define CTS_TEST_OPEN                   (0x02)

#define CTS_SHORT_TEST_UNDEFINED        (0x00)
#define CTS_SHORT_TEST_BETWEEN_COLS     (0x01)
#define CTS_SHORT_TEST_BETWEEN_ROWS     (0x02)
#define CTS_SHORT_TEST_BETWEEN_GND      (0x03)

#define TEST_RESULT_BUFFER_SIZE(cts_dev) \
    (cts_dev->fwdata.rows * cts_dev->fwdata.cols * 2)

#define RAWDATA_BUFFER_SIZE(cts_dev) \
	(cts_dev->fwdata.rows * cts_dev->fwdata.cols * 2)

static int disable_fw_monitor_mode(struct cts_device *cts_dev)
{
	int ret;
	u8 value;

	ret = cts_send_command(cts_dev, CTS_CMD_MONITOR_OFF);
	if (ret) {
		cts_err("Send command CTS_CMD_MONITOR_OFF failed %d", ret);
	}

	return ret;
}

static int disable_fw_low_power(struct cts_device *cts_dev)
{
	int ret;
	u8 value;

	ret = cts_send_command(cts_dev, CTS_CMD_LOW_POWER_OFF);
	if (ret) {
		cts_err("Send command CTS_CMD_LOW_POWER_OFF failed %d", ret);
	}

	return ret;
}

static int disable_fw_gesture_mode(struct cts_device *cts_dev)
{
	int ret;
	u8 value;

	ret = cts_send_command(cts_dev, CTS_CMD_QUIT_GESTURE_MONITOR);
	if (ret)
		cts_err(
		"Send command CTS_CMD_QUIT_GESTURE_MONITOR failed %d", ret);

	return ret;
}

int cts_write_file(struct file *filp, const void *data, size_t size)
{
	loff_t pos;
	ssize_t ret;

	pos = filp->f_pos;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
	ret = kernel_write(filp, data, size, &pos);
#else
	ret = kernel_write(filp, data, size, pos);
#endif

	if (ret >= 0) {
		filp->f_pos += ret;
	}

	return ret;
}

/* Make directory for filepath
 * If filepath = "/A/B/C/D.file", it will make dir /A/B/C recursive
 * like userspace mkdir -p
 */
int cts_mkdir_for_file(const char *filepath, umode_t mode)
{
	char *dirname = NULL;
	int dirname_len;
	char *s;
	int ret;
	mm_segment_t fs;

	if (filepath == NULL) {
		cts_err("Create dir for file with filepath = NULL");
		return -EINVAL;
	}

	if (filepath[0] == '\0' || filepath[0] != '/') {
		cts_err("Create dir for file with invalid filepath[0]: %c",
			filepath[0]);
		return -EINVAL;
	}

	dirname_len = strrchr(filepath, '/') - filepath;
	if (dirname_len == 0) {
		cts_info("Create dir for file '%s' in root dir", filepath);
		return 0;
	}

	dirname = kstrndup(filepath, dirname_len, GFP_KERNEL);
	if (dirname == NULL) {
		cts_err("Create dir alloc mem for dirname failed");
		return -ENOMEM;
	}

	cts_info("Create dir '%s' for file '%s'", dirname, filepath);

	fs = get_fs();
	set_fs(KERNEL_DS);

	s = dirname + 1;	/* Skip leading '/' */
	while (1) {
		char c = '\0';

		/* Bypass leading non-'/'s and then subsequent '/'s */
		while (*s) {
			if (*s == '/') {
				do {
					++s;
				} while (*s == '/');
				c = *s;	/* Save current char */
				*s = '\0';	/* and replace it with nul */
				break;
			}
			++s;
		}

		cts_dbg(" - Create dir '%s'", dirname);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
		ret = ksys_mkdir(dirname, 0777);
#else
       // ret = sys_mkdir(dirname, 0777);
        ret = 0;
#endif
		if (ret < 0 && ret != -EEXIST) {
			cts_info("Create dir '%s' failed %d(%s)",
				 dirname, ret, cts_strerror(ret));
			/* Remove any inserted nul from the path */
			*s = c;
			break;
		}
		/* Reset ret to 0 if return -EEXIST */
		ret = 0;

		if (c) {
			/* Remove any inserted nul from the path */
			*s = c;
		} else {
			break;
		}
	}

	set_fs(fs);

    if (dirname) {
        kfree(dirname);
    }

    return ret;
}

struct file *cts_test_data_filp = NULL;
int cts_start_dump_test_data_to_file(const char *filepath, bool append_to_file)
{
	int ret;

#define START_BANNER \
		">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n"

	cts_info("Start dump test data to file '%s'", filepath);

	ret = cts_mkdir_for_file(filepath, 0777);
	if (ret) {
		cts_err("Create dir for test data file failed %d", ret);
		return ret;
	}

	cts_test_data_filp = filp_open(filepath,
				       O_WRONLY | O_CREAT | (append_to_file ?
							     O_APPEND :
							     O_TRUNC),
				       S_IRUGO | S_IWUGO);
	if (IS_ERR(cts_test_data_filp)) {
		ret = PTR_ERR(cts_test_data_filp);
		cts_test_data_filp = NULL;
		cts_err("Open file '%s' for test data failed %d",
			cts_test_data_filp, ret);
		return ret;
	}

	cts_write_file(cts_test_data_filp, START_BANNER, strlen(START_BANNER));

	return 0;
#undef START_BANNER
}

void cts_stop_dump_test_data_to_file(void)
{
#define END_BANNER \
    "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n"
	int r;

	cts_info("Stop dump test data to file");

	if (cts_test_data_filp) {
		cts_write_file(cts_test_data_filp,
			       END_BANNER, strlen(END_BANNER));
		r = filp_close(cts_test_data_filp, NULL);
		if (r) {
			cts_err("Close test data file failed %d", r);
		}
		cts_test_data_filp = NULL;
	} else {
		cts_info("Stop dump tsdata to file with filp = NULL");
	}
#undef END_BANNER
}

static void cts_dump_tsdata(struct cts_device *cts_dev,
			    const char *desc, const u16 *data, bool to_console)
{
#define SPLIT_LINE_STR \
    "---------------------------------------------------------------------------------------------------------------"
#define ROW_NUM_FORMAT_STR  "%2d | "
#define COL_NUM_FORMAT_STR  "%-5u "
#define DATA_FORMAT_STR     "%-5u "

    int r, c;
    u32 max, min, sum, average;
    int max_r, max_c, min_r, min_c;
    char line_buf[128];
    int count = 0;
	int is_short_test = 0;
	int dis_rows= cts_dev->fwdata.rows;
	int dis_cols= cts_dev->fwdata.cols;
    char *p = NULL;
	u32 dump_data[CTS_MAX_DATA_NOTES] = {0};
	u32 val;
 
    if(strstr(desc,"Short") != NULL){
        dis_rows= 1;
	    dis_cols= cts_dev->fwdata.cols + cts_dev->fwdata.rows;;
		is_short_test = 1;
	 }
	else if(strstr(desc,"Self") != NULL){
	    dis_rows= 1;
        dis_cols= cts_dev->fwdata.cols + cts_dev->fwdata.rows;  
	 }
    max = min = data[0];
    
    sum = 0;
	val = 0;
    max_r = max_c = min_r = min_c = 0;
    for (r = 0; r < dis_rows; r++) {
        for (c = 0; c < dis_cols; c++) {
		#if 1	
			if(is_short_test){
				//dump_data[r * dis_cols + c] =  data[2*(r * dis_cols + c)+1]<<16 | data[2*(r * dis_cols + c)];
				dump_data[r * dis_cols + c] =  data[2*(r * dis_cols + c)+1] << 16 | data[2*(r * dis_cols + c)] ;
			    cts_info("%x,%x,%x,", data[2*(r * dis_cols + c)],data[2*(r * dis_cols + c)+1],dump_data[r * dis_cols + c]);
				}
			else
				dump_data[r * dis_cols + c] = data[r * dis_cols + c];
			
            val = dump_data[r * dis_cols + c];

			
		#else
		    val = data[r * dis_cols + c];
		#endif
         
            sum += val;
            if (val > max) {
                max = val;
                max_r = r;
                max_c = c;
            } else if (val < min) {
                min = val;
                min_r = r;
                min_c = c;
            }
        }
    }
    average = sum / (dis_rows * dis_cols);

    count = 0;
    count += scnprintf(line_buf + count, sizeof(line_buf) - count,
        " %s test data MIN: [%u][%u]=%u, MAX: [%u][%u]=%u, AVG=%u",
        desc, min_r, min_c, min, max_r, max_c, max, average);

  
    if (to_console) {
        cts_info(SPLIT_LINE_STR);
        cts_info("%s", line_buf);
        cts_info(SPLIT_LINE_STR);
    }
    if (cts_test_data_filp) {
        cts_write_file(cts_test_data_filp, SPLIT_LINE_STR, strlen(SPLIT_LINE_STR));
        cts_write_file(cts_test_data_filp, "\n", 1);
        cts_write_file(cts_test_data_filp, line_buf, count);
        cts_write_file(cts_test_data_filp, "\n", 1);
        cts_write_file(cts_test_data_filp, SPLIT_LINE_STR, strlen(SPLIT_LINE_STR));
        cts_write_file(cts_test_data_filp, "\n", 1);
    }

    count = 0;
    count += scnprintf(line_buf + count, sizeof(line_buf) - count, "   |  ");
    for (c = 0; c <dis_cols; c++) {
        count += scnprintf(line_buf + count, sizeof(line_buf) - count,
            COL_NUM_FORMAT_STR, c);
    }
    if (to_console) {
        cts_info("%s", line_buf);
        cts_info(SPLIT_LINE_STR);
    }
    if (cts_test_data_filp) {
        cts_write_file(cts_test_data_filp, line_buf, count);
        cts_write_file(cts_test_data_filp, "\n", 1);
        cts_write_file(cts_test_data_filp, SPLIT_LINE_STR, strlen(SPLIT_LINE_STR));
        cts_write_file(cts_test_data_filp, "\n", 1);
    }

    for (r = 0; r < dis_rows; r++) {
        count = 0;
        count += scnprintf(line_buf + count, sizeof(line_buf) - count,
            ROW_NUM_FORMAT_STR, r);
        for (c = 0; c < dis_cols; c++) {
            count +=
                scnprintf(line_buf + count, sizeof(line_buf) - count,
                    DATA_FORMAT_STR,
                    dump_data[r * dis_cols + c]);
        }
        if (to_console) {
            cts_info("%s", line_buf);
        }
        if (cts_test_data_filp) {
            cts_write_file(cts_test_data_filp, line_buf, count);
            cts_write_file(cts_test_data_filp, "\n", 1);
        }
    }
    if (to_console) {
        cts_info(SPLIT_LINE_STR);
    }
    if (cts_test_data_filp) {
        cts_write_file(cts_test_data_filp, SPLIT_LINE_STR, strlen(SPLIT_LINE_STR));
        cts_write_file(cts_test_data_filp, "\n", 1);
    }

#undef SPLIT_LINE_STR
#undef ROW_NUM_FORMAT_STR
#undef COL_NUM_FORMAT_STR
#undef DATA_FORMAT_STR
}

static bool is_invalid_node(u32 *invalid_nodes, u32 num_invalid_nodes,
			    u16 row, u16 col)
{
	int i;

	for (i = 0; i < num_invalid_nodes; i++) {
		if (MAKE_INVALID_NODE(row, col) == invalid_nodes[i]) {
			return true;
		}
	}

	return false;
}

static int validate_tsdata(struct cts_device *cts_dev,
    const char *desc, u16 *data,
    u32 *invalid_nodes, u32 num_invalid_nodes,
    bool per_node, int *min, int *max)
{
#define SPLIT_LINE_STR \
    "------------------------------"

    int r, c;
    int failed_cnt = 0;
    int is_short_test = 0;
    u32 validate_data[CTS_MAX_DATA_NOTES] = {0};
    int rows= cts_dev->fwdata.rows; 
    int cols= cts_dev->fwdata.cols;

    if(strstr(desc,"Short") != NULL){
		rows= 1;
	    cols= cts_dev->fwdata.cols + cts_dev->fwdata.rows;;
		is_short_test = 1;
	 }
	else if(strstr(desc,"Self") != NULL){
	    rows= 1;
        cols= cts_dev->fwdata.cols + cts_dev->fwdata.rows;  
	 }
	
    cts_info("%s validate data: %s, num invalid node: %u, thresh[0]=[%d, %d]",
        desc, per_node ? "Per-Node" : "Uniform-Threshold",
        num_invalid_nodes, min ? min[0] : INT_MIN, max ? max[0] : INT_MAX);
   
	
    for (r = 0; r <rows; r++) {
        for (c = 0; c <cols; c++) {
            int offset = r * cols + c;

            if (num_invalid_nodes &&
                is_invalid_node(invalid_nodes, num_invalid_nodes, r,c)) {
                continue;
            }
			if(is_short_test){
				validate_data[offset] =  data[2*offset+1]<< 16 | data[2*offset];
				}
			else
				validate_data[offset] = data[offset];
            if ((min != NULL && validate_data[offset] < min[per_node ? offset : 0]) ||
                (max != NULL && validate_data[offset] > max[per_node ? offset : 0])) {
                if (failed_cnt == 0) {
                    cts_info(SPLIT_LINE_STR);
                    cts_info("%s failed nodes:", desc);
                }
                failed_cnt++;

                cts_info("  %3d: [%-2d][%-2d] = %u,",
                    failed_cnt, r, c, validate_data[offset]);
            }
        }
    }

	if (failed_cnt) {
		cts_info(SPLIT_LINE_STR);
		cts_info("%s test %d node total failed", desc, failed_cnt);
	}

	return failed_cnt;

#undef SPLIT_LINE_STR
}

static int prepare_test(struct cts_device *cts_dev)
{
	int ret;

	cts_info("Prepare test");

	/*cts_plat_reset_device(cts_dev->pdata);*/

	ret = disable_fw_monitor_mode(cts_dev);
	if (ret) {
		cts_err("Disable firmware monitor mode failed %d", ret);
		return ret;
	}

	ret = disable_fw_gesture_mode(cts_dev);
	if (ret) {
		cts_err("Disable firmware monitor mode failed %d", ret);
		return ret;
	}

	ret = disable_fw_low_power(cts_dev);
	if (ret) {
		cts_err("Disable firmware low power failed %d", ret);
		return ret;
	}

	cts_dev->rtdata.testing = true;

	return 0;
}

static void post_test(struct cts_device *cts_dev)
{
    int ret;

    cts_info("Post test");

    cts_plat_reset_device(cts_dev->pdata);
    cts_dev->rtdata.testing = false;
}

/* Return 0 success
    negative value while error occurs
    positive value means how many nodes fail */
int cts_test_short(struct cts_device *cts_dev,
    struct cts_test_param *param)
{
    bool driver_validate_data = false;
    bool validate_data_per_node = false;
    bool stop_if_failed = false;
    bool dump_test_data_to_user = false;
    bool dump_test_data_to_console = false;
    bool dump_test_data_to_file = false;
    int  num_nodes;
    int  tsdata_frame_size;
    int  loopcnt;
    int  ret;
	int  i;
    u32 *test_result = NULL;
	u16  dis_test_result[32] = {0};
    u8   feature_ver;
    ktime_t start_time, end_time, delta_time;
    int err_num;


    if (cts_dev == NULL || param == NULL) {
        cts_err("Short test with invalid param: cts_dev: %p test param: %p",
            cts_dev, param);
        return -EINVAL;
    }

    num_nodes = cts_dev->fwdata.rows + cts_dev->fwdata.cols;
    tsdata_frame_size = 4 * num_nodes;

    driver_validate_data =
        !!(param->flags & CTS_TEST_FLAG_VALIDATE_DATA);
    validate_data_per_node =
        !!(param->flags & CTS_TEST_FLAG_VALIDATE_PER_NODE);
    dump_test_data_to_user =
        !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE);
    dump_test_data_to_console =
        !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE);
    dump_test_data_to_file =
        !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE);
    stop_if_failed =
        !!(param->flags & CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED);

    cts_info("Short test, flags: 0x%08x,"
               "num invalid node: %u, "
               "test data file: '%s' buf size: %d, "
               "drive log file: '%s' buf size: %d",
        param->flags, param->num_invalid_node,
        param->test_data_filepath, param->test_data_buf_size,
        param->driver_log_filepath, param->driver_log_buf_size);

    start_time = ktime_get();

    if (dump_test_data_to_user) {
        test_result = (u32 *)param->test_data_buf;
    } else {
        test_result = (u32 *)kmalloc(tsdata_frame_size, GFP_KERNEL);
        if (test_result == NULL) {
            cts_err("Allocate test result buffer failed");
            ret = -ENOMEM;
            goto show_test_result;
        }
    }

    ret = cts_stop_device(cts_dev);
    if (ret) {
        cts_err("Stop device failed %d", ret);
        goto free_mem;
    }

    cts_lock_device(cts_dev);

    ret = prepare_test(cts_dev);
    if (ret) {
        cts_err("Prepare test failed %d", ret);
        goto post_test;
    }
	
    for (i = 0; i < CFG_CTS_GET_DATA_RETRY; i++) {
        int r;
        u8 val;
        r = cts_start_fw_short_opentest(cts_dev);
        if (r) {
            cts_err("Enable get shortdata failed %d", r);
            continue;
        }else{
             cts_dbg("Enable get shortdata susccess ");
             break;
        	}
    }

    if (i >= CFG_CTS_GET_DATA_RETRY) {
        cts_err("Enable read shortdata failed");
        ret = -EIO;
        goto post_test;
    }
	
    if (dump_test_data_to_user) {
        *param->test_data_wr_size += tsdata_frame_size;
    }


     for (i = 0; i < CFG_CTS_GET_DATA_RETRY; i++) {
           int r = cts_get_shortdata(cts_dev, test_result);
            if (r) {
                cts_err("Get short data failed %d", r);
                mdelay(30);
            } else {
               cts_dbg(" get shortdata susccess ");
                break;
             }
     	}
        if (dump_test_data_to_user) {
            *param->test_data_wr_size += tsdata_frame_size;
        }
		
        memcpy(dis_test_result, test_result, tsdata_frame_size);
		
        if (dump_test_data_to_console || dump_test_data_to_file) {
            cts_dump_tsdata(cts_dev, "Short data", dis_test_result,
                dump_test_data_to_console);
        }

        if (driver_validate_data) {
            ret = validate_tsdata(cts_dev,
                "Short data", dis_test_result,
                param->invalid_nodes, param->num_invalid_node,
                validate_data_per_node, param->min, param->max);
           for(i=0;i<num_nodes;i++){
		   	  cts_err("Shortdata test,node %d", i);
              if(test_result[i]< *param->min)
			  	cts_err("Shortdata test failed %d", i);
		   }
        }


	    if (dump_test_data_to_file) {
	        int r = cts_start_dump_test_data_to_file(param->test_data_filepath,
	            !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE_APPEND));
	        if (r) {
	            cts_err("Start short test data to file failed %d", r);
	        }
	    }

	    if (dump_test_data_to_user) {
	        test_result += num_nodes;
	    }

		if (dump_test_data_to_file) {
        cts_stop_dump_test_data_to_file();
        }
	
post_test:
    post_test(cts_dev);

unlock_device:
    cts_unlock_device(cts_dev);

    cts_start_device(cts_dev);

free_mem:
    if (!dump_test_data_to_user && test_result) {
        kfree(test_result);
    }

show_test_result:
    end_time = ktime_get();
    delta_time = ktime_sub(end_time, start_time);
    if (ret > 0) {
        cts_info("Short test has %d nodes FAIL, ELAPSED TIME: %lldms",
            ret, ktime_to_ms(delta_time));
    } else if (ret < 0) {
        cts_info("Short test FAIL %d(%s), ELAPSED TIME: %lldms",
            ret, cts_strerror(ret), ktime_to_ms(delta_time));
    } else {
        cts_info("Short test PASS, ELAPSED TIME: %lldms",
            ktime_to_ms(delta_time));
    }


    return ret;
}

int cts_test_open(struct cts_device *cts_dev,
			  struct cts_test_param *param)
{
	bool driver_validate_data = false;
	bool validate_data_per_node = false;
	bool stop_if_failed = false;
	bool dump_test_data_to_user = false;
	bool dump_test_data_to_console = false;
	bool dump_test_data_to_file = false;
	int num_nodes;
	int tsdata_frame_size;
	int loopcnt;
	int ret;
	int i;
	u16 *test_result = NULL;
	u8 feature_ver;
	ktime_t start_time, end_time, delta_time;
	int err_num;

	if (cts_dev == NULL || param == NULL) {
		cts_err
		    ("open drive test with invalid param: cts_dev: %p test param: %p",
		     cts_dev, param);
		return -EINVAL;
	}

	num_nodes = cts_dev->fwdata.rows * cts_dev->fwdata.cols;
	tsdata_frame_size = 2 * num_nodes;

	driver_validate_data = !!(param->flags & CTS_TEST_FLAG_VALIDATE_DATA);
	validate_data_per_node =
	    !!(param->flags & CTS_TEST_FLAG_VALIDATE_PER_NODE);
	dump_test_data_to_user =
	    !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE);
	dump_test_data_to_console =
	    !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE);
	dump_test_data_to_file =
	    !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE);
	stop_if_failed =
	    !!(param->flags & CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED);

	cts_info("Open drive test, flags: 0x%08x,"
		    "num invalid node: %u, "
		    "test data file: '%s' buf size: %d, "
		    "drive log file: '%s' buf size: %d",
		    param->flags, param->num_invalid_node,
		    param->test_data_filepath, param->test_data_buf_size,
		    param->driver_log_filepath, param->driver_log_buf_size);

	start_time = ktime_get();

	if (dump_test_data_to_user) {
		test_result = (u16 *) param->test_data_buf;
	} else {
		test_result = (u16 *) kmalloc(tsdata_frame_size, GFP_KERNEL);
		if (test_result == NULL) {
			cts_err("Allocate test result buffer failed");
			ret = -ENOMEM;
			goto show_test_result;
		}
	}

	ret = cts_stop_device(cts_dev);
	if (ret) {
		cts_err("Stop device failed %d", ret);
		goto free_mem;
	}

	cts_lock_device(cts_dev);

	ret = prepare_test(cts_dev);
	if (ret) {
		cts_err("Prepare test failed %d", ret);
		goto post_test;
	}

	for (i = 0; i < CFG_CTS_GET_DATA_RETRY; i++) {
		int r;
		u8 val;
		r = cts_start_fw_short_opentest(cts_dev);
		if (r) {
			cts_err("Enable short and open test failed %d", r);
			continue;
		} else {
			cts_dbg("Enable short and open test susccess ");
			break;
		}
	}

	if (i >= CFG_CTS_GET_DATA_RETRY) {
		cts_err("Enable short and open test failed");
		ret = -EIO;
		goto post_test;
	}

	if (dump_test_data_to_user)
		*param->test_data_wr_size += tsdata_frame_size;

	for (i = 0; i < CFG_CTS_GET_DATA_RETRY; i++) {
		int r = cts_get_opendata(cts_dev, test_result);
		if (r) {
			cts_err("Get open data failed %d", r);
			mdelay(30);
		} else {
			cts_dbg(" get open susccess ");
			break;
		}
	}

	if (dump_test_data_to_user)
		*param->test_data_wr_size += tsdata_frame_size;

	if (dump_test_data_to_console || dump_test_data_to_file) {
		cts_dump_tsdata(cts_dev, "open data", test_result,
				dump_test_data_to_console);
	}

	if (driver_validate_data) {
		ret = validate_tsdata(cts_dev,
				      "open data", test_result,
				      param->invalid_nodes,
				      param->num_invalid_node,
				      validate_data_per_node, param->min,
				      param->max);
		if (ret)
			cts_err("open data test failed %d", ret);
	}

	if (dump_test_data_to_file) {
		int r =
		    cts_start_dump_test_data_to_file(param->test_data_filepath,
						     !!(param->flags &
							 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE_APPEND));
		if (r) {
			cts_err
			    ("Start open data test data to file failed %d",
			     r);
		}
	}

	if (dump_test_data_to_user)
		test_result += num_nodes;

	if (dump_test_data_to_file)
		cts_stop_dump_test_data_to_file();

 post_test:
	post_test(cts_dev);

 unlock_device:
	cts_unlock_device(cts_dev);

	cts_start_device(cts_dev);

 free_mem:
	if (!dump_test_data_to_user && test_result) {
		kfree(test_result);
	}

 show_test_result:
	end_time = ktime_get();
	delta_time = ktime_sub(end_time, start_time);
	if (ret > 0) {
		cts_info("Short test has %d nodes FAIL, ELAPSED TIME: %lldms",
			 ret, ktime_to_ms(delta_time));
	} else if (ret < 0) {
		cts_info("Short test FAIL %d(%s), ELAPSED TIME: %lldms",
			 ret, cts_strerror(ret), ktime_to_ms(delta_time));
	} else {
		cts_info("Short test PASS, ELAPSED TIME: %lldms",
			 ktime_to_ms(delta_time));
	}

	return ret;
}

int cts_test_reset_pin(struct cts_device *cts_dev, struct cts_test_param *param)
{
	ktime_t start_time, end_time, delta_time;
	int ret;

	if (cts_dev == NULL || param == NULL) {
		cts_err
		    ("Reset-pin test with invalid param: cts_dev: %p test param: %p",
		     cts_dev, param);
		return -EINVAL;
	}

	cts_info("Reset-Pin test, flags: 0x%08x, "
		 "drive log file: '%s' buf size: %d",
		 param->flags,
		 param->driver_log_filepath, param->driver_log_buf_size);

	start_time = ktime_get();

	ret = cts_stop_device(cts_dev);
	if (ret) {
		cts_err("Stop device failed %d", ret);
		goto show_test_result;
	}

    cts_lock_device(cts_dev);

    //cts_plat_set_reset(cts_dev->pdata, 0);
    gpio_set_value(cts_dev->pdata->rst_gpio, 0);
    mdelay(50);

    /* Check whether device is in normal mode */
	if (cts_plat_is_i2c_online(cts_dev->pdata, CTS_DEV_NORMAL_MODE_I2CADDR)) {
		ret = -EIO;
		cts_err("Device is alive while reset is low");
    }
    //cts_plat_set_reset(cts_dev->pdata, 1);
    gpio_set_value(cts_dev->pdata->rst_gpio, 1);
    mdelay(50);

    /* Check whether device is in normal mode */
	if (!cts_plat_is_i2c_online(cts_dev->pdata,
				    CTS_DEV_NORMAL_MODE_I2CADDR)) {
		ret = -EIO;
		cts_err("Device is offline while reset is high");
    }

    cts_unlock_device(cts_dev);

	{
		int r = cts_start_device(cts_dev);
		if (r) {
			cts_err("Start device failed %d", r);
		}
	}

	if (!cts_dev->rtdata.program_mode) {
		cts_set_normal_addr(cts_dev);
	}

 show_test_result:
	end_time = ktime_get();
	delta_time = ktime_sub(end_time, start_time);
	if (ret) {
		cts_info("Reset-Pin test FAIL %d(%s), ELAPSED TIME: %lldms",
			 ret, cts_strerror(ret), ktime_to_ms(delta_time));
	} else {
		cts_info("Reset-Pin test PASS, ELAPSED TIME: %lldms",
			 ktime_to_ms(delta_time));
	}

	return ret;
}

int cts_test_int_pin(struct cts_device *cts_dev, struct cts_test_param *param)
{
	ktime_t start_time, end_time, delta_time;
	int ret;

	if (cts_dev == NULL || param == NULL) {
		cts_err
		    ("Int-pin test with invalid param: cts_dev: %p test param: %p",
		     cts_dev, param);
		return -EINVAL;
	}

	cts_info("Int-Pin test, flags: 0x%08x, "
		 "drive log file: '%s' buf size: %d",
		 param->flags,
		 param->driver_log_filepath, param->driver_log_buf_size);

	start_time = ktime_get();

	ret = cts_stop_device(cts_dev);
	if (ret) {
		cts_err("Stop device failed %d", ret);
		goto show_test_result;
	}

	cts_lock_device(cts_dev);

	ret = cts_send_command(cts_dev, CTS_CMD_WRTITE_INT_HIGH);
	if (ret) {
		cts_err("Send command WRTITE_INT_HIGH failed %d", ret);
		goto unlock_device;
	}
	mdelay(10);
#if 0
    if (cts_plat_get_int_pin(cts_dev) == 0) {
        cts_err("INT pin state != HIGH");
        ret = -EFAULT;
        goto exit_int_test;
    }
#endif
	ret = cts_send_command(cts_dev, CTS_CMD_WRTITE_INT_LOW);
	if (ret) {
		cts_err("Send command WRTITE_INT_LOW failed %d", ret);
		goto exit_int_test;
	}
	mdelay(10);
#if 0
    if (cts_plat_get_int_pin(cts_dev) != 0) {
        cts_err("INT pin state != LOW");
        ret = -EFAULT;
        goto exit_int_test;
    }
#endif
 exit_int_test:
	{
		int r = cts_send_command(cts_dev, CTS_CMD_RELASE_INT_TEST);
		if (r) {
			cts_err("Send command RELASE_INT_TEST failed %d", r);
		}
	}
	mdelay(10);

 unlock_device:
	cts_unlock_device(cts_dev);

	{
		int r = cts_start_device(cts_dev);
		if (r) {
			cts_err("Start device failed %d", r);
		}
	}

 show_test_result:
	end_time = ktime_get();
	delta_time = ktime_sub(end_time, start_time);
	if (ret) {
		cts_info("Int-Pin test FAIL %d(%s), ELAPSED TIME: %lldms",
			 ret, cts_strerror(ret), ktime_to_ms(delta_time));
	} else {
		cts_info("Int-Pin test PASS, ELAPSED TIME: %lldms",
			 ktime_to_ms(delta_time));
	}

	return ret;
}

static int cts_deviation_calc(u16 *data, u8 rows, u8 cols,u16 *result)
{
	u32 max_val = 0, rawdata_val;
	u16 data_up = 0, data_down = 0, data_left = 0, data_right = 0;
	int rawdata_offset;
	u32 notes;
	int i;

	notes = rows * cols;

    if (cols == 0 || notes == 0)
			return -EINVAL;

	for (i = 0; i < notes; i++) {
		if(data[i] == 0){
			cts_err("%s:%d rawdata error,data[%d] = %d,NG\n",
				__func__,__LINE__,i,data[i]);
			return -EINVAL;
		}
	}

	for (i = 0; i < notes; i++) {
		rawdata_val = data[i];
		max_val = 0;
		if (i -cols>= 0) {
			rawdata_offset = i - cols;
			data_up =data[rawdata_offset];
			data_up = abs(rawdata_val - data_up);
			if (data_up > max_val)
				max_val = data_up;
		}

		if (i + cols < notes) {
			rawdata_offset = i + cols;
			data_down = data[rawdata_offset];
			data_down = abs(rawdata_val - data_down);
			if (data_down > max_val)
				max_val = data_down;
		}

		if (i % cols) {
			rawdata_offset = i - 1;
			data_left = data[rawdata_offset];
			data_left = abs(rawdata_val - data_left);
			if (data_left > max_val)
				max_val = data_left;
		}

		if ((i + 1) % cols) {
			rawdata_offset = i + 1;
			data_right = data[rawdata_offset];
			data_right = abs(rawdata_val - data_right);
			if (data_right > max_val)
				max_val = data_right;
		}
		result[i] = 1000* max_val / rawdata_val;
	}
	return 0;
}

int cts_test_deviation(struct cts_device *cts_dev, struct cts_test_param *param)

{
	struct cts_deviation_test_priv_param *priv_param;
	bool driver_validate_data = false;
	bool validate_data_per_node = false;
	bool stop_test_if_validate_fail = false;
	bool dump_test_data_to_user = false;
	bool dump_test_data_to_console = false;
	bool dump_test_data_to_file = false;
	int num_nodes;
	int mul_failed_nodes, self_failed_nodes;
	int mutual_nodes, self_nodes;
	int tsdata_frame_size;
	int frame;
	u16 *deviation = NULL;
	u16 deviation_raw[CTS_MAX_DATA_NOTES] = {0};
	u16 *mul_deviation = NULL;
	u16 *self_deviation = NULL;
	ktime_t start_time, end_time, delta_time;
	int i;
	int ret;

	if (cts_dev == NULL || param == NULL ||
	    param->priv_param_size != sizeof(*priv_param) ||
	    param->priv_param == NULL) {
		cts_err
		    ("Rawdata test with invalid param: priv param: %p size: %d",
		     param->priv_param, param->priv_param_size);
		return -EINVAL;
	}

	priv_param = param->priv_param;
	if (priv_param->frames <= 0) {
		cts_info("Rawdata test with too little frame %u",
			 priv_param->frames);
		return -EINVAL;
	}
    mutual_nodes = cts_dev->fwdata.rows * cts_dev->fwdata.cols;
	self_nodes = cts_dev->fwdata.rows + cts_dev->fwdata.cols;
	num_nodes = mutual_nodes + self_nodes;
	tsdata_frame_size = 2 * num_nodes;

	driver_validate_data = !!(param->flags & CTS_TEST_FLAG_VALIDATE_DATA);
	validate_data_per_node =
	    !!(param->flags & CTS_TEST_FLAG_VALIDATE_PER_NODE);
	dump_test_data_to_user =
	    !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE);
	dump_test_data_to_console =
	    !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE);
	dump_test_data_to_file =
	    !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE);
	stop_test_if_validate_fail =
	    !!(param->flags & CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED);

	cts_info("Deviation test, flags: 0x%08x, frames: %d, "
		 "num invalid node: %u, "
		 "test data file: '%s' buf size: %d, "
		 "drive log file: '%s' buf size: %d",
		 param->flags, priv_param->frames, param->num_invalid_node,
		 param->test_data_filepath, param->test_data_buf_size,
		 param->driver_log_filepath, param->driver_log_buf_size);

	start_time = ktime_get();

	if (dump_test_data_to_user) {
		deviation = (u16 *)param->test_data_buf;
	} else {
		deviation = (u16 *)kmalloc(tsdata_frame_size, GFP_KERNEL);
		if (deviation == NULL) {
			cts_err("Allocate memory for Deviation failed");
			ret = -ENOMEM;
			goto show_test_result;
		}
	}

	mul_deviation = deviation;
	self_deviation = deviation + 2 * mutual_nodes;

	/* Stop device to avoid un-wanted interrrupt */
	ret = cts_stop_device(cts_dev);
	if (ret) {
		cts_err("Stop device failed %d", ret);
		goto free_mem;
	}

	cts_lock_device(cts_dev);

	ret = prepare_test(cts_dev);
	if (ret) {
		cts_err("Prepare test failed %d", ret);
		goto post_test;
	}

	for (i = 0; i < CFG_CTS_GET_DATA_RETRY; i++) {
		int r;
		u8 val;
		r = cts_enable_get_rawdata(cts_dev);
		if (r) {
			cts_err("Enable get rawdata failed %d", r);
			continue;
		} else {
			break;
		}
		mdelay(CFG_CTS_GET_FLAG_DELAY);
	}

	if (i >= CFG_CTS_GET_DATA_RETRY) {
		cts_err("Enable read jitter failed");
		ret = -EIO;
		goto post_test;
	}

	if (dump_test_data_to_file) {
		int r =
		    cts_start_dump_test_data_to_file(param->test_data_filepath,
						     !!(param->flags &
							 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE_APPEND));
		if (r)
			cts_err("Start dump test data to file failed %d", r);
	}

	for (frame = 0; frame < priv_param->frames; frame++) {
		bool data_valid = false;

		for (i = 0; i < CFG_CTS_GET_DATA_RETRY; i++) {
			int r = cts_get_rawdata(cts_dev, deviation_raw);
			if (r) {
				cts_err("Get rawdata failed %d", r);
				mdelay(30);
			} else {
				data_valid = true;
				break;
			}
		}

		if (!data_valid) {
			ret = -EIO;
			break;
		}

		if (dump_test_data_to_user)
			*param->test_data_wr_size += tsdata_frame_size;
		
        ret = cts_deviation_calc(deviation_raw, cts_dev->fwdata.rows, 
							cts_dev->fwdata.cols, mul_deviation);
        if (ret){
			  cts_err("calc mutual deviation failed %d", ret);
        	}
		
		ret = cts_deviation_calc(&deviation_raw[mutual_nodes], 2, 
							cts_dev->fwdata.cols, self_deviation);
        if (ret){
			  cts_err("calc self deviation failed %d", ret);
        	}

		if (dump_test_data_to_console || dump_test_data_to_file) {
			cts_dump_tsdata(cts_dev, "Mutual_deviation", mul_deviation,
					dump_test_data_to_console);
			cts_dump_tsdata(cts_dev, "Self_deviation", self_deviation,
					dump_test_data_to_console);
		}

		if (driver_validate_data) {
			mul_failed_nodes = validate_tsdata(cts_dev,
							   "Mutual_deviation", mul_deviation,
							   param->invalid_nodes,
							   param->num_invalid_node,
							   validate_data_per_node,
							   NULL,
							   param->max);
			if (mul_failed_nodes) {
				cts_err("Mutual deviation test failed %d", ret);
				if (stop_test_if_validate_fail) {
					break;
				}
			}
			self_failed_nodes = validate_tsdata(cts_dev,
							    "Self_deviation", self_deviation,
							    param->invalid_nodes, 0,
							    validate_data_per_node,
							    NULL,
							    param->self_max);
			if (self_failed_nodes) {
				cts_err("self deviation test failed %d", ret);
				if (stop_test_if_validate_fail)
					break;
			}
		}
		
		if (dump_test_data_to_user)
			deviation += num_nodes;  
	}
    ret = mul_failed_nodes + self_failed_nodes;
	
	if (dump_test_data_to_file)
		cts_stop_dump_test_data_to_file();

	for (i = 0; i < CFG_CTS_GET_DATA_RETRY; i++) {
		int r = cts_disable_get_rawdata(cts_dev);
		if (r) {
			cts_err("Disable get rawdata failed %d", r);
			continue;
		} else {
			break;
		}
	}

 post_test:
	post_test(cts_dev);
	cts_unlock_device(cts_dev);

	{
		int r = cts_start_device(cts_dev);
		if (r)
			cts_err("Start device failed %d", r);
	}

 free_mem:
	if (!dump_test_data_to_user && deviation != NULL)
		kfree(deviation);

 show_test_result:
	end_time = ktime_get();
	delta_time = ktime_sub(end_time, start_time);
	if (ret > 0) {
		cts_info("Deviation test has %d nodes FAIL, ELAPSED TIME: %lldms",
			 ret, ktime_to_ms(delta_time));
	} else if (ret < 0) {
		cts_info("Deviation test FAIL %d(%s), ELAPSED TIME: %lldms",
			 ret, cts_strerror(ret), ktime_to_ms(delta_time));
	} else {
		cts_info("Deviation test PASS, ELAPSED TIME: %lldms",
			 ktime_to_ms(delta_time));
	}

	return ret;
}

int cts_test_rawdata(struct cts_device *cts_dev, struct cts_test_param *param)
{
	struct cts_rawdata_test_priv_param *priv_param;
	bool driver_validate_data = false;
	bool validate_data_per_node = false;
	bool stop_test_if_validate_fail = false;
	bool dump_test_data_to_user = false;
	bool dump_test_data_to_console = false;
	bool dump_test_data_to_file = false;
	int num_nodes;
	int mul_failed_nodes, self_failed_nodes;
	int mutual_nodes, self_nodes;
	int tsdata_frame_size;
	int frame;
	u16 *rawdata = NULL;
	u16 *mul_rawdata = NULL;
	u16 *self_rawdata = NULL;
	ktime_t start_time, end_time, delta_time;
	int i;
	int ret;

	if (cts_dev == NULL || param == NULL ||
	    param->priv_param_size != sizeof(*priv_param) ||
	    param->priv_param == NULL) {
		cts_err
		    ("Rawdata test with invalid param: priv param: %p size: %d",
		     param->priv_param, param->priv_param_size);
		return -EINVAL;
	}

	priv_param = param->priv_param;
	if (priv_param->frames <= 0) {
		cts_info("Rawdata test with too little frame %u",
			 priv_param->frames);
		return -EINVAL;
	}
    mutual_nodes = cts_dev->fwdata.rows * cts_dev->fwdata.cols;
	self_nodes = cts_dev->fwdata.rows + cts_dev->fwdata.cols;
	num_nodes = mutual_nodes + self_nodes;
	tsdata_frame_size = 2 * num_nodes;

	driver_validate_data = !!(param->flags & CTS_TEST_FLAG_VALIDATE_DATA);
	validate_data_per_node =
	    !!(param->flags & CTS_TEST_FLAG_VALIDATE_PER_NODE);
	dump_test_data_to_user =
	    !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE);
	dump_test_data_to_console =
	    !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE);
	dump_test_data_to_file =
	    !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE);
	stop_test_if_validate_fail =
	    !!(param->flags & CTS_TEST_FLAG_STOP_TEST_IF_VALIDATE_FAILED);

	cts_info("Rawdata test, flags: 0x%08x, frames: %d, "
		 "num invalid node: %u, "
		 "test data file: '%s' buf size: %d, "
		 "drive log file: '%s' buf size: %d",
		 param->flags, priv_param->frames, param->num_invalid_node,
		 param->test_data_filepath, param->test_data_buf_size,
		 param->driver_log_filepath, param->driver_log_buf_size);

	start_time = ktime_get();

	if (dump_test_data_to_user) {
		rawdata = (u16 *)param->test_data_buf;
	} else {
		rawdata = (u16 *)kmalloc(tsdata_frame_size, GFP_KERNEL);
		if (rawdata == NULL) {
			cts_err("Allocate memory for rawdata failed");
			ret = -ENOMEM;
			goto show_test_result;
		}
	}

	mul_rawdata = rawdata;
	self_rawdata = rawdata + 2 * mutual_nodes;  /*per note 2bytes*/
		
    /* Stop device to avoid un-wanted interrrupt */
    ret = cts_stop_device(cts_dev);
    if (ret) {
        cts_err("Stop device failed %d", ret);
        goto free_mem;
    }

    cts_lock_device(cts_dev);

	ret = prepare_test(cts_dev);
    if (ret) {
        cts_err("Prepare test failed %d", ret);
        goto post_test;
    }

    for (i = 0; i < CFG_CTS_GET_DATA_RETRY; i++) {
        int r;
        r = cts_enable_get_rawdata(cts_dev);
        if (r) {
            cts_err("Enable get tsdata failed %d", r);
			continue;
		} else {
			break;
		}
		mdelay(1);
	}

	if (i >= CFG_CTS_GET_DATA_RETRY) {
		cts_err("Enable read tsdata failed");
		ret = -EIO;
		goto post_test;
	}

	if (dump_test_data_to_file) {
		int r =
		    cts_start_dump_test_data_to_file(param->test_data_filepath,
						     !!(param->flags &
							 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE_APPEND));
		if (r)
			cts_err("Start dump test data to file failed %d", r);
	}

	for (frame = 0; frame < priv_param->frames; frame++) {
		bool data_valid = false;

		for (i = 0; i < CFG_CTS_GET_DATA_RETRY; i++) {
			int r = cts_get_rawdata(cts_dev, rawdata);
			if (r) {
				cts_err("Get rawdata failed %d", r);
				mdelay(30);
			} else {
				data_valid = true;
				break;
			}
		}

		if (!data_valid) {
			ret = -EIO;
			break;
		}

		if (dump_test_data_to_user)
			*param->test_data_wr_size += tsdata_frame_size;

		//memcpy(self_rawdata, rawdata + mutual_nodes, self_nodes * 2);

		if (dump_test_data_to_console || dump_test_data_to_file) {
			cts_dump_tsdata(cts_dev, "Rawdata", rawdata,
					dump_test_data_to_console);
			cts_dump_tsdata(cts_dev, "Selfdata", self_rawdata,
					dump_test_data_to_console);
		}

		if (driver_validate_data) {
			mul_failed_nodes = validate_tsdata(cts_dev,
							   "Rawdata", rawdata,
							   param->invalid_nodes,
							   param->num_invalid_node,
							   validate_data_per_node,
							   param->min,
							   param->max);
			if (mul_failed_nodes) {
				cts_err("Rawdata test failed:%d", mul_failed_nodes);
				if (stop_test_if_validate_fail) {
					break;
				}
			}
			self_failed_nodes = validate_tsdata(cts_dev,
							    "Selfdata", self_rawdata,
							    param->invalid_nodes, 0,
							    validate_data_per_node,
							    param->self_min,
							    param->self_max);
			if (self_failed_nodes) {
				cts_err("self Rawdata test failed:%d", mul_failed_nodes);
				if (stop_test_if_validate_fail)
					break;
			}
		}
	}
    ret = mul_failed_nodes + self_failed_nodes;

	if (dump_test_data_to_user)
		 rawdata += num_nodes;

	if (dump_test_data_to_file)
		cts_stop_dump_test_data_to_file();

	for (i = 0; i < CFG_CTS_GET_DATA_RETRY; i++) {
		int r = cts_disable_get_rawdata(cts_dev);
		if (r) {
			cts_err("Disable get rawdata failed %d", r);
			continue;
		} else {
			break;
        }
	}

 post_test:
	post_test(cts_dev);
	cts_unlock_device(cts_dev);

	{
		int r = cts_start_device(cts_dev);
		if (r)
			cts_err("Start device failed %d", r);
	}

 free_mem:
	if (!dump_test_data_to_user && rawdata != NULL)
		kfree(rawdata);

 show_test_result:
	end_time = ktime_get();
	delta_time = ktime_sub(end_time, start_time);
	if (ret > 0) {
		cts_info("Rawdata test has %d nodes FAIL, ELAPSED TIME: %lldms",
			 ret, ktime_to_ms(delta_time));
	} else if (ret < 0) {
		cts_info("Rawdata test FAIL %d(%s), ELAPSED TIME: %lldms",
			 ret, cts_strerror(ret), ktime_to_ms(delta_time));
	} else {
		cts_info("Rawdata test PASS, ELAPSED TIME: %lldms",
			 ktime_to_ms(delta_time));
	}

	return ret;
}

int cts_test_noise(struct cts_device *cts_dev, struct cts_test_param *param)
{
	struct cts_noise_test_priv_param *priv_param;
	bool driver_validate_data = false;
	bool validate_data_per_node = false;
	bool dump_test_data_to_user = false;
	bool dump_test_data_to_console = false;
	bool dump_test_data_to_file = false;
	int num_nodes;
	int mul_failed_nodes, self_failed_nodes;
	int mutual_nodes, self_nodes;
	int tsdata_frame_size;
	int frame;
	u16 *buffer = NULL;
	int buf_size = 0;
	u16 *curr_rawdata = NULL;
	u16 *max_rawdata = NULL;
	u16 *min_rawdata = NULL;
	u16 *noise = NULL;
	bool first_frame = true;
	bool data_valid = false;
	u16 *mul_noise = NULL;
	u16 *self_noise = NULL;
	ktime_t start_time, end_time, delta_time;
	int i;
	int ret;

	if (cts_dev == NULL || param == NULL ||
	    param->priv_param_size != sizeof(*priv_param) ||
	    param->priv_param == NULL) {
		cts_err
		    ("Noise test with invalid param: priv param: %p size: %d",
		     param->priv_param, param->priv_param_size);
		return -EINVAL;
	}

	priv_param = param->priv_param;
	if (priv_param->frames < 2) {
		cts_err("Noise test with too little frame %u",
			priv_param->frames);
		return -EINVAL;
	}
    mutual_nodes = cts_dev->fwdata.rows * cts_dev->fwdata.cols;
	self_nodes = cts_dev->fwdata.rows + cts_dev->fwdata.cols;
	num_nodes = mutual_nodes + self_nodes;
	tsdata_frame_size = 2 * num_nodes;

	driver_validate_data = !!(param->flags & CTS_TEST_FLAG_VALIDATE_DATA);
	validate_data_per_node =
	    !!(param->flags & CTS_TEST_FLAG_VALIDATE_PER_NODE);
	dump_test_data_to_user =
	    !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_USERSPACE);
	dump_test_data_to_console =
	    !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_CONSOLE);
	dump_test_data_to_file =
	    !!(param->flags & CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE);

	cts_info("Noise test, flags: 0x%08x, frames: %d, "
		 "num invalid node: %u, "
		 "test data file: '%s' buf size: %d, "
		 "drive log file: '%s' buf size: %d",
		 param->flags, priv_param->frames, param->num_invalid_node,
		 param->test_data_filepath, param->test_data_buf_size,
		 param->driver_log_filepath, param->driver_log_buf_size);

	start_time = ktime_get();

	buf_size = (driver_validate_data ? 4 : 1) * tsdata_frame_size;
	buffer = (u16 *)kmalloc(buf_size, GFP_KERNEL);
	if (buffer == NULL) {
		cts_err("Alloc mem for touch data failed");
		ret = -ENOMEM;
		goto show_test_result;
	}

	curr_rawdata = buffer;
	if (driver_validate_data) {
		cts_info("driver_validate_data:%d", driver_validate_data);
		max_rawdata = curr_rawdata + 1 * num_nodes;
		min_rawdata = curr_rawdata + 2 * num_nodes;
		noise = curr_rawdata + 3 * num_nodes;
	}

	/* Stop device to avoid un-wanted interrrupt */
	ret = cts_stop_device(cts_dev);
	if (ret) {
		cts_err("Stop device failed %d", ret);
		goto free_mem;
	}

	cts_lock_device(cts_dev);

	ret = prepare_test(cts_dev);
	if (ret) {
		cts_err("Prepare test failed %d", ret);
		goto post_test;
	}

    for (i = 0; i < CFG_CTS_GET_DATA_RETRY; i++) {
        int r;
        u8 val;
        r = cts_enable_get_rawdata(cts_dev);
        if (r) {
            cts_err("Enable get tsdata failed %d", r);
            continue;
        }else{
              break;
        	}
    }


    if (i >= CFG_CTS_GET_DATA_RETRY) {
        cts_err("Enable read tsdata failed");
        ret = -EIO;
        goto unlock_device;
    }

	if (dump_test_data_to_file) {
		int r =
		    cts_start_dump_test_data_to_file(param->test_data_filepath,
						     !!(param->flags &
							 CTS_TEST_FLAG_DUMP_TEST_DATA_TO_FILE_APPEND));
		if (r) {
			cts_err("Start dump test data to file failed %d", r);
		}
	}

	msleep(50);

	for (frame = 0; frame < priv_param->frames; frame++) {
		int r;

        for (i = 0; i < CFG_CTS_GET_DATA_RETRY; i++) {
            r = cts_get_rawdata(cts_dev, curr_rawdata);
            if (r) {
                cts_err("Get rawdata failed %d", r);
                mdelay(30);
            } else {
                break;
            }
        }

        if (i >= CFG_CTS_GET_DATA_RETRY) {
            cts_err("Read rawdata failed");
            ret = -EIO;
            goto disable_get_tsdata;
        }

        if (dump_test_data_to_console || dump_test_data_to_file) {
            cts_dump_tsdata(cts_dev, "Noise-Rawdata", curr_rawdata,
                dump_test_data_to_console);
			cts_dump_tsdata(cts_dev, "Self_Noise-data", &curr_rawdata[mutual_nodes],
					dump_test_data_to_console);
        }

		if (dump_test_data_to_user) {
			memcpy(param->test_data_buf + frame * tsdata_frame_size,
			       curr_rawdata, tsdata_frame_size);
			*param->test_data_wr_size += tsdata_frame_size;
		}

		if (driver_validate_data) {
			if (unlikely(first_frame)) {
				memcpy(max_rawdata, curr_rawdata,
				       tsdata_frame_size);
				memcpy(min_rawdata, curr_rawdata,
				       tsdata_frame_size);
				first_frame = false;
			} else {
				for (i = 0; i < num_nodes; i++) {
					if (curr_rawdata[i] > max_rawdata[i]) {
						max_rawdata[i] =
						    curr_rawdata[i];
					} else if (curr_rawdata[i] <
						   min_rawdata[i]) {
						min_rawdata[i] =
						    curr_rawdata[i];
					}
				}
			}
		}
	}

	data_valid = true;

disable_get_tsdata:
    for (i = 0; i < CFG_CTS_GET_DATA_RETRY; i++) {
        int r = cts_disable_get_rawdata(cts_dev);
        if (r) {
            cts_err("Disable get rawdata failed %d", r);
            continue;
        } else {
            break;
        }
	}
	cts_dbg("Start get noise data,frame = %d", frame);
	
	for (i = 0; i < num_nodes; i++) {
			noise[i] = max_rawdata[i] - min_rawdata[i];
		}
		
		cts_dbg("Start copy noise data,frame = %d", frame);
    if (dump_test_data_to_user) {
		memcpy(param->test_data_buf + (frame + 0) * tsdata_frame_size,
			noise, tsdata_frame_size);
		cts_dbg("Start copy max raw data,frame = %d", frame);
		memcpy(param->test_data_buf + (frame + 1) * tsdata_frame_size,
			 max_rawdata, tsdata_frame_size);
	    cts_dbg("Start copy min raw data,frame = %d", frame);
		memcpy(param->test_data_buf + (frame + 2) * tsdata_frame_size,
		min_rawdata, tsdata_frame_size);
        *param->test_data_wr_size += tsdata_frame_size*3;
		}
	if (driver_validate_data && data_valid) {
		

		if (dump_test_data_to_console || dump_test_data_to_file) {
			cts_dump_tsdata(cts_dev, "Noise", noise,
				dump_test_data_to_console);
			cts_dump_tsdata(cts_dev, "Self_Noise", &noise[mutual_nodes],
				dump_test_data_to_console);
			cts_dump_tsdata(cts_dev, "Rawdata MAX", max_rawdata,
				dump_test_data_to_console);
			cts_dump_tsdata(cts_dev, "Self_Rawdata MAX", &max_rawdata[mutual_nodes],
				dump_test_data_to_console);
			cts_dump_tsdata(cts_dev, "Rawdata MIN", min_rawdata,
				dump_test_data_to_console);
			cts_dump_tsdata(cts_dev, "Self_Rawdata MIN", &min_rawdata[mutual_nodes],
				dump_test_data_to_console);
			}

      
		cts_dbg("Start validate noise data");
        cts_dbg("Noise threthod:%d,%d",param->max_limits[0],param->max_limits[mutual_nodes+0]);
			mul_failed_nodes = validate_tsdata(cts_dev,
							   "Noise test", noise,
							   param->invalid_nodes,
							   param->num_invalid_node,
							   validate_data_per_node,
							   NULL,
							   param->max);
			if (mul_failed_nodes)
				cts_err("Noise test failed:%d", mul_failed_nodes);
			self_failed_nodes = validate_tsdata(cts_dev,
							    "Self_Noise test", &noise[mutual_nodes],
							    param->invalid_nodes, 0,
							    validate_data_per_node,
							    NULL,
							    &param->max[mutual_nodes]);
			if (self_failed_nodes)
				cts_err("self noise test failed:%d", mul_failed_nodes);
	}

	if (dump_test_data_to_file)
		cts_stop_dump_test_data_to_file();
 post_test:
	post_test(cts_dev);
 unlock_device:
	cts_unlock_device(cts_dev);

	{
		int r = cts_start_device(cts_dev);
		if (r)
			cts_err("Start device failed %d", r);
	}

 free_mem:
	if (buffer != NULL)
		kfree(buffer);

 show_test_result:
	end_time = ktime_get();
	delta_time = ktime_sub(end_time, start_time);
	if (ret > 0) {
		cts_info("Noise test has %d nodes FAIL, ELAPSED TIME: %lldms",
			 ret, ktime_to_ms(delta_time));
	} else if (ret < 0) {
		cts_info("Noise test FAIL %d(%s), ELAPSED TIME: %lldms",
			 ret, cts_strerror(ret), ktime_to_ms(delta_time));
	} else {
		cts_info("Noise test PASS, ELAPSED TIME: %lldms",
			 ktime_to_ms(delta_time));
    }

    return ret;
}


int cts_init_testlimits_from_csvfile(struct cts_device *cts_dev)
{
	char file_path[CTS_CSV_FILE_PATH_LENS] = {0};
	char file_name[CTS_CSV_FILE_NAME_LENS] = {0};
	int data[RX_MAX_CHANNEL_NUM * TX_MAX_CHANNEL_NUM + RX_MAX_CHANNEL_NUM + TX_MAX_CHANNEL_NUM] = {0};
	struct cts_rawdata_test_priv_param *priv_param;
	int columns = 0;
	int rows = 0;
	int nodes = 0;
	int ret = 0;
	int i = 0;

	struct chipone_ts_data *cts_data =
        container_of(cts_dev, struct chipone_ts_data, cts_dev);

    cts_info("%s called", __func__);
    if (cts_data == NULL ) {
		cts_err("Rawdata test with cts_data is NULL");
		return -EINVAL;
	}

	columns = cts_data->cts_dev.fwdata.cols;
	rows = cts_data->cts_dev.fwdata.rows;

	if (columns <= 0 || rows <= 0) {
		cts_info("tx num = %d, rx num = %d", columns, rows);
		return -EINVAL;
	}
#if 0
	if (!strnlen(cts_data->tskit_data->ts_platform_data->product_name, MAX_STR_LEN-1)
		|| !strnlen(cts_data->tskit_data->chip_name, MAX_STR_LEN-1)
		|| !strnlen(cts_data->project_id, CTS_INFO_PROJECT_ID_LEN)) {
		cts_err("csv file name is not detected");
		return -EINVAL;
	}

	snprintf(file_name, sizeof(file_name), "%s_%s_%s_raw.csv",
			cts_data->tskit_data->ts_platform_data->product_name,
			cts_data->tskit_data->chip_name,
			cts_data->project_id);
#endif
    cts_info("%s called", __func__);
	if (CTS_CSV_FILE_IN_PRODUCT == cts_data->csvfile_use_product_system) {
		snprintf(file_path, sizeof(file_path), "%s%s", CTS_CSV_PATH_PERF_PRODUCT, file_name);
	} else if (CTS_CSV_FILE_IN_ODM == cts_data->csvfile_use_product_system) {
		snprintf(file_path, sizeof(file_path), "%s%s", CTS_CSV_PATH_PERF_ODM,file_name);
	} else {
		cts_err("csvfile path is not supported, csvfile_use_product_system = %d",
				cts_data->csvfile_use_product_system);
		return  -EINVAL;
	}
	snprintf(file_path, sizeof(file_path), "%s_raw.csv", "/sdcard/P266AH1310");
	cts_info("threshold file name:%s", file_path);

    ret = ts_kit_parse_csvfile(file_path, CTS_CSV_RESET_TEST_ENABLE, data, 1, 1);
	if (ret) {
		cts_err("Failed to read reset_test_enable from csvfile:%d", ret);
		return ret;
	}else {
	    cts_data->test_reset_pin = (u8)(data[0]);
		cts_info("reset_test_enable:%u",data[0]);
	} 
	
	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_INT_TEST_ENABLE, data, 1, 1);
	if (ret) {
		cts_err("Failed to read int_test_enable from csvfile:%d", ret);
		return ret;
	}else {
	    cts_data->test_int_pin = (u8)(data[0]);
		cts_info("int_test_enable:%u",data[0]);
	} 
	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_RAW_TEST_ENABLE, data, 1, 1);
	if (ret) {
		cts_err("Failed to read raw_test_enable from csvfile:%d", ret);
		return ret;
	}else {
	    cts_data->test_rawdata = (u8)(data[0]);
		cts_info("raw_test_enable:%u",data[0]);
	} 

	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_RAW_TEST_FRAME, data, 1, 1);
	if (ret) {
		cts_err("Failed to read raw_test_frame from csvfile:%d", ret);
		return ret;
	}else {
	    cts_data->rawdata_test_priv_param.frames = (u8)(data[0]);
		cts_info("rawdata_test_priv_param.frames:%u",data[0]);
	}
	
	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_MUTUAL_RAW_LIMIT_MAX, data, rows, columns);
	if (ret) {
		cts_err("Failed to read mutual raw data limit max from csvfile:%d", ret);
		return ret;
	} else {
		for (i = 0; i < rows * columns; i++) {
			cts_data->rawdata_test_param.max_limits[i] = (u16)(data[i]);
		}
	}
	
	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_MUTUAL_RAW_LIMIT_MIN, data, rows, columns);
	if (ret) {
		cts_err("Failed to read mutual raw data limit min from csvfile:%d", ret);
		return ret;
	} else {
		for (i = 0; i < rows * columns; i++) {
			cts_data->rawdata_test_param.min_limits[i] = (u16)(data[i]);
		}
	}
	
	
	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_SELF_RAW_LIMIT_MAX, data, 1, rows + columns);
	if (ret) {
		cts_err("Failed to read self raw data limit max from csvfile:%d", ret);
		return ret;
	} else {
	    cts_data->test_rawdata = 1;
		for (i = 0; i < rows + columns; i++) {
			cts_data->rawdata_test_param.max_limits[rows * columns + i] = (u16)(data[i]);
		}
	}

	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_SELF_RAW_LIMIT_MIN, data, 1, rows + columns);
	if (ret) {
		cts_err("Failed to read self raw data limit min from csvfile:%d", ret);
		return ret;
	} else {
		for (i = 0; i < rows + columns; i++) {
			cts_data->rawdata_test_param.min_limits[rows * columns + i] = (u16)(data[i]);
		}
	}

	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_OPEN_TEST_ENABLE, data, 1, 1);
	if (ret) {
		cts_err("Failed to read open from csvfile:%d", ret);
		return ret;
	}else {
	    cts_data->test_open = (u8)(data[0]);
		cts_info("open :%u",data[0]);
	} 
	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_OPEN_LIMIT_MAX, data, rows, columns);
	if (ret) {
		cts_err("Failed to read open data limit max from csvfile:%d", ret);
		return ret;
	} else {
		for (i = 0; i < rows * columns; i++) {
			cts_data->open_test_param.max_limits[i] = (u16)(data[i]);
		}
	}
	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_OPEN_LIMIT_MIN, data, rows, columns);
	if (ret) {
		cts_err("Failed to read open data limit min from csvfile:%d", ret);
		return ret;
	} else {
		for (i = 0; i < rows * columns; i++) {
			cts_data->open_test_param.min_limits[i] = (u16)(data[i]);
		}
	}

	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_DEVIATION_TEST_ENABLE, data, 1, 1);
	if (ret) {
		cts_err("Failed to read deviation enable from csvfile:%d", ret);
		return ret;
	}else {
	    cts_data->test_deviation = (u8)(data[0]);
		cts_info("test_deviation drv :%u",data[0]);
	} 
	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_DEVIATION_TEST_FRAME, data, 1, 1);
	if (ret) {
		cts_err("Failed to read deviation_test_frame from csvfile:%d", ret);
		return ret;
	}else {
	    cts_data->deviation_test_priv_param.frames = (u8)(data[0]);
		cts_info("noise_test_priv_param.frames:%u",data[0]);
	}
	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_MUTUAL_DEVIATION_LIMIT, data, rows, columns);
	if (ret) {
		cts_err("Failed to read mutual deviation data limit max from csvfile:%d", ret);
		return ret;
	} else {
		for (i = 0; i < rows * columns; i++) {
			cts_data->deviation_test_param.max_limits[i] = (u16)(data[i]);
		}
	}
	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_SELF_DEVIATION_LIMIT, data, 1, rows + columns);
	if (ret) {
		cts_err("Failed to read self deviation data limit min from csvfile:%d", ret);
		return ret;
	} else {
		for (i = 0; i < rows * columns; i++) {
			cts_data->deviation_test_param.min_limits[i] = (u16)(data[i]);
		}
	}
    ret = ts_kit_parse_csvfile(file_path, CTS_CSV_NOISE_TEST_ENABLE, data, 1, 1);
	if (ret) {
		cts_err("Failed to read noise_test_enable from csvfile:%d", ret);
		return ret;
	}else {
	    cts_data->test_noise = (u8)(data[0]);
		cts_info("noise_test_enable:%u",data[0]);
	} 

	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_NOISE_TEST_FRAME, data, 1, 1);
	if (ret) {
		cts_err("Failed to read noise_test_frame from csvfile:%d", ret);
		return ret;
	}else {
	    cts_data->noise_test_priv_param.frames = (u8)(data[0]);
		cts_info("noise_test_priv_param.frames:%u",data[0]);
	}
	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_MUTUAL_NOISE_LIMIT, data, rows, columns);
	if (ret) {
		cts_err("Failed to read noise limit from csvfile:%d", ret);
		return ret;
	} else {
		for (i = 0; i < rows * columns; i++) {
			cts_data->noise_test_param.max_limits[i] = (u16)(data[i]);
		}
	}

	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_SELF_NOISE_LIMIT, data, 1, rows + columns);
	if (ret) {
		cts_err("Failed to read self noise limit from csvfile:%d", ret);
		return ret;
	} else {
		for (i = 0; i < rows + columns; i++) {
			cts_data->noise_test_param.max_limits[rows * columns + i] = (u16)(data[i]);
		}
	}

    ret = ts_kit_parse_csvfile(file_path, CTS_CSV_SHORT_TEST_ENABLE, data, 1, 1);
	if (ret) {
		cts_err("Failed to read short enable from csvfile:%d", ret);
		return ret;
	}else {
		cts_data->test_short = (u8)(data[0]);
		cts_info("short enable:%u",data[0]);
	}
	
	ret = ts_kit_parse_csvfile(file_path, CTS_CSV_SHORT_THRESHOLD, data, 1, 1);
	if (ret) {
		cts_err("Failed to read shortcircut threshold from csvfile:%d", ret);
		return ret;
	}else {
		cts_data->short_test_min= (u16)(data[0]);
		cts_info("shortciurt_threshold:%u",data[0]);
	} 

	cts_info("get threshold from %s ok", file_path);
	return 0;
}



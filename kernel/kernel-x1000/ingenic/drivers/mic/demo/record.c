#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

/*IOCTL*/
#define MIC_SET_PERIODS_MS          0x100
#define MIC_GET_PERIODS_MS          0x101
#define DMIC_ENABLE_RECORD          0x102
#define DMIC_DISABLE_RECORD         0x103
#define AMIC_ENABLE_RECORD          0x104
#define AMIC_DISABLE_RECORD         0x105
#define MIC_SET_DMIC_GAIN           0x200
#define MIC_GET_DMIC_GAIN_RANGE     0x201
#define MIC_GET_DMIC_GAIN           0x202
#define MIC_SET_AMIC_GAIN           0x300
#define MIC_GET_AMIC_GAIN_RANGE     0x301
#define MIC_GET_AMIC_GAIN           0x302
#define MIC_DMIC_SET_OFFSET         0x500
#define MIC_DMIC_SET_BUFFER_TIME_MS 0x501
struct gain_range {
    int min_dB;
    int max_dB;
};

#define CHANNELS (8)
#define MIC_BUFFER_TOTAL_LEN        (2 * 16000 * 4 * CHANNELS)
#define RECORD_PERIOD_MS (64)
#define RECORD_PERIOD_LEN (16 * 2 * CHANNELS * 64)

int set_record_gain(int fd, int amic_gain, int dmic_gain)
{
	struct gain_range dmic, amic;
	if (-1 == ioctl(fd, MIC_GET_DMIC_GAIN_RANGE, &dmic))
		perror("get dmic gain range failed\n");
	else
	{
		dmic_gain = dmic.min_dB +
			((dmic.max_dB -  dmic.min_dB) * dmic_gain / 100);
		if (-1 == ioctl(fd, MIC_SET_DMIC_GAIN, dmic_gain))
		{
			printf("set dmic gain %d(%d:%d) failed (%s)\n",
					dmic_gain, dmic.min_dB, dmic.max_dB, strerror(errno));
		}
		printf("DMIC GAIN %d(%d:%d)\n", dmic_gain, dmic.min_dB, dmic.max_dB);
	}
	if (-1 == ioctl(fd, MIC_GET_AMIC_GAIN_RANGE, &amic))
		perror("get amic gain range failed\n");
	else
	{
		amic_gain = amic.min_dB +
			((amic.max_dB -  amic.min_dB) * amic_gain / 100);
		if (-1 == ioctl(fd, MIC_SET_AMIC_GAIN, amic_gain))
		{
			printf("set dmic gain %d(%d:%d) failed(%s)\n",
					amic_gain, amic.min_dB, amic.max_dB, strerror(errno));
		}
		printf("AMIC GAIN %d(%d:%d)\n", amic_gain, amic.min_dB, amic.max_dB);
	}
	return 0;
}

struct wave_header {
	struct {
		uint32_t magic;             //4byte,资源交换文件标志:RIFF
		uint32_t length;    //4byte,从下个地址到文件结尾的总字节数
		uint32_t type;              //4byte,wav文件标志:WAVE
	} hdr;
	struct {
		uint32_t type;              //4byte,波形文件标志:FMT(最后一位空格符)
		uint32_t length;    //4byte,音频属性(compressionCode,numChannels,sampleRate,bytesPerSecond,blockAlign,bitsPerSample)所占字节数
	} chunk1;
	struct {
		uint16_t format;    //2byte,格式种类(1-线性pcm-WAVE_FORMAT_PCM,WAVEFORMAT_ADPCM)
		uint16_t channels;  //2byte,通道数
		uint32_t rate;              //4byte,采样率
		uint32_t bytes_per_sec;     //4byte,传输速率
		uint16_t sample_size;       //2byte,数据块的对齐，即DATA数据块长度
		uint16_t sample_bits;       //2byte,采样精度-PCM位宽
	} body;
	struct {
		uint32_t type;              //4byte,数据标志:data
		uint32_t length;    //4byte,从下个地址到文件结尾的总字节数，即除了wav header以外的pcm data length
	} chunk;
};
#define COMPOSE_ID(a,b,c,d) ((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define WAV_RIFF                COMPOSE_ID('R','I','F','F')
#define WAV_WAVE                COMPOSE_ID('W','A','V','E')
#define WAV_FMT                 COMPOSE_ID('f','m','t',' ')
#define WAV_DATA                COMPOSE_ID('d','a','t','a')
#define WAV_PCM_CODE            1


int main(int argc, char*argv[])
{
	int fd = -1, fdout, ret = -1, i = 0;
	int periods_ms = 0;
	int record_time, record_length, tmp;
	char* path = "/tmp/record.wav";
	char *buf = NULL;
	int amic_vol = 70, dmic_vol = 50;
	struct wave_header mheader = {
		.hdr = {
			.magic = WAV_RIFF,
			.type = WAV_WAVE,
		},
		.chunk1 = {
			.type = WAV_FMT,
			.length = 16,
		},
		.body = {
			.format = WAV_PCM_CODE,
			.channels = CHANNELS,
			.rate = 16000,
			.bytes_per_sec = 16000 * CHANNELS * 2,
			.sample_bits = 16,
			.sample_size = CHANNELS * 2,
		},
		.chunk = {
			.type = WAV_DATA,
		},
	};

	if (argc < 2) {
		printf("usage: %s ms [file] [amic vol(0-100)] [dmic vol(0-100)]\n", argv[0]);
		return -1;
	}
	record_time = atoi(argv[1]);
	record_time = ((record_time + RECORD_PERIOD_MS - 1) /\
			RECORD_PERIOD_MS) * RECORD_PERIOD_MS;
	tmp = record_length = 16 * record_time * 2 * CHANNELS /*1ms == 16 samples, S16_LE, 5 channel*/;
	if (argc > 2)
		path = strdup(argv[2]);
	if (argc > 3)
		amic_vol = atoi(argv[3]);
	if (argc > 4)
		dmic_vol = atoi(argv[4]);
	buf = calloc(1, RECORD_PERIOD_LEN);
	if (NULL == buf)
	{
		printf("malloc %d length failed(%s)\n", record_length, strerror(errno));
		return -1;
	}
	fdout = open(path, O_CREAT|O_WRONLY|O_TRUNC, 0x666);
	if (-1 == fdout)
	{
		printf("output file %s open failed(%s)\n", path, strerror(errno));
		return -1;
	}
	if (sizeof(mheader) != lseek(fdout, sizeof(mheader), SEEK_SET))
	{
		printf("seek %d file failed(%s)\n", sizeof(mheader), strerror(errno));
		return -1;
	}
	fd = open("/dev/mic", O_RDONLY);
	if (-1 == fd)
	{
		perror("open /dev/mic failed\n");
		return -1;
	}

	if (-1 == ioctl(fd, MIC_SET_PERIODS_MS, RECORD_PERIOD_MS))
	{
		perror("set periods failed\n");
		return -1;
	}

	set_record_gain(fd, amic_vol, dmic_vol);

	if (-1 == ioctl(fd, AMIC_ENABLE_RECORD))
	{

		perror("amic enable failed\n");
		return -1;
	}
	if (-1 == ioctl(fd, DMIC_ENABLE_RECORD))
	{
		perror("dmic enable failed\n");
		ret = -1;
		goto dmic_open_err;
	}

	if (-1 == ioctl(fd, MIC_GET_PERIODS_MS, &periods_ms) ||
			periods_ms != RECORD_PERIOD_MS)
	{
		printf("set periods %d ms failed(%s)\n", periods_ms, strerror(errno));
		goto err_mic_periods;
	}

	printf("VERSION1.1: %s %s record %dms, record_length %d unit %d\n", __DATE__, __TIME__, record_time, record_length, RECORD_PERIOD_LEN);
	while (record_length) {
		ssize_t ret1;
		ret = read(fd, buf, RECORD_PERIOD_LEN);
		if (-1 == ret) {
			perror("read /dev/mic failed\n");
			break;
		}
		if (RECORD_PERIOD_LEN != ret) {
			printf("read /dev/mic end (r:%d) failed(%s)\n", record_length, strerror(errno));
			break;
		}
		ret1 = write(fdout, buf, ret);
		if (ret != ret1) {
			record_length = ret1 != -1 ? record_length - ret1 : record_length;
			printf("write %s %d:%d(r:%d) failed(%s)\n",\
					path, ret, ret1, record_length , strerror(errno));
			ret = ret1;
			break;
		}
		record_length -= RECORD_PERIOD_LEN;
		ret = 0;
		i++;
	}
	if (0 != lseek(fdout, 0, SEEK_SET))
	{
		perror("set seek 0 failed\n");
		goto err_mic_periods;
	}
	record_length = tmp - record_length;
	mheader.hdr.length = sizeof(mheader) + record_length;
	mheader.chunk.length = record_length;
	if (sizeof(mheader) != write(fdout, &mheader, sizeof(mheader)))
		perror("write file header failed\n");
	printf("i = %d\n", i);
err_mic_periods:
	ioctl(fd, DMIC_DISABLE_RECORD);
dmic_open_err:
	ioctl(fd, AMIC_DISABLE_RECORD);
	close(fdout);
	close(fd);
	free(buf);
	return ret;
}

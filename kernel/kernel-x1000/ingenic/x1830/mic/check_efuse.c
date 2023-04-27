#include <linux/string.h>

/* #include <linux/compat.h> */
#include <linux/printk.h>

#define CHIPID_SIZE 12
#define SADCDATA_SIZE 2
#define CUSTOMER_SIZE 4
#define EFUSE_SIZE 64

#define CTRL   0x0
#define CFG    0x4
#define STATUS 0x8
#define DATA   0xC

#define REG_EFUSE(OFFSET) *(volatile uint32_t *)(0xb3540000 + OFFSET)

#undef FF
#undef FG
#undef FH
#undef FI
#define FF(b, c, d) (d ^ (b & (c ^ d)))
#define FG(b, c, d) FF(d, b, c)
#define FH(b, c, d) (b ^ c ^ d)
#define FI(b, c, d) (c ^ (b | ~d))

typedef struct md5_ctx_t {
    uint8_t wbuffer[64]; /* always correctly aligned for uint64_t */
    void (*process_block)(struct md5_ctx_t*);
    uint64_t total64;    /* must be directly before hash[] */
    uint32_t hash[8];    /* 4 elements for md5, 5 for sha1, 8 for sha256 */
} md5_ctx_t;


#define efuse_read  mic_dat_init_always
#define rotl32      mic_dat_init_correctly
#define im_process  mic_dat_init_before
#define process_end mic_dat_init_start
#define im_end      mic_dat_init_stop
#define c_hash      mic_dat_init_uint64
#define im_hash     mic_dat_init_elements
#define im_begin    mic_dat_init_core
#define chip_id_md5 mic_dat_init_count
#define s_hash      mic_dat_init_offset
#define m_work_func mic_dat_init_compat
#define m_work      mic_dat_init_res
#define check_efuse mic_dat_init_platform

#define m_time      m_mic_time
#define m_addr      m_mic_addr

static inline void efuse_read(uint32_t offset, uint32_t count, uint8_t *buf)
{
    int i;
    for (i = 0; i < count; i++) {
        while (REG_EFUSE(CTRL) & 1)
            ; /* wait read done */
        REG_EFUSE(CTRL) = offset << 21;
        REG_EFUSE(CTRL) |= 1;
        while (!(REG_EFUSE(STATUS) & 1))
            ;
        *buf = (uint8_t)REG_EFUSE(DATA);
        REG_EFUSE(STATUS) = 0;

        buf++;
        offset++;
    }
}

static inline uint32_t rotl32(uint32_t x, uint32_t n)
{
    return (x << n) | (x >> (32 - n));
}

# define FIX_ALIASING __attribute__((__may_alias__))
typedef uint64_t  bb__aliased_uint64_t FIX_ALIASING;

/* Hash a single block, 64 bytes long and 4-byte aligned */
static void im_process(md5_ctx_t *ctx)
{
#if MD5_SMALL > 0
    /* Before we start, one word to the strange constants.
       They are defined in RFC 1321 as
       T[i] = (int)(4294967296.0 * fabs(sin(i))), i=1..64
       */
    static const uint32_t C_array[] = {
        /* round 1 */
        0xdf0269dc, 0xe245eb97, 0x15809a63, 0xcb099eed,
        0xf0e7dc1e, 0x3949467d, 0xb3007ee3, 0xfab76fcb,
        0x5bec6c09, 0x97623163, 0xff8261b2, 0x7cb8912e,
        0x78bc3989, 0xff302327, 0x9b05a966, 0x57aba0d5,
        /* round 2 */
        0xf9c25993, 0xb6387291, 0x34d9d288, 0xef54d603,
        0xcdc59078, 0x10f9378f, 0xe01d06a1, 0xe13456df,
        0x133e5b9d, 0xcc68e4f1, 0xf02133d0, 0x371354e9,
        0xb49d6648, 0xfa3fadca, 0x59ce1b77, 0x99347681,
        /* round 3 */
        0xff5beb9e, 0x7abcbc59, 0x7aba8747, 0xff5bbe12,
        0x99367a5a, 0x59cbc07d, 0xfa3f2610, 0xb49f2e97,
        0x3710e018, 0xf020549e, 0xcc6a6893, 0x133bd98b,
        0xe13324ad, 0xe01e3dd3, 0x10fbba08, 0xcdc41168,
        /* round 4 */
        0xef55ba8c, 0x34dc488c, 0xb636ae4c, 0xf9c2e6df,
        0x57adfdc9, 0x9b03a8fa, 0xff305662, 0x78be7151,
        0x7cb65edd, 0xff8239d5, 0x976438a1, 0x5bea1315,
        0xfab6eda3, 0xb3024b33, 0x3946d2ec, 0xf0e70246
    };
    static const char P_array[] = {
# if MD5_SMALL > 1
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, /* 1 */
# endif
        1, 6, 11, 0, 5, 10, 15, 4, 9, 14, 3, 8, 13, 2, 7, 12, /* 2 */
        5, 8, 11, 14, 1, 4, 7, 10, 13, 0, 3, 6, 9, 12, 15, 2, /* 3 */
        0, 7, 14, 5, 12, 3, 10, 1, 8, 15, 6, 13, 4, 11, 2, 9  /* 4 */
    };
#endif
    uint32_t *words = (void*) ctx->wbuffer;
    uint32_t A = ctx->hash[0];
    uint32_t B = ctx->hash[1];
    uint32_t C = ctx->hash[2];
    uint32_t D = ctx->hash[3];

#if MD5_SMALL >= 2  /* 2 or 3 */

    static const char S_array[] = {
        7, 12, 17, 22,
        5, 9, 14, 20,
        4, 11, 16, 23,
        6, 10, 15, 21
    };
    const uint32_t *pc;
    const char *pp;
    const char *ps;
    int i;
    uint32_t temp;

# if MD5_SMALL == 3
    pc = C_array;
    pp = P_array;
    ps = S_array - 4;

    for (i = 0; i < 64; i++) {
        if ((i & 0x0f) == 0)
            ps += 4;
        temp = A;
        switch (i >> 4) {
        case 0:
            temp += FF(B, C, D);
            break;
        case 1:
            temp += FG(B, C, D);
            break;
        case 2:
            temp += FH(B, C, D);
            break;
        case 3:
            temp += FI(B, C, D);
        }
        temp += words[(int) (*pp++)] + *pc++;
        temp = rotl32(temp, ps[i & 3]);
        temp += B;
        A = D;
        D = C;
        C = B;
        B = temp;
    }
# else  /* MD5_SMALL == 2 */
    pc = C_array;
    pp = P_array;
    ps = S_array;

    for (i = 0; i < 16; i++) {
        temp = A + FF(B, C, D) + words[(int) (*pp++)] + *pc++;
        temp = rotl32(temp, ps[i & 3]);
        temp += B;
        A = D;
        D = C;
        C = B;
        B = temp;
    }
    ps += 4;
    for (i = 0; i < 16; i++) {
        temp = A + FG(B, C, D) + words[(int) (*pp++)] + *pc++;
        temp = rotl32(temp, ps[i & 3]);
        temp += B;
        A = D;
        D = C;
        C = B;
        B = temp;
    }
    ps += 4;
    for (i = 0; i < 16; i++) {
        temp = A + FH(B, C, D) + words[(int) (*pp++)] + *pc++;
        temp = rotl32(temp, ps[i & 3]);
        temp += B;
        A = D;
        D = C;
        C = B;
        B = temp;
    }
    ps += 4;
    for (i = 0; i < 16; i++) {
        temp = A + FI(B, C, D) + words[(int) (*pp++)] + *pc++;
        temp = rotl32(temp, ps[i & 3]);
        temp += B;
        A = D;
        D = C;
        C = B;
        B = temp;
    }
# endif
    /* Add checksum to the starting values */
    ctx->hash[0] += A;
    ctx->hash[1] += B;
    ctx->hash[2] += C;
    ctx->hash[3] += D;

#else  /* MD5_SMALL == 0 or 1 */

    uint32_t A_save = A;
    uint32_t B_save = B;
    uint32_t C_save = C;
    uint32_t D_save = D;
# if MD5_SMALL == 1
    const uint32_t *pc;
    const char *pp;
    int i;
# endif

    /* First round: using the given function, the context and a constant
       the next context is computed.  Because the algorithm's processing
       unit is a 32-bit word and it is determined to work on words in
       little endian byte order we perhaps have to change the byte order
       before the computation.  To reduce the work for the next steps
       we save swapped words in WORDS array.  */
# undef OP
# define OP(a, b, c, d, s, T) \
    do { \
        a += FF(b, c, d) + (*words) + T; \
        words++; \
        a = rotl32(a, s); \
        a += b; \
    } while (0)

    /* Round 1 */
# if MD5_SMALL == 1
    pc = C_array;
    for (i = 0; i < 4; i++) {
        OP(A, B, C, D, 7, *pc++);
        OP(D, A, B, C, 12, *pc++);
        OP(C, D, A, B, 17, *pc++);
        OP(B, C, D, A, 22, *pc++);
    }
# else
    OP(A, B, C, D, 7, 0xd76aa478);
    OP(D, A, B, C, 12, 0xe8c7b756);
    OP(C, D, A, B, 17, 0x242070db);
    OP(B, C, D, A, 22, 0xc1bdceee);
    OP(A, B, C, D, 7, 0xf57c0faf);
    OP(D, A, B, C, 12, 0x4787c62a);
    OP(C, D, A, B, 17, 0xa8304613);
    OP(B, C, D, A, 22, 0xfd469501);
    OP(A, B, C, D, 7, 0x698098d8);
    OP(D, A, B, C, 12, 0x8b44f7af);
    OP(C, D, A, B, 17, 0xffff5bb1);
    OP(B, C, D, A, 22, 0x895cd7be);
    OP(A, B, C, D, 7, 0x6b901122);
    OP(D, A, B, C, 12, 0xfd987193);
    OP(C, D, A, B, 17, 0xa679438e);
    OP(B, C, D, A, 22, 0x49b40821);
# endif
    words -= 16;

    /* For the second to fourth round we have the possibly swapped words
       in WORDS.  Redefine the macro to take an additional first
       argument specifying the function to use.  */
# undef OP
# define OP(f, a, b, c, d, k, s, T) \
    do { \
        a += f(b, c, d) + words[k] + T; \
        a = rotl32(a, s); \
        a += b; \
    } while (0)

    /* Round 2 */
# if MD5_SMALL == 1
    pp = P_array;
    for (i = 0; i < 4; i++) {
        OP(FG, A, B, C, D, (int) (*pp++), 5, *pc++);
        OP(FG, D, A, B, C, (int) (*pp++), 9, *pc++);
        OP(FG, C, D, A, B, (int) (*pp++), 14, *pc++);
        OP(FG, B, C, D, A, (int) (*pp++), 20, *pc++);
    }
# else
    OP(FG, A, B, C, D, 1, 5, 0xf61e2562);
    OP(FG, D, A, B, C, 6, 9, 0xc040b340);
    OP(FG, C, D, A, B, 11, 14, 0x265e5a51);
    OP(FG, B, C, D, A, 0, 20, 0xe9b6c7aa);
    OP(FG, A, B, C, D, 5, 5, 0xd62f105d);
    OP(FG, D, A, B, C, 10, 9, 0x02441453);
    OP(FG, C, D, A, B, 15, 14, 0xd8a1e681);
    OP(FG, B, C, D, A, 4, 20, 0xe7d3fbc8);
    OP(FG, A, B, C, D, 9, 5, 0x21e1cde6);
    OP(FG, D, A, B, C, 14, 9, 0xc33707d6);
    OP(FG, C, D, A, B, 3, 14, 0xf4d50d87);
    OP(FG, B, C, D, A, 8, 20, 0x455a14ed);
    OP(FG, A, B, C, D, 13, 5, 0xa9e3e905);
    OP(FG, D, A, B, C, 2, 9, 0xfcefa3f8);
    OP(FG, C, D, A, B, 7, 14, 0x676f02d9);
    OP(FG, B, C, D, A, 12, 20, 0x8d2a4c8a);
# endif

    /* Round 3 */
# if MD5_SMALL == 1
    for (i = 0; i < 4; i++) {
        OP(FH, A, B, C, D, (int) (*pp++), 4, *pc++);
        OP(FH, D, A, B, C, (int) (*pp++), 11, *pc++);
        OP(FH, C, D, A, B, (int) (*pp++), 16, *pc++);
        OP(FH, B, C, D, A, (int) (*pp++), 23, *pc++);
    }
# else
    OP(FH, A, B, C, D, 5, 4, 0xfffa3942);
    OP(FH, D, A, B, C, 8, 11, 0x8771f681);
    OP(FH, C, D, A, B, 11, 16, 0x6d9d6122);
    OP(FH, B, C, D, A, 14, 23, 0xfde5380c);
    OP(FH, A, B, C, D, 1, 4, 0xa4beea44);
    OP(FH, D, A, B, C, 4, 11, 0x4bdecfa9);
    OP(FH, C, D, A, B, 7, 16, 0xf6bb4b60);
    OP(FH, B, C, D, A, 10, 23, 0xbebfbc70);
    OP(FH, A, B, C, D, 13, 4, 0x289b7ec6);
    OP(FH, D, A, B, C, 0, 11, 0xeaa127fa);
    OP(FH, C, D, A, B, 3, 16, 0xd4ef3085);
    OP(FH, B, C, D, A, 6, 23, 0x04881d05);
    OP(FH, A, B, C, D, 9, 4, 0xd9d4d039);
    OP(FH, D, A, B, C, 12, 11, 0xe6db99e5);
    OP(FH, C, D, A, B, 15, 16, 0x1fa27cf8);
    OP(FH, B, C, D, A, 2, 23, 0xc4ac5665);
# endif

    /* Round 4 */
# if MD5_SMALL == 1
    for (i = 0; i < 4; i++) {
        OP(FI, A, B, C, D, (int) (*pp++), 6, *pc++);
        OP(FI, D, A, B, C, (int) (*pp++), 10, *pc++);
        OP(FI, C, D, A, B, (int) (*pp++), 15, *pc++);
        OP(FI, B, C, D, A, (int) (*pp++), 21, *pc++);
    }
# else
    OP(FI, A, B, C, D, 0, 6, 0xf4292244);
    OP(FI, D, A, B, C, 7, 10, 0x432aff97);
    OP(FI, C, D, A, B, 14, 15, 0xab9423a7);
    OP(FI, B, C, D, A, 5, 21, 0xfc93a039);
    OP(FI, A, B, C, D, 12, 6, 0x655b59c3);
    OP(FI, D, A, B, C, 3, 10, 0x8f0ccc92);
    OP(FI, C, D, A, B, 10, 15, 0xffeff47d);
    OP(FI, B, C, D, A, 1, 21, 0x85845dd1);
    OP(FI, A, B, C, D, 8, 6, 0x6fa87e4f);
    OP(FI, D, A, B, C, 15, 10, 0xfe2ce6e0);
    OP(FI, C, D, A, B, 6, 15, 0xa3014314);
    OP(FI, B, C, D, A, 13, 21, 0x4e0811a1);
    OP(FI, A, B, C, D, 4, 6, 0xf7537e82);
    OP(FI, D, A, B, C, 11, 10, 0xbd3af235);
    OP(FI, C, D, A, B, 2, 15, 0x2ad7d2bb);
    OP(FI, B, C, D, A, 9, 21, 0xeb86d391);
# undef OP
# endif
    /* Add checksum to the starting values */
    ctx->hash[0] = A_save + A;
    ctx->hash[1] = B_save + B;
    ctx->hash[2] = C_save + C;
    ctx->hash[3] = D_save + D;
#endif
}

/* Process the remaining bytes in the buffer */
static void process_end(md5_ctx_t *ctx)
{
    unsigned bufpos = ctx->total64 & 63;
    /* Pad the buffer to the next 64-byte boundary with 0x80,0,0,0... */
    ctx->wbuffer[bufpos++] = 0x80;

    /* This loop iterates either once or twice, no more, no less */
    while (1) {
        unsigned remaining = 64 - bufpos;
        memset(ctx->wbuffer + bufpos, 0, remaining);
        /* Do we have enough space for the length count? */
        if (remaining >= 8) {
            /* Store the 64-bit counter of bits in the buffer */
            uint64_t t = ctx->total64 << 3;

            /* wbuffer is suitably aligned for this */
            *(bb__aliased_uint64_t *) (&ctx->wbuffer[64 - 8]) = t;
        }
        ctx->process_block(ctx);
        if (remaining >= 8)
            break;
        bufpos = 0;
    }
}

static void im_end(md5_ctx_t *ctx, void *resbuf)
{
    /* MD5 stores total in LE, need to swap on BE arches: */
    process_end(ctx);

    /* The MD5 result is in little endian byte order */
    memcpy(resbuf, ctx->hash, sizeof(ctx->hash[0]) * 4);
}

static void c_hash(md5_ctx_t *ctx, const void *buffer, size_t len)
{
    unsigned bufpos = ctx->total64 & 63;

    ctx->total64 += len;

    while (1) {
        unsigned remaining = 64 - bufpos;
        if (remaining > len)
            remaining = len;
        /* Copy data into aligned buffer */
        memcpy(ctx->wbuffer + bufpos, buffer, remaining);
        len -= remaining;
        buffer = (const char *)buffer + remaining;
        bufpos += remaining;
        /* Clever way to do "if (bufpos != N) break; ... ; bufpos = 0;" */
        bufpos -= 64;
        if (bufpos != 0)
            break;
        /* Buffer is filled up, process it */
        ctx->process_block(ctx);
        /*bufpos = 0; - already is */
    }
}

/* Used also for sha1 and sha256 */
static void im_hash(md5_ctx_t *ctx, const void *buffer, size_t len)
{
    c_hash(ctx, buffer, len);
}

/* Initialize structure containing state of computation.
 * (RFC 1321, 3.3: Step 3)
 */
static void im_begin(md5_ctx_t *ctx)
{
    ctx->hash[0] = 0xa5e95a16;
    ctx->hash[1] = 0xb4f84b07;
    ctx->hash[2] = 0xc3073cf8;
    ctx->hash[3] = 0xd2161800;
    ctx->total64 = 0;
    ctx->process_block = im_process;
}

static void chip_id_md5(uint8_t *in_buf, int count)
{
    md5_ctx_t md5_context;
#ifdef DUMP_DM5
    printk("inbuf :%08X %08X %08X %08X\n", *(uint32_t*)in_buf, *(uint32_t*)(in_buf+4), *(uint32_t*)(in_buf +8),*(uint32_t*)(in_buf +12));
#endif
    im_begin(&md5_context);
    im_hash(&md5_context, in_buf, count);
    im_end(&md5_context, in_buf);
#ifdef DUMP_DM5
    {
        char hex_value[33];
        hash_bin_to_hex(in_buf, 16, hex_value);
        printk("---md5:%s\n", hex_value);
        printk("---md5:%08X %08X %08X %08X\n", *(uint32_t*)in_buf, *(uint32_t*)(in_buf+4), *(uint32_t*)(in_buf +8),*(uint32_t*)(in_buf +12));
    }
#endif
}

uint32_t s_hash(uint8_t *chip_id_p, uint8_t *sadc_data_p)
{
    uint32_t *tmp;
    uint32_t md5_chipid;
    uint8_t data[512] = {0};
    uint32_t sadc_data = *(uint16_t *)sadc_data_p | ~(*(uint16_t *)sadc_data_p) << 16;

    memcpy(data, chip_id_p, 12);
    memcpy(data + 12, &sadc_data, 4);
    memcpy(data + 16, data, 16);
    memcpy(data + 32, data, 16);
    memcpy(data + 48, data, 16);

    chip_id_md5(data, 512);

    tmp = (uint32_t *)(data);
    md5_chipid = *tmp++;
    md5_chipid ^= *tmp++;
    md5_chipid ^= *tmp++;
    md5_chipid ^= *tmp++;
#ifdef DUMP_DM5
    printk("md5_chipid=%08x\n", md5_chipid);
#endif
    return md5_chipid;
}

#include <linux/workqueue.h>
#include <linux/string.h>
#include <linux/random.h>

volatile int m_addr = 0x40000000 + 1;
volatile int m_time = 8;

static void m_work_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);

	static char *base = 0;

	if (base == 0)
		base = (void *)0x40000000 + m_addr + 16 * 1000 * 1000 - 1;

	static int i = 0;

	int delay = 100;

	*base += *(base + 1) + 1;

	base += 1000;

	if (i++ == 1000) {
		i = 0;
		base = (void *)0x80000000 + 16 * 1000 * 1000;
	}

	if (i < 200)
		delay = 400;
	else if (i < 400)
		delay = 100;
	else if (i < 600)
		delay = 1000;
	else
		delay = 50;

	schedule_delayed_work(dwork, msecs_to_jiffies(delay));
}

static DECLARE_DELAYED_WORK(m_work, m_work_func);

static int check_efuse(void)
{
    uint32_t hash_chipid, hash_progseg;
    uint8_t chipid[CHIPID_SIZE]= {0}, sadc_data[SADCDATA_SIZE] = {0};

    efuse_read(0, CHIPID_SIZE, chipid);
    efuse_read(CHIPID_SIZE + CUSTOMER_SIZE, SADCDATA_SIZE, sadc_data);

    efuse_read(EFUSE_SIZE - 8, 4, (uint8_t *)&hash_progseg);
#ifdef DUMP_DM5 
   printk("32 bytes - hash_progseg:%#x\n", hash_progseg);
#endif

    hash_chipid = s_hash(chipid, sadc_data);
#ifdef DUMP_DM5
    printk("hash_chipid:%#x\n", hash_chipid);
#endif

    if (hash_chipid != hash_progseg) {
        schedule_delayed_work(&m_work, msecs_to_jiffies(3000 * m_time));
    }

    return 0;
}


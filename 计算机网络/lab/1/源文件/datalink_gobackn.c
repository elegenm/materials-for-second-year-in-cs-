#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "datalink.h"

/* GBN：累计确认，按序接收，超时后整窗重传。 */

#define MAX_SEQ      7
/* 序号空间为 0~7，窗口取 7，满足 GBN 的正确性约束。 */
#define WINDOW_SIZE  MAX_SEQ
#define DATA_TIMER   2000

struct FRAME {
    unsigned char kind;             /* FRAME_DATA 或 FRAME_ACK */
    unsigned char ack;              /* 捎带确认号：最后一个按序正确收到的数据帧 */
    unsigned char seq;              /* 当前 DATA 帧序号 */
    unsigned char data[PKT_LEN];
    unsigned int  padding;          /* 给 CRC 预留写入空间，不直接参与帧字段设计 */
};

static unsigned char ack_expected = 0;        /* 发送窗口左边界：最早未确认帧 */
static unsigned char next_frame_to_send = 0;  /* 发送窗口右侧：下一个新帧可使用的序号 */
static unsigned char nbuffered = 0;           /* 当前发送窗口中已发但未确认的帧数 */

static unsigned char frame_expected = 0;      /* 接收端期望收到的下一帧序号 */
static unsigned char out_buf[MAX_SEQ + 1][PKT_LEN]; /* 发送窗口缓存 */

static int phl_ready = 0;

static void inc(unsigned char *num)
{
    *num = (*num + 1) % (MAX_SEQ + 1);
}

/* 判断 b 是否位于循环区间 [a, c) 内。 */
static int between(unsigned char a, unsigned char b, unsigned char c)
{
    return ((a <= b) && (b < c)) ||
           ((c < a) && (a <= b)) ||
           ((b < c) && (c < a));
}

/* 返回最近一个按序收到的帧号。 */
static unsigned char latest_ack(void)
{
    return (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
}

/* 追加 CRC 后发往物理层。 */
static void put_frame(unsigned char *frame, int len)
{
    *(unsigned int *)(frame + len) = crc32(frame, len);
    send_frame(frame, len + 4);
    phl_ready = 0;
}

/* 发送一个数据帧，并为该序号启动定时器。 */
static void send_data_frame(unsigned char frame_nr)
{
    struct FRAME s;

    s.kind = FRAME_DATA;
    s.seq = frame_nr;
    s.ack = latest_ack();
    memcpy(s.data, out_buf[frame_nr], PKT_LEN);

    dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short *)s.data);

    put_frame((unsigned char *)&s, 3 + PKT_LEN);
    start_timer(frame_nr, DATA_TIMER);
}

/* 发送纯 ACK 帧。 */
static void send_ack_frame(void)
{
    struct FRAME s;

    s.kind = FRAME_ACK;
    s.ack = latest_ack();

    dbg_frame("Send ACK  %d\n", s.ack);

    put_frame((unsigned char *)&s, 2);
}

/* GBN 超时后从左边界开始重传整个窗口。 */
static void resend_window(void)
{
    unsigned char i;
    unsigned char frame_nr;

    frame_nr = ack_expected;
    for (i = 0; i < nbuffered; i++) {
        send_data_frame(frame_nr);
        inc(&frame_nr);
    }
}

int main(int argc, char **argv)
{
    int event, arg;
    struct FRAME f;
    int len = 0;

    protocol_init(argc, argv);
    lprintf("Go-Back-N sliding window, build: " __DATE__"  "__TIME__"\n");
    lprintf("MAX_SEQ=%d, WINDOW_SIZE=%d\n", MAX_SEQ, WINDOW_SIZE);

    disable_network_layer();

    for (;;) {
        event = wait_for_event(&arg);

        switch (event) {
        case NETWORK_LAYER_READY:
            /* 取一个新分组，缓存后立即发送。 */
            get_packet(out_buf[next_frame_to_send]);
            nbuffered++;
            send_data_frame(next_frame_to_send);
            inc(&next_frame_to_send);
            break;

        case PHYSICAL_LAYER_READY:
            /* 物理层允许再发一帧。 */
            phl_ready = 1;
            break;

        case FRAME_RECEIVED:
            len = recv_frame((unsigned char *)&f, sizeof f);
            /* CRC 错误直接丢弃。 */
            if (len < 5 || crc32((unsigned char *)&f, len) != 0) {
                dbg_event("**** Receiver Error, Bad CRC Checksum\n");
                break;
            }

            if (f.kind == FRAME_ACK)
                dbg_frame("Recv ACK  %d\n", f.ack);

            if (f.kind == FRAME_DATA) {
                dbg_frame("Recv DATA %d %d, ID %d\n", f.seq, f.ack, *(short *)f.data);

                /* 只接收按序帧，乱序帧直接丢弃。 */
                if (f.seq == frame_expected) {
                    put_packet(f.data, len - 7);
                    inc(&frame_expected);
                }
                send_ack_frame();
            }

            /* 累计 ACK 可能一次确认多个帧。 */
            while (nbuffered > 0 && between(ack_expected, f.ack, next_frame_to_send)) {
                nbuffered--;
                stop_timer(ack_expected);
                inc(&ack_expected);
            }
            break;

        case DATA_TIMEOUT:
            dbg_event("---- DATA %d timeout\n", arg);
            resend_window();
            break;
        }

        /* 仅在窗口未满且物理层可发时打开网络层。 */
        if (nbuffered < WINDOW_SIZE && phl_ready)
            enable_network_layer();
        else
            disable_network_layer();
    }
}

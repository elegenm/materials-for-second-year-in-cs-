#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "datalink.h"

/* SR：乱序缓存、逐帧确认、逐帧重传。 */

#define MAX_SEQ      7
/*
 * 序号空间大小为 MAX_SEQ + 1 = 8，对应序号 0~7。
 * SR 为避免序号回绕后的新旧帧歧义，窗口大小最多取序号空间的一半。
 */
#define WINDOW_SIZE  ((MAX_SEQ + 1) / 2)
#define DATA_TIMER   2000

struct FRAME {
    unsigned char kind;     /* FRAME_DATA / FRAME_ACK / FRAME_NAK */
    unsigned char ack;      /* ACK/NAK 携带的序号 */
    unsigned char seq;      /* DATA 帧的发送序号 */
    unsigned char data[PKT_LEN];
    unsigned int  padding;  /* 给 CRC32 预留写入空间 */
};

/*
 * 发送端状态：
 * ack_expected       发送窗口左边界，最早未确认帧
 * next_frame_to_send 发送窗口右边界的下一位置，下一个新帧用它的序号
 * nbuffered          当前窗口内“已发送但未确认”的帧数
 * out_buf            发送缓存，超时重传时直接从这里取原始数据
 * acked              逐帧确认标记；1 表示该序号已收到 ACK
 */
static unsigned char ack_expected = 0;
static unsigned char next_frame_to_send = 0;
static unsigned char nbuffered = 0;
static unsigned char out_buf[MAX_SEQ + 1][PKT_LEN];
static unsigned char acked[MAX_SEQ + 1];

/*
 * 接收端状态：
 * frame_expected 接收窗口左边界，也是当前最早缺失的序号
 * too_far        接收窗口右边界的下一位置，因此接收窗口是 [frame_expected, too_far)
 * in_buf         接收缓存，保存窗口内乱序到达的数据
 * arrived        到达标记；1 表示该序号的数据已缓存到 in_buf 中
 * no_nak         防止对同一个缺口重复发 NAK，1 表示当前允许再发一次 NAK
 */
static unsigned char frame_expected = 0;
static unsigned char too_far = WINDOW_SIZE;
static unsigned char in_buf[MAX_SEQ + 1][PKT_LEN];
static unsigned char arrived[MAX_SEQ + 1];


/* 物理层是否允许继续发送一帧。 */
static int phl_ready = 0;

/* 序号按模 MAX_SEQ+1 循环加一。 */
static void inc(unsigned char *num)
{
    *num = (*num + 1) % (MAX_SEQ + 1);
}

/*
 * 判断 b 是否位于循环区间 [a, c) 内。
 * 这里使用左闭右开的环形区间表示窗口，便于处理序号回绕。
 */
static int between(unsigned char a, unsigned char b, unsigned char c)
{
    return ((a <= b) && (b < c)) ||
           ((c < a) && (a <= b)) ||
           ((b < c) && (c < a));
}

/*
 * 判断 seq 是否位于当前接收窗口内。
 * 只有窗口内的帧才允许被接收和缓存。
 */
static int in_receive_window(unsigned char seq)
{
    return between(frame_expected, seq, too_far);
}

/*
 * 判断 seq 是否属于“上一轮接收窗口”。
 * 这种帧通常是旧重复帧：不能再次交付，但需要重发 ACK。
 */
static int in_previous_window(unsigned char seq)
{
    unsigned char low = (frame_expected + MAX_SEQ + 1 - WINDOW_SIZE) % (MAX_SEQ + 1);

    return between(low, seq, frame_expected);
}

/*
 * 给帧尾追加 CRC，然后交给实验平台发送。
 * 发送后把 phl_ready 置 0，等待下一次 PHYSICAL_LAYER_READY。
 */
static void put_frame(unsigned char *frame, int len)
{
    *(unsigned int *)(frame + len) = crc32(frame, len);
    send_frame(frame, len + 4);
    phl_ready = 0;
}

/* 发送逐帧 ACK，表示 ack_nr 这一帧已经收到。 */
static void send_ack_frame(unsigned char ack_nr)
{
    struct FRAME s;

    s.kind = FRAME_ACK;
    s.ack = ack_nr;

    dbg_frame("Send ACK  %d\n", s.ack);

    put_frame((unsigned char *)&s, 2);
}

/*
 * 发送 NAK，请求对方重传当前缺失的帧。
 * 发出后 no_nak 置 0，避免在缺口补齐前重复发送相同 NAK。
 */
static void send_nak_frame(unsigned char nak_nr)
{
    struct FRAME s;

    s.kind = FRAME_NAK;
    s.ack = nak_nr;

    dbg_frame("Send NAK  %d\n", s.ack);

    put_frame((unsigned char *)&s, 2);
}

/*
 * 发送一个数据帧，并启动该帧自己的定时器。
 * DATA 帧中也会顺便捎带当前接收端最近一次按序接收进度。
 */
static void send_data_frame(unsigned char frame_nr)
{
    struct FRAME s;

    s.kind = FRAME_DATA;
    s.seq = frame_nr;
    /* ACK 字段回的是最近一个按序收到的序号。 */
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
    memcpy(s.data, out_buf[frame_nr], PKT_LEN);

    dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short *)s.data);

    put_frame((unsigned char *)&s, 3 + PKT_LEN);
    start_timer(frame_nr, DATA_TIMER);
}

/*
 * 标记单帧已确认，并尽量向前滑动发送窗口。
 * SR 的 ACK 是逐帧的，因此某一帧确认后要先记在 acked[] 中，
 * 只有左边界开始连续的一段都确认了，窗口才真正前移。
 */
static void mark_acked(unsigned char seq)
{
    if (between(ack_expected, seq, next_frame_to_send) && !acked[seq]) {
        acked[seq] = 1;
        stop_timer(seq);
    }

    while (nbuffered > 0 && acked[ack_expected]) {
        acked[ack_expected] = 0;
        nbuffered--;
        inc(&ack_expected);
    }
}

int main(int argc, char **argv)
{
    int event, arg;
    struct FRAME f;
    int len = 0;

    protocol_init(argc, argv);
    lprintf("Selective Repeat sliding window, build: " __DATE__"  "__TIME__"\n");
    lprintf("MAX_SEQ=%d, WINDOW_SIZE=%d\n", MAX_SEQ, WINDOW_SIZE);

    disable_network_layer();

    for (;;) {
        event = wait_for_event(&arg);

        switch (event) {
        case NETWORK_LAYER_READY:
            /*
             * 发送窗口未满时，从网络层取一个新分组：
             * 1. 写入发送缓存 out_buf[]
             * 2. 清除该序号的 ACK 标记
             * 3. 发送新帧并推进 next_frame_to_send
             */
            get_packet(out_buf[next_frame_to_send]);
            acked[next_frame_to_send] = 0;
            nbuffered++;
            send_data_frame(next_frame_to_send);
            inc(&next_frame_to_send);
            break;

        case PHYSICAL_LAYER_READY:
            /* 物理层允许再发一帧。 */
            phl_ready = 1;
            break;

        case FRAME_RECEIVED:
            /* 每次只取一帧，根据 kind 决定后续处理逻辑。 */
            len = recv_frame((unsigned char *)&f, sizeof f);
            if (len < 5 || crc32((unsigned char *)&f, len) != 0) {
                dbg_event("**** Receiver Error, Bad CRC Checksum\n");
                /* CRC 错误时无法信任该帧内容，只能请求重传当前最早缺失帧。 */
            
                send_nak_frame(frame_expected);
                break;
            }

            switch (f.kind) {
            case FRAME_ACK:
                dbg_frame("Recv ACK  %d\n", f.ack);
                /* ACK 只确认一个具体序号，不是累计确认。 */
                mark_acked(f.ack);
                break;

            case FRAME_NAK:
                dbg_frame("Recv NAK  %d\n", f.ack);
                /* NAK 明确指出缺失帧，因此只重传这一帧。 */
                if (between(ack_expected, f.ack, next_frame_to_send) && !acked[f.ack])
                    send_data_frame(f.ack);
                break;

            case FRAME_DATA:
                dbg_frame("Recv DATA %d %d, ID %d\n", f.seq, f.ack, *(short *)f.data);

                /*
                 * DATA 帧除了携带数据，还捎带了对方的 ACK。
                 * 因此收到 DATA 时，本机要同时更新：
                 * 1. 发送端窗口状态（处理 f.ack）
                 * 2. 接收端窗口状态（处理 f.seq / f.data）
                 */
                mark_acked(f.ack);

                if (in_receive_window(f.seq)) {
                    /*
                     * 窗口内帧可以接收：
                     * 1. 如果不是左边界 frame_expected，说明前面有缺口，必要时发 NAK
                     * 2. 若该帧首次到达，则写入缓存并标记 arrived[]
                     * 3. 立即回 ACK(f.seq)，告诉对方这帧已收到
                     */
                    if (f.seq != frame_expected )
                        send_nak_frame(frame_expected);

                    if (!arrived[f.seq]) {
                        arrived[f.seq] = 1;
                        memcpy(in_buf[f.seq], f.data, len - 7);
                    }

                    send_ack_frame(f.seq);

                    /*
                     * 只要左边界 frame_expected 已经到达，就可以持续交付：
                     * 这一步会把缓存中的连续帧依次交给网络层，
                     * 同时推进 frame_expected 和 too_far。
                     */
                    while (arrived[frame_expected]) {
                        put_packet(in_buf[frame_expected], len - 7);
                        arrived[frame_expected] = 0;
                        
                        inc(&frame_expected);
                        inc(&too_far);
                    }
                } else if (in_previous_window(f.seq)) {
                    /*
                     * 上一窗口的旧重复帧不再交付。
                     * 一般是 ACK 丢失后，对方又重发了旧帧，因此这里只重复 ACK。
                     */
                    send_ack_frame(f.seq);
                }
                break;
            }
            break;

        case DATA_TIMEOUT:
            dbg_event("---- DATA %d timeout\n", arg);
            /* SR 的超时策略：谁超时就只重传谁。 */
            if (!acked[arg])
                send_data_frame((unsigned char)arg);
            break;
        }

        /*
         * 只有同时满足两件事才允许网络层继续交付新分组：
         * 1. 发送窗口未满
         * 2. 物理层当前允许发送
         */
        if (nbuffered < WINDOW_SIZE && phl_ready)
            enable_network_layer();
        else
            disable_network_layer();
    }
}

#include "shared.h"

void run_diner(SharedState *s, int id) {
    sem_t *mx = sem_open(SEM_MUTEX, 0);
    sem_t *a3 = sem_open(SEM_ALL3,  0);
    sem_t *dn = sem_open(SEM_DONE,  0);
    sem_t *sp = sem_open(SEM_SPACE, 0);
    sem_t *up = sem_open(SEM_UPDATE, 0);

    while (!s->done) {
        /* Chờ waiter báo có đủ 3 loại trên khay */
        sem_wait(a3);
        if (s->done) break; /* Nếu đã phục vụ đủ số lượng chung, thoát */

        /* Lock để double-check — tránh race condition */
        sem_wait(mx);

        if (s->tray[0] > 0 &&
            s->tray[1] > 0 &&
            s->tray[2] > 0 &&
            !s->done) {

            /* Vẫn đủ 3 loại — lấy đúng 1P + 1C + 1D */
            s->tray[0]--;
            s->tray[1]--;
            s->tray[2]--;
            s->tray_total -= 3;
            s->served++;

            printf("[Diner %d] Lay 1P + 1C + 1D -> XONG | Phuc vu: %d/%d\n",
                   id, s->served, s->K);
            fflush(stdout);

            /* Kiểm tra điều kiện kết thúc */
            if (s->served >= s->K) {
                s->done = 1;
                sem_post(up); /* Đánh thức waiter để kết thúc */
                sem_post(sp); /* Đánh thức chef (nếu đang chờ space) để kết thúc */
                /* Đánh thức tất cả các diner khác đang chờ để họ thoát */
                for (int i = 0; i < s->K; i++) {
                    sem_post(a3);
                }
            }

            sem_post(mx);

            /* Trả lại 3 slot trên khay cho chef */
            sem_post(sp);
            sem_post(sp);
            sem_post(sp);

            /* Báo waiter đã lấy xong */
            sem_post(dn);

            /* Bỏ lệnh break ở đây để Diner quay lại vòng lặp, chờ lấy món tiếp theo */

        } else {
            /*
             * Race condition: tray không còn đủ
             * Trả lại signal để diner khác còn cơ hội
             */
            sem_post(mx);
            sem_post(a3);
        }
    }

    sem_close(mx);
    sem_close(a3);
    sem_close(dn);
    sem_close(sp);
    sem_close(up);
}

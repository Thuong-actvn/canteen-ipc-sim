#include "shared.h"

void run_waiter(SharedState *s) {
    sem_t *mx = sem_open(SEM_MUTEX, 0);
    sem_t *a3 = sem_open(SEM_ALL3,  0);
    sem_t *dn = sem_open(SEM_DONE,  0);
    sem_t *up = sem_open(SEM_UPDATE, 0);
    sem_t *chk = sem_open(SEM_CHECKED, 0);

    while (!s->done) {
        sem_wait(up); // Chờ có đĩa mới đưa lên khay hoặc có tín hiệu kết thúc
        if (s->done) {
            sem_post(chk); // Giải phóng Chef nếu Chef đang chờ
            break;
        }

        sem_wait(mx);

        /* Kiểm tra đủ 3 loại trên khay */
        int has_all = s->tray[0] > 0 &&
                      s->tray[1] > 0 &&
                      s->tray[2] > 0;

        /* Xác định trạng thái để in log */
        const char *status;
        if      (s->tray_total == s->M) status = "DAY KHAY";
        else if (has_all)               status = "DU 3 MON";
        else                            status = "CHO THEM";

        printf("[Waiter] Khay: P=%d C=%d D=%d (%d dia) | %s\n",
               s->tray[0], s->tray[1], s->tray[2],
               s->tray_total, status);
        fflush(stdout);

        if (has_all) {
            /* 
             * Báo cho 1 diner ra lấy món.
             * Đánh thức từng diner một để đảm bảo tuần tự nghiêm ngặt,
             * tránh tình trạng dồn tín hiệu SEM_DONE làm sai lệch lock-step.
             */
            sem_post(a3);
            sem_post(mx);

            /* Chờ diner đó xác nhận đã lấy xong */
            sem_wait(dn);
            sem_post(chk); // Đã phục vụ xong, cho phép Chef tiếp tục đưa món
        } else {
            sem_post(mx);
            sem_post(chk); // Khay chưa đủ món, cho phép Chef tiếp tục đưa món
        }
    }

    sem_close(mx);
    sem_close(a3);
    sem_close(dn);
    sem_close(up);
    sem_close(chk);
}

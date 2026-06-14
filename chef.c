#include "shared.h"

void run_chef(SharedState *s) {
    const char *names[] = FOOD_NAMES;
    sem_t *mx = sem_open(SEM_MUTEX, 0);
    sem_t *sp = sem_open(SEM_SPACE, 0);
    sem_t *up = sem_open(SEM_UPDATE, 0);
    sem_t *chk = sem_open(SEM_CHECKED, 0);
    int luot = 1;

    while (!s->done) {

        /* ══ BƯỚC 1: Nấu thêm 1 món nếu chưa đạt K (theo produced) ══ */
        sem_wait(mx);

        /* Kiểm tra còn loại nào chưa nấu đủ K */
        int can_cook = 0;
        for (int i = 0; i < 3; i++)
            if (s->produced[i] < s->K) { can_cook = 1; break; }

        /* Kho trống nhưng còn nấu được → quay lại nấu tiếp */
        int has_stock = 0;
        for (int i = 0; i < 3; i++)
            if (s->inventory[i] > 0) { has_stock = 1; break; }
            
        if (!has_stock && !can_cook) {
            sem_post(mx);
            sem_post(up);
            break;
        }

        printf("\n--- Luot %d ---\n", luot++);

        if (can_cook) {
            /* Chọn ngẫu nhiên loại chưa đạt K để nấu */
            int mon;
            do { mon = rand() % 3; } while (s->produced[mon] >= s->K);
            s->inventory[mon]++;
            s->produced[mon]++;

            printf("[Chef]   Nau mon %s        | Kho: P=%d C=%d D=%d\n",
                   names[mon],
                   s->inventory[0], s->inventory[1], s->inventory[2]);
            fflush(stdout);
        }

        /* Cập nhật lại has_stock sau khi nấu */
        has_stock = 0;
        for (int i = 0; i < 3; i++)
            if (s->inventory[i] > 0) { has_stock = 1; break; }

        if (!has_stock) {
            sem_post(mx);
            continue;
        }

        sem_post(mx);

        /* ══ BƯỚC 2: Đưa 1 đĩa từ kho lên khay waiter ══ */

        /* Chờ có slot trống — block nếu tray_total == M (khay đầy) */
        sem_wait(sp);

        /* Kiểm tra done sau khi unblock */
        if (s->done) {
            sem_post(up);
            break;
        }

        sem_wait(mx);

        /* Chọn ngẫu nhiên loại có trong kho để đưa */
        int has_any = 0;
        for (int i = 0; i < 3; i++)
            if (s->inventory[i] > 0) { has_any = 1; break; }

        if (!has_any) {
            sem_post(mx);
            sem_post(sp);  /* trả lại slot không dùng */
            continue;
        }

        int give;
        do { give = rand() % 3; } while (s->inventory[give] == 0);

        s->inventory[give]--;
        s->tray[give]++;
        s->tray_total++;

        printf("[Chef]   Dua mon %s        | Kho: P=%d C=%d D=%d"
               " | Khay: P=%d C=%d D=%d (%d dia)\n",
               names[give],
               s->inventory[0], s->inventory[1], s->inventory[2],
               s->tray[0], s->tray[1], s->tray[2], s->tray_total);
        fflush(stdout);
        sem_post(mx);
        
        sem_post(up);  /* Báo cho waiter có khay cập nhật mới */
        
        /* 
         * BƯỚC 3: Đảm bảo Waiter đã xử lý xong đĩa vừa đưa trước khi đưa tiếp.
         * Điều này giúp loại bỏ tranh chấp Mutex (starvation) và đảm bảo tính tuần tự 100%.
         */
        sem_wait(chk); 
    }

    sem_close(mx);
    sem_close(sp);
    sem_close(up);
    sem_close(chk);
}

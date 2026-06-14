#ifndef SHARED_H
#define SHARED_H

#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>

/* ── Tên IPC (dùng chung toàn bộ chương trình) ── */
#define SHM_NAME    "/canteen_shm"
#define SEM_MUTEX   "/canteen_mutex"
#define SEM_SPACE   "/canteen_space"
#define SEM_ALL3    "/canteen_all3"
#define SEM_DONE    "/canteen_done"
#define SEM_UPDATE  "/canteen_update"
#define SEM_CHECKED "/canteen_check"

/* ── Index loại món ── */
#define FOOD_P      0   /* Khai vị   */
#define FOOD_C      1   /* Món chính */
#define FOOD_D      2   /* Bánh ngọt */
#define FOOD_NAMES  {"P", "C", "D"}

/* ── Cấu trúc dữ liệu dùng chung giữa tất cả tiến trình ── */
typedef struct {
    int inventory[3];  /* Kho nội bộ của chef               */
    int produced[3];   /* Tổng số đã nấu mỗi loại (≤ K)    */
    int tray[3];       /* Khay waiter đang cầm              */
    int tray_total;    /* Tổng đĩa trên khay                */
    int served;        /* Số diner đã được phục vụ xong     */
    int K;             /* Số diner                          */
    int M;             /* = 2K+1, giới hạn khay             */
    int done;          /* Flag kết thúc: 0=chạy, 1=xong     */
} SharedState;

/* ── Prototypes ── */
SharedState* init_ipc(int K);
void         cleanup_ipc(SharedState *state);
void         run_chef(SharedState *state);
void         run_waiter(SharedState *state);
void         run_diner(SharedState *state, int id);

#endif /* SHARED_H */

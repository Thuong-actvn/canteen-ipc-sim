#include "shared.h"

SharedState* init_ipc(int K) {
    /* ── Tạo và map shared memory ── */
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }
    if (ftruncate(fd, sizeof(SharedState)) == -1) {
        perror("ftruncate");
        exit(1);
    }
    SharedState *s = mmap(NULL, sizeof(SharedState),
                          PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (s == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    close(fd);

    /* ── Khởi tạo các trường ── */
    s->K          = K;
    s->M          = 2 * K + 1;
    s->served     = 0;
    s->tray_total = 0;
    s->done       = 0;
    for (int i = 0; i < 3; i++) s->tray[i] = 0;

    /*
     * Khởi tạo inventory ngẫu nhiên:
     *   Mỗi loại P, C, D độc lập random từ 0 đến K
     *   Chef sẽ nấu thêm từng món cho đến khi P = C = D = K
     */
    srand(time(NULL));
    for (int i = 0; i < 3; i++) {
        s->inventory[i] = rand() % (K + 1);  /* 0..K */
        s->produced[i]  = s->inventory[i];   /* đã nấu = kho ban đầu */
    }

    /* ── Tạo 4 semaphore ── */
    if (sem_open(SEM_MUTEX, O_CREAT, 0666, 1) == SEM_FAILED) {
        perror("sem_open mutex");
        exit(1);
    }
    if (sem_open(SEM_SPACE, O_CREAT, 0666, s->M) == SEM_FAILED) {
        perror("sem_open space");
        exit(1);
    }
    if (sem_open(SEM_ALL3, O_CREAT, 0666, 0) == SEM_FAILED) {
        perror("sem_open all3");
        exit(1);
    }
    if (sem_open(SEM_DONE, O_CREAT, 0666, 0) == SEM_FAILED) {
        perror("sem_open done");
        exit(1);
    }
    if (sem_open(SEM_UPDATE, O_CREAT, 0666, 0) == SEM_FAILED) {
        perror("sem_open update");
        exit(1);
    }
    if (sem_open(SEM_CHECKED, O_CREAT, 0666, 0) == SEM_FAILED) {
        perror("sem_open checked");
        exit(1);
    }

    return s;
}

void cleanup_ipc(SharedState *state) {
    munmap(state, sizeof(SharedState));
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_SPACE);
    sem_unlink(SEM_ALL3);
    sem_unlink(SEM_DONE);
    sem_unlink(SEM_UPDATE);
    sem_unlink(SEM_CHECKED);
}

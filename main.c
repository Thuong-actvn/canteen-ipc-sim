#include "shared.h"
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s K\n", argv[0]);
        return 1;
    }
    int K = atoi(argv[1]);
    if (K < 1) {
        fprintf(stderr, "Lỗi: K phải >= 1\n");
        return 1;
    }

    /* Khởi tạo IPC trước khi fork */
    SharedState *state = init_ipc(K);
    printf("[Main] Canteen bắt đầu: K=%d, M=%d\n", K, state->M);
    printf("[Main] Kho ban đầu: P=%d C=%d D=%d\n\n",
           state->inventory[0], state->inventory[1], state->inventory[2]);

    pid_t pid_chef, pid_waiter;
    pid_t pid_diners[K];

    /* Fork Chef */
    pid_chef = fork();
    if (pid_chef == 0) { run_chef(state); exit(0); }

    /* Fork Waiter */
    pid_waiter = fork();
    if (pid_waiter == 0) { run_waiter(state); exit(0); }

    /* Fork K Diners */
    for (int i = 0; i < K; i++) {
        pid_diners[i] = fork();
        if (pid_diners[i] == 0) { run_diner(state, i); exit(0); }
    }

    /* Chờ TẤT CẢ con kết thúc trước khi cleanup */
    waitpid(pid_chef,   NULL, 0);
    waitpid(pid_waiter, NULL, 0);
    for (int i = 0; i < K; i++)
        waitpid(pid_diners[i], NULL, 0);

    printf("\n[Main] Tất cả tiến trình kết thúc. Dọn dẹp IPC...\n");
    cleanup_ipc(state);
    return 0;
}

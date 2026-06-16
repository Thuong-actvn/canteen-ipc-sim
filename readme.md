# Canteen IPC Simulation

Dự án này là một bài tập mô phỏng đồng bộ hóa đa tiến trình (Inter-Process Communication - IPC) trong môi trường Linux, sử dụng **Bộ nhớ chia sẻ (Shared Memory)** và **POSIX Semaphore**.

## Bài toán Canteen
Hệ thống mô phỏng 3 vai trò hoạt động song song bằng các tiến trình (`fork()`):
- **Chef (Đầu bếp)**: Liên tục nấu các món (P - Khai vị, C - Món chính, D - Bánh ngọt) và đặt lên khay. Khay chứa có sức chứa giới hạn (`M = 2K + 1`).
- **Waiter (Phục vụ)**: Giám sát khay. Khi khay có đủ combo 3 món (1P + 1C + 1D), Waiter sẽ báo hiệu gọi một Diner ra lấy thức ăn.
- **Diner (Thực khách)**: Có `K` tiến trình Diner chờ thức ăn. Khi Waiter gọi, Diner được đánh thức, lấy đi 3 món khỏi khay và báo lại để hệ thống tiếp tục.

Chương trình sẽ kết thúc an toàn (Graceful Shutdown) sau khi phục vụ đủ `K` phần ăn.

## Kiến trúc IPC
Hệ thống sử dụng các Semaphore để tránh tình trạng Race Condition và Deadlock:
- `SEM_MUTEX`: Đảm bảo độc quyền khi truy cập vùng nhớ chia sẻ (Shared State).
- `SEM_SPACE`: Quản lý sức chứa của khay, block Chef khi khay đầy.
- `SEM_ALL3`: Đánh thức Diner khi có đủ 3 món trên khay.
- `SEM_DONE`: Diner báo cho Waiter sau khi lấy xong.
- `SEM_UPDATE` / `SEM_CHECKED`: Đồng bộ luồng (Lock-step) giữa Chef và Waiter, đảm bảo các log và hành động chạy theo trình tự không bị rối.

## Cấu trúc Mã nguồn
- `shared.h`: Chứa định nghĩa cấu trúc `SharedState` và tên Semaphore.
- `main.c`: Khởi tạo IPC, tạo các tiến trình con bằng `fork()` và đợi (`waitpid`) chúng kết thúc để dọn dẹp bộ nhớ.
- `chef.c`: Chứa vòng lặp logic của tiến trình Đầu bếp.
- `waiter.c`: Chứa vòng lặp logic của tiến trình Phục vụ.
- `diner.c`: Chứa vòng lặp logic của tiến trình Thực khách.

## Cách biên dịch và chạy

Biên dịch bằng GCC:
```bash
gcc *.c -o canteen -pthread -lrt
```

Chạy chương trình với tham số `K` (ví dụ `K=1` cho 1 suất ăn):
```bash
./canteen 1
```

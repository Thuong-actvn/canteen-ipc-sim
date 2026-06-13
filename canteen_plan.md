# Kế hoạch thực hiện: Canteen Simulation

> **Môn:** Linux Kernel Programming
> **Bài toán:** Mô phỏng quá trình phục vụ thức ăn ở canteen với 3 đối tượng: Đầu bếp, Người phục vụ, Thực khách

---

## Mục lục

1. [Yêu cầu bài toán (chuẩn hóa)](#1-yêu-cầu-bài-toán-chuẩn-hóa)
2. [Giai đoạn 1 — Lý thuyết nền](#2-giai-đoạn-1--lý-thuyết-nền)
3. [Giai đoạn 2 — Thiết kế cấu trúc](#3-giai-đoạn-2--thiết-kế-cấu-trúc)
4. [Giai đoạn 3 — Viết code](#4-giai-đoạn-3--viết-code)
5. [Giai đoạn 4 — Test & Debug](#5-giai-đoạn-4--test--debug)
6. [Giai đoạn 5 — Makefile & README](#6-giai-đoạn-5--makefile--readme)

---

## 1. Yêu cầu bài toán (chuẩn hóa)

### 1.1 Tổng quan hệ thống

Chương trình fork ra **3 loại tiến trình** từ main:

```
main
 ├── fork → Chef process      (1 tiến trình)
 ├── fork → Waiter process    (1 tiến trình)
 └── fork → Diner process     (K tiến trình)
```

### 1.2 Các thực thể và vai trò

#### Chef (Đầu bếp)

**Bước 1 — Nấu món (Cooking):**

- Tại thời điểm khởi động, nấu một lượt ngẫu nhiên với ràng buộc mỗi loại `≤ K`
- Mỗi lượt chỉ nấu thêm được **1 món** (P hoặc C hoặc D)
- Tổng tối đa nấu được: **3K đĩa** (= K món P + K món C + K món D)
- Kho nội bộ của chef: `inventory[P]`, `inventory[C]`, `inventory[D]`

**Bước 2 — Đưa món cho Waiter (Serving):**

- Mỗi lần chỉ đưa được **1 đĩa** từ kho cho waiter
- Nếu khay waiter đang có **M = 2K+1 đĩa** → **dừng đưa** và chờ
- Tiếp tục đưa khi khay còn chỗ (`tray_total < M`)

**Log bắt buộc mỗi lượt:**

```
[Chef] Nấu món <P|C|D> | Kho: P=x C=x D=x
[Chef] Đưa món <P|C|D> | Kho: P=x C=x D=x | Khay: P=x C=x D=x (tổng x đĩa)
```

---

#### Waiter (Người phục vụ) — Process riêng bắt buộc

**Vai trò trung gian điều phối:**

- Nhận từng đĩa từ chef, cập nhật khay `tray[P]`, `tray[C]`, `tray[D]`
- Liên tục theo dõi khay:
  - Nếu `tray[P]>0 ∧ tray[C]>0 ∧ tray[D]>0` → **phát tín hiệu** cho các diner
  - Nếu `tray_total == M` → **không nhận thêm**, ưu tiên cho diner lấy trước
- Khi diner lấy xong → giải phóng chỗ trên khay → báo chef tiếp tục

**Log bắt buộc mỗi lượt:**

```
[Waiter] Khay: P=x C=x D=x (tổng x đĩa) | <ĐỦ 3 MÓN | CHỜ THÊM | ĐẦY KHAY>
```

---

#### Diner (Thực khách — K tiến trình)

**Điều kiện lấy món:**

- Chỉ lấy khi waiter có **đồng thời cả 3 loại** P, C, D trên khay
- Nếu thiếu bất kỳ loại nào → **không lấy gì**, tiếp tục chờ

**Khi lấy món:**

- Lấy **đúng 1P + 1C + 1D**
- Đây là cách tối giản thỏa điều kiện "ít nhất 1 mỗi loại", đảm bảo công bằng giữa các diner và hành vi hệ thống hoàn toàn xác định (mỗi lần lấy luôn giảm đúng 3 đĩa)
- Sau khi lấy đủ → coi là **được phục vụ xong**, rời đi

**Log bắt buộc khi rời đi:**

```
[Diner i] Lấy 1P + 1C + 1D → XONG | Phục vụ: x/K
```

---

### 1.3 Ràng buộc bất biến (Invariants)

| # | Ràng buộc | Ý nghĩa |
|---|-----------|---------|
| 1 | `inventory[i] ≤ K` với mọi i | Mỗi loại tối đa K đĩa trong kho |
| 2 | `∑ inventory[i] ≤ 3K` | Tổng kho đúng 3K (đủ cho K diner) |
| 3 | `0 ≤ tray_total ≤ M = 2K+1` | Khay không vượt M |
| 4 | `tray_total == M → tray[i]>0 ∀i` | Pigeonhole: đầy khay → chắc đủ 3 loại |
| 5 | Mỗi diner lấy đúng `1P + 1C + 1D` | Điều kiện hoàn thành, công bằng, xác định |

> **Tại sao Invariant 4 đúng (Pigeonhole Principle)?**
> Khay có `2K+1` đĩa nhưng chỉ có 3 loại, mỗi loại tối đa K đĩa.
> Nếu chỉ có 2 loại thì tối đa `K + K = 2K` đĩa < `2K+1`.
> Vậy **bắt buộc phải có cả 3 loại** khi khay đầy.

---

### 1.4 Luồng hoạt động tổng thể

```
KHỞI TẠO:
  Chef nấu lượt đầu ngẫu nhiên: mỗi loại ≤ K, tổng ≤ 3K
  inventory[P], inventory[C], inventory[D] được set ngẫu nhiên

VÒNG LẶP CHÍNH:
  Chef:   Nấu thêm 1 món vào kho nếu còn thiếu
          → Chờ chỗ trống trên khay (sem_space)
          → Đưa 1 đĩa từ kho lên khay waiter

  Waiter: Theo dõi khay liên tục
          → Nếu đủ 3 loại: báo diner (sem_all3), chờ diner lấy (sem_done)
          → In log trạng thái khay mỗi lượt

  Diner:  Chờ tín hiệu từ waiter (sem_all3)
          → Double-check trong mutex: tray vẫn đủ không?
          → Nếu đủ: lấy đúng 1P+1C+1D, trả 3 slot (sem_space×3), báo waiter
          → Nếu không đủ: trả lại signal (sem_all3), tiếp tục chờ

KẾT THÚC:
  served == K → set done = 1
  Tất cả tiến trình kiểm tra done → thoát vòng lặp
  main: waitpid() tất cả con → cleanup_ipc()
```

---

### 1.5 Cơ chế IPC

| Tài nguyên | Loại | Init | Mục đích |
|------------|------|------|---------|
| `SharedState` | `shm_open + mmap` | — | Lưu inventory, tray, served, done |
| `sem_mutex` | POSIX Named Semaphore | 1 | Bảo vệ critical section trên SharedState |
| `sem_space` | POSIX Named Semaphore | M | Đếm slot trống trên khay |
| `sem_all3` | POSIX Named Semaphore | 0 | Waiter báo diner khi đủ P+C+D |
| `sem_done` | POSIX Named Semaphore | 0 | Diner báo waiter đã lấy xong |

---

### 1.6 Cấu trúc file

```
canteen/
├── Makefile
├── README.txt
├── shared.h          ← SharedState struct, tên IPC, hằng số, prototypes
├── ipc.c             ← init_ipc(), cleanup_ipc()
├── main.c            ← parse K, fork, waitpid, cleanup
├── chef.c            ← run_chef(): nấu + đưa món
├── waiter.c          ← run_waiter(): nhận + kiểm tra + báo hiệu
└── diner.c           ← run_diner(): chờ + lấy đúng 1P+1C+1D + rời đi
```

---

## 2. Giai đoạn 1 — Lý thuyết

### 2.1 Tiến trình (Process) và `fork()`

#### Tiến trình là gì?

Một **tiến trình** là một chương trình đang thực thi, được kernel quản lý thông qua cấu trúc `task_struct`. Mỗi tiến trình có:

- **PID** (Process ID): định danh duy nhất trong hệ thống
- **Không gian địa chỉ ảo riêng**: bao gồm vùng code, stack, heap, data
- **Bảng file descriptor**: quản lý các file/socket/pipe đang mở
- **Trạng thái**: `RUNNING`, `SLEEPING`, `ZOMBIE`, `STOPPED`

Kernel lưu toàn bộ thông tin này trong `task_struct` — còn gọi là **Process Control Block (PCB)**.

#### `fork()` hoạt động như thế nào?

```c
pid_t pid = fork();
// pid == 0  → đang chạy ở process CON  (child)
// pid >  0  → đang chạy ở process CHA  (parent), pid là PID của con
// pid <  0  → fork thất bại (hệ thống hết tài nguyên)
```

Khi `fork()` được gọi, kernel tạo một bản sao gần như hoàn toàn của tiến trình cha:

```
Trước fork():            Sau fork():
┌──────────────┐         ┌──────────────┐   ┌──────────────┐
│   Parent     │         │   Parent     │   │    Child     │
│  pid = 1000  │  ────►  │  pid = 1000  │   │  pid = 1001  │
│  code, heap  │         │  code, heap  │   │  code, heap  │ ← bản sao
│  stack, fd   │         │  stack, fd   │   │  stack, fd   │ ← bản sao
└──────────────┘         └──────────────┘   └──────────────┘
```

**Copy-on-Write (COW):**
Thực tế kernel không sao chép ngay toàn bộ bộ nhớ. Hai tiến trình **chia sẻ các trang vật lý**, chỉ khi một trong hai **ghi vào** thì kernel mới tạo bản sao riêng cho tiến trình đó. Điều này giúp `fork()` rất nhanh dù không gian địa chỉ có thể rất lớn.

**Ngoại lệ quan trọng — `MAP_SHARED`:**
Vùng nhớ được map với `MAP_SHARED` **không bao giờ được copy** theo cơ chế COW. Cả cha lẫn con đều trỏ vào **cùng một trang vật lý** mãi mãi. Đây là nền tảng để các tiến trình giao tiếp qua shared memory.

```c
SharedState *state = mmap(..., MAP_SHARED, ...);
fork();  // fork xảy ra sau khi đã mmap
// Từ đây: parent, chef, waiter, diner đều nhìn thấy cùng state
// Thay đổi ở bất kỳ tiến trình nào → tất cả đều thấy ngay
```

#### `waitpid()` — chờ tiến trình con kết thúc

```c
#include <sys/wait.h>

pid_t waitpid(pid_t pid, int *status, int options);
// pid > 0  : chờ đúng tiến trình có PID đó
// status   : lưu trạng thái kết thúc (truyền NULL nếu không cần)
// options=0: block cho đến khi con kết thúc
```

Thứ tự đúng trong `main.c`:

```c
// Chờ TẤT CẢ con kết thúc
waitpid(pid_chef,   NULL, 0);
waitpid(pid_waiter, NULL, 0);
for (int i = 0; i < K; i++)
    waitpid(pid_diners[i], NULL, 0);

// CHỈ SAU KHI tất cả con đã chết → mới cleanup IPC
cleanup_ipc(state);
```

> **Zombie process:**
> Nếu cha không gọi `waitpid()`, tiến trình con sau khi chết trở thành **zombie** — entry vẫn tồn tại trong bảng tiến trình của kernel dù không còn chạy. `waitpid()` "thu hồi" zombie này và giải phóng tài nguyên.

> **Cảnh báo nghiêm trọng:**
> Nếu gọi `cleanup_ipc()` trước khi con chết → `shm_unlink()` xóa tên shared memory trong khi con vẫn đang dùng → **undefined behavior**, chương trình crash hoặc kết quả hoàn toàn sai.

---

### 2.2 Shared Memory POSIX — `shm_open()` + `mmap()`

#### Tại sao cần shared memory?

Sau `fork()`, mỗi tiến trình có không gian địa chỉ ảo riêng. Biến cục bộ trong cha **không tự động chia sẻ** với con sau khi fork. Cần một cơ chế để nhiều tiến trình đọc/ghi cùng một vùng dữ liệu — đó là **shared memory**.

#### Cơ chế hoạt động ở tầng vật lý

```
Virtual Address Space:              Physical Memory:

Chef process:                       ┌─────────────────┐
  state → 0xA000  ─────────────────►│                 │
                                    │   SharedState   │
Waiter process:                     │  inventory[]    │  ← 1 trang vật lý
  state → 0xB000  ─────────────────►│  tray[]         │    duy nhất
                                    │  served, done   │
Diner 0 process:                    │                 │
  state → 0xC000  ─────────────────►└─────────────────┘
```

Địa chỉ ảo khác nhau ở mỗi process, nhưng đều trỏ vào **cùng một trang vật lý**. Mọi thay đổi hiển thị ngay lập tức với tất cả tiến trình.

#### API đầy đủ và giải thích từng bước

```c
#include <sys/mman.h>   // mmap, munmap
#include <fcntl.h>      // O_CREAT, O_RDWR
#include <unistd.h>     // ftruncate, close

// Bước 1: Tạo/mở đối tượng shared memory theo tên
// Tên bắt đầu bằng "/" → được lưu trong /dev/shm/
// O_CREAT: tạo mới nếu chưa có
// O_RDWR:  mở để đọc và ghi
// 0666:    permission (rw-rw-rw-)
int fd = shm_open("/canteen_shm", O_CREAT | O_RDWR, 0666);

// Bước 2: Đặt kích thước vùng nhớ
// (shm object mới tạo có size = 0, phải set trước khi mmap)
ftruncate(fd, sizeof(SharedState));

// Bước 3: Map vào không gian địa chỉ của process
// NULL         : kernel tự chọn địa chỉ ảo
// PROT_READ|WRITE: có thể đọc và ghi
// MAP_SHARED   : thay đổi hiển thị ngay với process khác (quan trọng!)
// fd, 0        : map từ đầu file shm
SharedState *state = mmap(NULL, sizeof(SharedState),
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED, fd, 0);

// Bước 4: Đóng fd — mapping vẫn tồn tại độc lập với fd
close(fd);

// Bước 5: Dùng như struct C bình thường — mọi write đều ngay lập tức visible
state->served++;
state->tray[0] = 3;

// Bước 6: Hủy mapping trong mỗi process khi xong
munmap(state, sizeof(SharedState));

// Bước 7: Xóa tên khỏi hệ thống — chỉ gọi 1 lần trong main sau waitpid
shm_unlink("/canteen_shm");
```

#### Vòng đời của shared memory trong bài

```
main:   shm_open(O_CREAT) → ftruncate → mmap → khởi tạo SharedState
  │
  ├─ fork() chef, waiter, K diners
  │     Các process con: shm_open(O_RDWR) → mmap → dùng state
  │
  │  ... hoạt động song song ...
  │
  ├─ waitpid() tất cả con → tất cả con đã thoát
  │
  └─ munmap() → shm_unlink()   ← chỉ ở đây mới cleanup
```

> **Lưu ý thực tế:**
> Shared memory POSIX tồn tại trong `/dev/shm/`. Nếu chương trình crash trước khi gọi `shm_unlink()`, file vẫn còn ở đó đến khi reboot hoặc xóa thủ công:
> ```bash
> ls /dev/shm/          # xem có canteen_shm không
> rm /dev/shm/canteen_shm   # xóa thủ công nếu cần
> ```

---

### 2.3 POSIX Named Semaphore

#### Semaphore là gì?

Semaphore là một **biến đếm nguyên không âm** được kernel quản lý, dùng để đồng bộ hóa giữa các tiến trình/luồng. Chỉ có 2 thao tác **nguyên tử** (atomic):

- **P — `sem_wait()`:** Giảm giá trị 1. Nếu giá trị đang = 0 thì **block** cho đến khi > 0
- **V — `sem_post()`:** Tăng giá trị 1. Nếu có process đang chờ thì **thức dậy đúng 1** trong số đó

```
Semaphore value = 3:    [■■■]
sem_wait() → value = 2: [■■ ]   không block
sem_wait() → value = 1: [■  ]   không block
sem_wait() → value = 0: [   ]   không block
sem_wait() → BLOCK      [   ]   process đưa vào sleep queue!
sem_wait() → BLOCK      [   ]   thêm 1 process nữa vào sleep queue
sem_post() → value = 1: [■  ]   thức dậy 1 process từ sleep queue
```

#### Named vs Unnamed semaphore

| | Named (`sem_open`) | Unnamed (`sem_init`) |
|---|---|---|
| Dùng xuyên process | Có | Không |
| Dùng xuyên thread | Có | Có |
| Tên định danh | `/tên` trong `/dev/shm/` | Không có tên |
| Phù hợp bài này | **Bắt buộc** (do dùng fork) | Không phù hợp |

#### API đầy đủ

```c
#include <semaphore.h>
#include <fcntl.h>

// ── Tạo semaphore (gọi trong main TRƯỚC khi fork) ──
// "/tên"  : tên định danh, lưu trong /dev/shm/sem.tên
// O_CREAT : tạo mới nếu chưa có
// 0666    : permission
// 1       : giá trị khởi tạo
sem_t *mutex = sem_open("/canteen_mutex", O_CREAT, 0666, 1);

// ── Mở semaphore đã tồn tại (gọi trong process con sau fork) ──
sem_t *mutex = sem_open("/canteen_mutex", 0);

// ── P: giảm 1, block nếu = 0 ──
sem_wait(mutex);

// ── Thử P không block (trả về -1 thay vì block nếu = 0) ──
int ret = sem_trywait(mutex);  // ret == -1 → không lấy được

// ── V: tăng 1, thức dậy 1 process đang chờ ──
sem_post(mutex);

// ── Đọc giá trị hiện tại (dùng để debug) ──
int val;
sem_getvalue(mutex, &val);
printf("sem value = %d\n", val);

// ── Đóng trong mỗi process khi xong ──
sem_close(mutex);

// ── Xóa tên khỏi hệ thống (chỉ gọi 1 lần trong main) ──
sem_unlink("/canteen_mutex");
```

#### Cơ chế bên trong kernel

```
sem_wait() khi value = 0:
  1. Process gọi sem_wait() → kernel nhận syscall
  2. Kernel kiểm tra value == 0
  3. Kernel đưa process vào WAIT QUEUE của semaphore
  4. Process chuyển trạng thái: RUNNING → SLEEPING
  5. Kernel context switch → chạy process khác

sem_post():
  1. Process gọi sem_post() → kernel nhận syscall
  2. Kernel tăng value lên 1
  3. Nếu WAIT QUEUE không rỗng:
       → Lấy 1 process ra khỏi queue
       → Đặt trạng thái READY (không giảm value, vì process kia "tiêu thụ" luôn)
  4. Scheduler chạy process đó khi đến lượt
```

#### 4 semaphore trong bài — mục đích và cách dùng

**`sem_mutex` (init = 1) — Mutual Exclusion:**

```c
// Đảm bảo chỉ 1 tiến trình đọc/ghi SharedState tại 1 thời điểm
sem_wait(sem_mutex);    // vào critical section
    state->tray[mon]++;
    state->tray_total++;
    state->served++;
sem_post(sem_mutex);    // ra khỏi critical section
```

**`sem_space` (init = M) — Đếm slot trống trên khay:**

```c
// Chef: chờ có chỗ trước khi đưa đĩa lên khay
sem_wait(sem_space);    // M→M-1; block nếu tray_total đã == M (khay đầy)
// ... đưa 1 đĩa lên khay ...

// Diner: trả lại đúng 3 slot sau khi lấy 1P+1C+1D
sem_post(sem_space);    // +1
sem_post(sem_space);    // +1
sem_post(sem_space);    // +1  → tổng cộng +3
```

**`sem_all3` (init = 0) — Tín hiệu có đủ 3 loại:**

```c
// Waiter: post cho từng diner chưa được phục vụ
int remaining = s->K - s->served;
for (int i = 0; i < remaining; i++)
    sem_post(sem_all3);    // thức dậy từng diner

// Diner: chờ được thức dậy
sem_wait(sem_all3);        // ngủ cho đến khi waiter post

// Diner (khi fail double-check): trả lại signal
sem_post(sem_all3);        // không "nuốt" signal của diner khác
```

**`sem_done` (init = 0) — Xác nhận diner đã lấy xong:**

```c
// Diner: báo waiter sau khi lấy món thành công
sem_post(sem_done);

// Waiter: chờ xác nhận có người đã lấy
sem_wait(sem_done);        // biết chắc tray đã được cập nhật
```

---

### 2.4 Critical Section và Race Condition

#### Race condition là gì?

Xảy ra khi **nhiều tiến trình cùng truy cập và sửa đổi dữ liệu chung** mà không có đồng bộ hóa. Kết quả phụ thuộc vào thứ tự thực thi — hoàn toàn không xác định được.

**Ví dụ race condition trong bài — nếu không có mutex:**

```c
// Diner 0 và Diner 1 cùng chạy đồng thời, KHÔNG có mutex:

// Diner 0:                          // Diner 1:
read  served = 2                     read  served = 2    ← đọc cùng lúc!
served = 2 + 1 = 3
write served = 3                     served = 2 + 1 = 3
                                     write served = 3    ← ghi đè!
// Kết quả: served = 3 thay vì 4 → sai hoàn toàn!
```

Tại tầng máy, `served++` thực ra là **3 lệnh assembly**:
```
LOAD  R1, [served]     ; đọc giá trị vào register
ADD   R1, R1, 1        ; cộng 1
STORE [served], R1     ; ghi lại
```
Kernel có thể ngắt và chuyển tiến trình **giữa bất kỳ 2 lệnh nào**, gây race.

#### Critical Section

Là đoạn code truy cập tài nguyên dùng chung, phải đảm bảo **chỉ 1 tiến trình thực thi tại một thời điểm**.

```c
// Cấu trúc chuẩn — dùng sem_mutex như binary semaphore
sem_wait(sem_mutex);    // ── vào critical section ──
    state->tray[0]--;
    state->tray[1]--;
    state->tray[2]--;
    state->tray_total -= 3;
    state->served++;
sem_post(sem_mutex);    // ── ra khỏi critical section ──
```

**Quy tắc vàng:** Mọi thao tác đọc/ghi trên `SharedState` đều phải nằm trong `sem_mutex`.

#### Double-Check Pattern (Kiểm tra lại sau khi lock)

**Vấn đề:** `sem_all3` được post `remaining` lần, nhiều diner cùng thức dậy, nhưng tray có thể không còn đủ cho tất cả.

```
Tình huống: K=3, tray có P=1, C=1, D=1
Waiter post sem_all3 × 3 (cho 3 diner chưa được phục vụ)

Diner 0: sem_wait(all3) → thức → lock → OK → lấy → tray: P=0,C=0,D=0
Diner 1: sem_wait(all3) → thức → lock → P=0! → KHÔNG LẤY
Diner 2: sem_wait(all3) → thức → lock → P=0! → KHÔNG LẤY
→ Diner 1, 2 phải trả lại signal và chờ lượt sau
```

Pattern xử lý đúng:

```c
sem_wait(sem_all3);              // thức dậy khi waiter báo
sem_wait(sem_mutex);             // lock để kiểm tra an toàn

if (s->tray[0] > 0 &&
    s->tray[1] > 0 &&
    s->tray[2] > 0 &&
    !s->done) {
    // VẪN ĐỦ 3 LOẠI → lấy món
    s->tray[0]--;
    s->tray[1]--;
    s->tray[2]--;
    s->tray_total -= 3;
    s->served++;
    if (s->served >= s->K) s->done = 1;

    sem_post(sem_mutex);
    sem_post(sem_space);  // trả slot P
    sem_post(sem_space);  // trả slot C
    sem_post(sem_space);  // trả slot D
    sem_post(sem_done);   // báo waiter
    break;                // diner này xong

} else {
    // KHÔNG ĐỦ → trả lại signal, không "nuốt" mất
    sem_post(sem_mutex);
    sem_post(sem_all3);   // ← QUAN TRỌNG
}
```

> **Tại sao phải `sem_post(sem_all3)` khi fail?**
> Signal này có thể dành cho một **diner khác** chưa được phục vụ.
> Nếu nuốt mất, diner đó sẽ chờ mãi mãi → **starvation / deadlock**.

---

### 2.5 Deadlock và cách phòng tránh

#### Deadlock là gì?

Xảy ra khi **hai hoặc nhiều tiến trình chờ nhau vô hạn**, mỗi tiến trình giữ tài nguyên mà tiến trình kia cần để tiếp tục.

**4 điều kiện cần đồng thời xảy ra (Coffman conditions):**

| # | Điều kiện | Giải thích |
|---|-----------|------------|
| 1 | Mutual exclusion | Tài nguyên chỉ dùng được bởi 1 tiến trình tại 1 thời điểm |
| 2 | Hold and wait | Giữ ≥1 tài nguyên trong khi chờ thêm tài nguyên khác |
| 3 | No preemption | Không thể cưỡng đoạt tài nguyên từ tiến trình đang giữ |
| 4 | Circular wait | Tồn tại vòng chờ: A chờ B, B chờ C, C chờ A |

**Ví dụ deadlock trong bài nếu code sai:**

```
Chef:  giữ sem_mutex, đang sem_wait(sem_space) vì khay đầy
Diner: đang sem_wait(sem_mutex) để vào critical section lấy món

→ Chef chờ Diner lấy để trả sem_space
→ Diner chờ Chef nhả sem_mutex
→ Vòng chờ → DEADLOCK!
```

**Cách phòng tránh — quy tắc thứ tự lấy semaphore:**

```c
// SAI — giữ mutex trong khi chờ sem_space
sem_wait(sem_mutex);
    sem_wait(sem_space);    // có thể block mãi trong khi đang giữ mutex!
    // ...
sem_post(sem_mutex);

// ĐÚNG — chờ sem_space TRƯỚC khi lấy mutex
sem_wait(sem_space);        // chờ ở đây mà không giữ gì cả
sem_wait(sem_mutex);
    // ...
sem_post(sem_mutex);
```

**Quy tắc tổng quát:** Không bao giờ gọi `sem_wait()` trên semaphore khác trong khi đang giữ `sem_mutex`. Nếu cần chờ nhiều semaphore, phải lấy theo **thứ tự nhất quán** và không giữ một cái khi chờ cái kia.

---

### 2.6 Sơ đồ giao tiếp semaphore đầy đủ

**Vùng dữ liệu dùng chung** — được bảo vệ bởi `sem_mutex` (init=1):

```
[ SharedState — shared memory ]
  inventory[3]  tray[3]  tray_total  served  K  M  done
```

**Luồng tín hiệu giữa 3 tiến trình:**

| Semaphore | Init | Hướng tín hiệu | Ý nghĩa |
|-----------|------|----------------|---------|
| `sem_space` | M | Chef `wait()` ← Diner `post() ×3` | Slot trống trên khay; Chef chờ khi khay đầy |
| `sem_all3` | 0 | Waiter `post() ×remaining` → Diner `wait()` | Waiter báo khi đủ P+C+D |
| `sem_done` | 0 | Diner `post()` → Waiter `wait()` | Diner xác nhận đã lấy xong |
| `sem_mutex` | 1 | Tất cả `wait()`/`post()` bọc mọi R/W | Mutual exclusion trên SharedState |

**Luồng hoàn chỉnh theo thứ tự thời gian:**

```
Bước 1 — Chef đưa món:
  sem_wait(space)          <- chờ nếu tray_total == M
  sem_wait(mutex)
    tray[give]++, tray_total++
  sem_post(mutex)

Bước 2 — Waiter kiểm tra:
  sem_wait(mutex)
    kiểm tra tray, in log
    nếu đủ P+C+D: sem_post(all3) × remaining
  sem_post(mutex)
  sem_wait(done)           <- chờ diner xác nhận

Bước 3 — Diner lấy món:
  sem_wait(all3)           <- ngủ đến khi waiter báo
  sem_wait(mutex)
    double-check: tray[i] > 0 ∀i ?
    Nếu đủ:  tray[i]-- ×3, served++
             sem_post(mutex)
             sem_post(space) ×3   <- trả 3 slot -> mở khoá Chef
             sem_post(done)       <- báo Waiter
             break
    Nếu không: sem_post(mutex)
               sem_post(all3)     <- trả lại signal, không nuốt mất
```

## 3. Giai đoạn 2 — Thiết kế cấu trúc

### 3.1 `SharedState` struct

```c
typedef struct {
    int inventory[3];  /* [0]=P [1]=C [2]=D — kho nội bộ của chef      */
    int tray[3];       /* [0]=P [1]=C [2]=D — khay waiter đang cầm     */
    int tray_total;    /* tổng đĩa trên khay = tray[0]+tray[1]+tray[2] */
    int served;        /* số diner đã được phục vụ xong                 */
    int K;             /* tổng số diner                                 */
    int M;             /* = 2K+1, giới hạn tối đa khay                 */
    int done;          /* flag: 0 = đang chạy, 1 = kết thúc            */
} SharedState;
```

### 3.2 Vai trò 4 semaphore — bảng tổng hợp

| Semaphore | Init | Chef dùng | Waiter dùng | Diner dùng |
|-----------|------|-----------|-------------|------------|
| `sem_mutex` | 1 | wait/post bọc mọi R/W | wait/post bọc mọi R/W | wait/post bọc mọi R/W |
| `sem_space` | M | **wait** trước khi đưa đĩa | — | **post ×3** sau khi lấy 1P+1C+1D |
| `sem_all3` | 0 | — | **post ×remaining** khi đủ 3 loại | **wait** để thức dậy; **post** khi fail |
| `sem_done` | 0 | — | **wait** xác nhận | **post** sau khi lấy xong |

### 3.3 Thứ tự semaphore trong từng tiến trình

```
Chef:
  [nấu]  sem_wait(mutex) → inventory[mon]++ → sem_post(mutex)
  [đưa]  sem_wait(space) → sem_wait(mutex) → tray[give]++ → sem_post(mutex)

Waiter:
  sem_wait(mutex) → kiểm tra tray → in log → sem_post(mutex)
  Nếu đủ 3 loại:
    sem_post(all3) × remaining
    sem_wait(done)

Diner:
  sem_wait(all3) → sem_wait(mutex)
    Nếu tray đủ:
      tray[i]-- × 3, tray_total -= 3, served++
      sem_post(mutex)
      sem_post(space) × 3
      sem_post(done)
      break
    Nếu tray không đủ:
      sem_post(mutex)
      sem_post(all3)   ← trả lại signal
```

---

## 4. Giai đoạn 3 — Viết code

> **Thứ tự bắt buộc:** `shared.h` → `ipc.c` → `main.c` → `chef.c` → `waiter.c` → `diner.c`

---

### Bước 1: `shared.h`

```c
#ifndef SHARED_H
#define SHARED_H

#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ── Tên IPC (dùng chung toàn bộ chương trình) ── */
#define SHM_NAME    "/canteen_shm"
#define SEM_MUTEX   "/canteen_mutex"
#define SEM_SPACE   "/canteen_space"
#define SEM_ALL3    "/canteen_all3"
#define SEM_DONE    "/canteen_done"

/* ── Index loại món ── */
#define FOOD_P      0   /* Khai vị  */
#define FOOD_C      1   /* Món chính */
#define FOOD_D      2   /* Bánh ngọt */
#define FOOD_NAMES  {"P", "C", "D"}

/* ── Cấu trúc dữ liệu dùng chung giữa tất cả tiến trình ── */
typedef struct {
    int inventory[3];  /* Kho nội bộ của chef               */
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
```

---

### Bước 2: `ipc.c`

```c
#include "shared.h"

SharedState* init_ipc(int K) {
    /* ── Tạo và map shared memory ── */
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(SharedState));
    SharedState *s = mmap(NULL, sizeof(SharedState),
                          PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
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
     *   Ràng buộc: inventory[i] <= K với mọi i, tổng = 3K
     *
     * Thuật toán:
     *   1. Phân bổ ngẫu nhiên tuần tự cho từng loại
     *   2. Bù phần còn thiếu (remaining > 0) vào các loại chưa đạt K
     */
    srand(time(NULL));
    int remaining = 3 * K;

    for (int i = 0; i < 3; i++) {
        int cap = (remaining > K) ? K : remaining;
        s->inventory[i] = (cap > 0) ? (rand() % (cap + 1)) : 0;
        remaining -= s->inventory[i];
    }
    /* Bù phần còn thiếu */
    while (remaining > 0) {
        for (int i = 0; i < 3 && remaining > 0; i++) {
            if (s->inventory[i] < K) {
                s->inventory[i]++;
                remaining--;
            }
        }
    }

    /* ── Tạo 4 semaphore ── */
    sem_open(SEM_MUTEX, O_CREAT, 0666, 1);       /* binary semaphore */
    sem_open(SEM_SPACE, O_CREAT, 0666, s->M);    /* M slot trống     */
    sem_open(SEM_ALL3,  O_CREAT, 0666, 0);       /* chưa có tín hiệu */
    sem_open(SEM_DONE,  O_CREAT, 0666, 0);       /* chưa có xác nhận */

    return s;
}

void cleanup_ipc(SharedState *state) {
    munmap(state, sizeof(SharedState));
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_SPACE);
    sem_unlink(SEM_ALL3);
    sem_unlink(SEM_DONE);
}
```

---

### Bước 3: `main.c`

```c
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
```

---

### Bước 4: `chef.c`

```c
#include "shared.h"

void run_chef(SharedState *s) {
    const char *names[] = FOOD_NAMES;
    sem_t *mx = sem_open(SEM_MUTEX, 0);
    sem_t *sp = sem_open(SEM_SPACE, 0);

    while (!s->done) {

        /* ══ BƯỚC 1: Nấu thêm 1 món vào kho inventory ══ */
        sem_wait(mx);

        /* Kiểm tra còn loại nào chưa đạt K */
        int can_cook = 0;
        for (int i = 0; i < 3; i++)
            if (s->inventory[i] < s->K) { can_cook = 1; break; }

        if (!can_cook) {
            sem_post(mx);
            break;    /* Đã nấu đủ 3K đĩa, dừng chef */
        }

        /* Chọn ngẫu nhiên loại còn thiếu để nấu */
        int mon;
        do { mon = rand() % 3; } while (s->inventory[mon] >= s->K);
        s->inventory[mon]++;

        printf("[Chef] Nấu món %s | Kho: P=%d C=%d D=%d\n",
               names[mon],
               s->inventory[0], s->inventory[1], s->inventory[2]);
        fflush(stdout);
        sem_post(mx);

        /* ══ BƯỚC 2: Đưa 1 đĩa từ kho lên khay waiter ══ */

        /* Chờ có slot trống — block nếu tray_total == M (khay đầy) */
        sem_wait(sp);
        sem_wait(mx);

        /* Chọn ngẫu nhiên loại có trong kho để đưa */
        int give;
        do { give = rand() % 3; } while (s->inventory[give] == 0);

        s->inventory[give]--;
        s->tray[give]++;
        s->tray_total++;

        printf("[Chef] Đưa món %s | Kho: P=%d C=%d D=%d"
               " | Khay: P=%d C=%d D=%d (%d đĩa)\n",
               names[give],
               s->inventory[0], s->inventory[1], s->inventory[2],
               s->tray[0], s->tray[1], s->tray[2], s->tray_total);
        fflush(stdout);
        sem_post(mx);
    }

    sem_close(mx);
    sem_close(sp);
}
```

---

### Bước 5: `waiter.c`

```c
#include "shared.h"

void run_waiter(SharedState *s) {
    sem_t *mx = sem_open(SEM_MUTEX, 0);
    sem_t *a3 = sem_open(SEM_ALL3,  0);
    sem_t *dn = sem_open(SEM_DONE,  0);

    while (!s->done) {
        sem_wait(mx);

        /* Kiểm tra đủ 3 loại trên khay */
        int has_all = s->tray[0] > 0 &&
                      s->tray[1] > 0 &&
                      s->tray[2] > 0;

        /* Xác định trạng thái để in log */
        const char *status;
        if      (s->tray_total == s->M) status = "ĐẦY KHAY";
        else if (has_all)               status = "ĐỦ 3 MÓN";
        else                            status = "CHỜ THÊM";

        printf("[Waiter] Khay: P=%d C=%d D=%d (%d đĩa) | %s\n",
               s->tray[0], s->tray[1], s->tray[2],
               s->tray_total, status);
        fflush(stdout);

        if (has_all) {
            /* Báo cho tất cả diner chưa được phục vụ */
            int remaining = s->K - s->served;
            for (int i = 0; i < remaining; i++)
                sem_post(a3);
            sem_post(mx);

            /* Chờ ít nhất 1 diner xác nhận đã lấy xong */
            sem_wait(dn);
        } else {
            sem_post(mx);
            usleep(1000);    /* Nhường CPU, tránh busy-wait */
        }
    }

    sem_close(mx);
    sem_close(a3);
    sem_close(dn);
}
```

---

### Bước 6: `diner.c`

```c
#include "shared.h"

void run_diner(SharedState *s, int id) {
    sem_t *mx = sem_open(SEM_MUTEX, 0);
    sem_t *a3 = sem_open(SEM_ALL3,  0);
    sem_t *dn = sem_open(SEM_DONE,  0);
    sem_t *sp = sem_open(SEM_SPACE, 0);

    while (!s->done) {
        /* Chờ waiter báo có đủ 3 loại trên khay */
        sem_wait(a3);

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

            printf("[Diner %d] Lấy 1P + 1C + 1D → XONG | Phục vụ: %d/%d\n",
                   id, s->served, s->K);
            fflush(stdout);

            /* Kiểm tra điều kiện kết thúc */
            if (s->served >= s->K)
                s->done = 1;

            sem_post(mx);

            /* Trả lại 3 slot trên khay cho chef */
            sem_post(sp);
            sem_post(sp);
            sem_post(sp);

            /* Báo waiter đã lấy xong */
            sem_post(dn);

            break;    /* Diner này hoàn thành, thoát */

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
}
```

---

## 5. Giai đoạn 4 — Test & Debug


### 5.1 Checklist test

```bash
# Build
make

# Test K=1 (M=3): edge case tối giản
./canteen 1

# Test K=2 (M=5): trường hợp nhỏ nhất có ý nghĩa
./canteen 2

# Test K=3 (M=7): nhiều diner, kiểm tra không deadlock
./canteen 3

# Sau mỗi lần chạy — kiểm tra không rò rỉ IPC
ls /dev/shm | grep canteen    # phải trống hoàn toàn
```

### 5.2 Debug khi chương trình bị treo

```bash
# Theo dõi tất cả system call của các tiến trình con
strace -f ./canteen 2 2>&1 | grep -E "sem_wait|sem_post"

# Xem tiến trình nào đang block ở sem_wait nào
strace -f ./canteen 2 2>&1 | grep "sem_wait" | grep -v "= 0"

# Kiểm tra giá trị semaphore lúc đang chạy (debug)
# Thêm vào code: int v; sem_getvalue(sem, &v); printf("sem val=%d\n", v);

# Dọn IPC thủ công nếu chương trình crash giữa chừng
rm -f /dev/shm/canteen_shm
rm -f /dev/shm/sem.canteen_*
```

### 5.3 Bảng lỗi hay gặp

| Triệu chứng | Nguyên nhân | Cách sửa |
|-------------|-------------|---------|
| Chương trình treo mãi | Deadlock: chef giữ mutex, chờ space | `sem_wait(space)` phải đứng **trước** `sem_wait(mutex)` trong chef |
| `served` tăng sai số | Race condition | Mọi `served++` phải nằm trong `sem_mutex` |
| Diner lấy khi thiếu món | Thiếu double-check | Thêm `if (tray[i]>0 ∀i)` sau khi lock mutex |
| Diner chờ mãi không xong | Nuốt signal `sem_all3` | Thêm `sem_post(a3)` trong nhánh else của diner |
| `/dev/shm` còn sót | `cleanup_ipc` chạy trước con chết | Đảm bảo `waitpid` đủ tất cả con trước khi cleanup |
| `served` vượt K | Thiếu check `!s->done` | Thêm `&& !s->done` vào điều kiện double-check |
| Output lẫn lộn khó đọc | Nhiều process in cùng lúc | Thêm PID vào log: `printf("[Chef pid=%d]", getpid())` |

---

## 6. Giai đoạn 5 — Makefile & README


### 6.1 Makefile

```makefile
CC     = gcc
CFLAGS = -Wall -Wextra -g
SRCS   = main.c chef.c waiter.c diner.c ipc.c
TARGET = canteen

all: $(TARGET)

$(TARGET): $(SRCS) shared.h
	$(CC) $(CFLAGS) -o $@ $(SRCS) -lrt -lpthread

clean:
	rm -f $(TARGET)
	@echo "Dọn IPC còn sót (nếu có):"
	-rm -f /dev/shm/canteen_shm
	-rm -f /dev/shm/sem.canteen_*

.PHONY: all clean
```

### 6.2 README.txt

```
╔══════════════════════════════════════════════╗
║          Canteen Simulation                  ║
║  Linux System Programming — IPC với fork()  ║
╚══════════════════════════════════════════════╝

MÔ TẢ:
  Mô phỏng phục vụ thức ăn tại canteen với 3 đối tượng:
    - Chef   : nấu và đưa từng đĩa lên khay (1 tiến trình)
    - Waiter : theo dõi khay, báo hiệu khi đủ 3 loại (1 tiến trình)
    - Diner  : chờ và lấy đúng 1P+1C+1D khi đủ (K tiến trình)

  IPC: POSIX Shared Memory (shm_open + mmap) + Named Semaphore

YÊU CẦU HỆ THỐNG:
  - Linux (Ubuntu/Debian khuyến nghị)
  - gcc, make

BUILD:
  make

CHẠY:
  ./canteen K
  Trong đó K là số thực khách (K >= 1)

  Ví dụ:
    ./canteen 2    (2 thực khách, M=5)
    ./canteen 3    (3 thực khách, M=7)

OUTPUT MỖI LƯỢT:
  [Main]     Thông tin khởi tạo
  [Chef]     Món nấu thêm và món đưa lên khay
  [Waiter]   Trạng thái khay (ĐỦ 3 MÓN / CHỜ THÊM / ĐẦY KHAY)
  [Diner i]  Thông báo khi được phục vụ xong

DỌN DẸP:
  make clean

VÍ DỤ OUTPUT (K=2):
  [Main] Canteen bắt đầu: K=2, M=5
  [Main] Kho ban đầu: P=2 C=2 D=2
  [Chef] Nấu món C | Kho: P=2 C=2 D=2
  [Chef] Đưa món P | Kho: P=1 C=2 D=2 | Khay: P=1 C=0 D=0 (1 đĩa)
  [Waiter] Khay: P=1 C=0 D=0 (1 đĩa) | CHỜ THÊM
  ...
  [Waiter] Khay: P=1 C=1 D=1 (3 đĩa) | ĐỦ 3 MÓN
  [Diner 0] Lấy 1P + 1C + 1D → XONG | Phục vụ: 1/2
  ...
  [Diner 1] Lấy 1P + 1C + 1D → XONG | Phục vụ: 2/2
  [Main] Tất cả tiến trình kết thúc. Dọn dẹp IPC...
```


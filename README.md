# Multi-User Event Scheduler

A multi-user event scheduling system built with C, demonstrating core OS concepts including **concurrency, synchronization, file locking, and process management**. This project is developed as an Operating Systems lab mini-project.

## Overview

The Multi-User Event Scheduler is a **client-server application** that allows multiple users to create, manage, and schedule events concurrently. The system implements:

- **Role-based Access Control**: Admin, User, and Guest roles with different permissions
- **Multi-threaded Server**: Handles multiple client connections simultaneously using POSIX threads
- **Priority-based Scheduling**: Implements a scheduling algorithm with aging to prevent starvation
- **File Locking**: Uses fcntl file locks to ensure data integrity when multiple processes access shared data
- **Authentication**: MD5-based password hashing for secure user authentication
- **Event Persistence**: Stores events in a CSV format file for persistent data management

## Key Features

### User Roles
- **ADMIN**: Full control - can create users, add/delete any tasks, and stop the server
- **USER**: Can create account, add/delete own tasks, view all tasks
- **GUEST**: Read-only access - can only view tasks (no account needed)

### Core Functionality
- Create User/Admin - Registration with MD5 password hashing
- View Tasks - Display all scheduled tasks with details
- Add Tasks - Schedule events with configurable intervals and priorities
- Delete Tasks - Remove tasks (users can delete own, admins can delete any)
- Stop Server - Admin-only command to gracefully shut down the server

### Scheduling Algorithm
- **Priority-based Scheduling**: Tasks are sorted by priority
- **Aging Mechanism**: Older tasks get priority boost to prevent starvation
- **Dual-Core Simulation**: Can execute up to 2 tasks concurrently (simulating 2-core CPU)
- **Time-based Intervals**: Supports hourly, daily, weekly, and custom intervals

## Project Structure

```
OSLabMiniproject-Multi-User-Event-Scheduler/
├── README.md                    # Project documentation
├── Makefile                     # Build configuration
├── OS_Mini_Project.pdf          # Project specifications
│
├── server/                      # Server components
│   ├── server.c                 # Main server logic & client handlers
│   ├── scheduler.c              # Event scheduling algorithm
│   └── worker.c                 # Task execution logic
│
├── client/                      # Client application
│   └── client.c                 # Client UI & command interface
│
├── common/                      # Shared utilities
│   ├── events.h                 # Event data structure definitions
│   ├── util.c                   # File locking, MD5 hashing, authentication
│   ├── scheduler.h              # Scheduler function declarations
│   ├── worker.h                 # Worker function declarations
│   └── util.h                   # Utility function declarations
│
└── data/                        # Runtime data storage
    ├── users.txt                # Stored user credentials (role, username, password_hash)
    ├── tasks.txt                # Stored events (CSV format)
    └── events.log               # Server event log
```

## Getting Started

### Prerequisites

- **GCC Compiler** (with C99 support)
- **POSIX Threads** (pthreads library)
- **OpenSSL** (for MD5 hashing)
- **Linux/Unix** operating system

### Installation

```bash
# Install required dependencies (Ubuntu/Debian)
sudo apt-get install build-essential libssl-dev

# Clone the repository
git clone https://github.com/HarshaVPolar/OSLabMiniproject-Multi-User-Event-Scheduler.git
cd OSLabMiniproject-Multi-User-Event-Scheduler

# Create data directory
mkdir -p data
```

### Compilation

```bash
# Build the project
make

# Clean build artifacts
make clean
```

This generates:
- `scheduler_server` - The server executable
- `client_app` - The client executable

## Running the Application

### Step 1: Start the Server

```bash
./scheduler_server
# Output: Server running...
```

The server listens on **localhost:8080** and logs all events to `data/events.log`

### Step 2: Start Client(s)

In another terminal:

```bash
./client_app
```

### Example Usage Flow

#### 1. Create User Account
```
> 1
Username: john
Password: pass123
[Response] User created
```

#### 2. Create Admin Account
```
> 2
Admin Username: admin
Password: admin123
Admin Key: admin_key_12345
[Response] Admin created (with correct admin key)
```

#### 3. Login and Add Task
```
> 3
Select Role: 2 (USER)
Username: john
Password: pass123

> 2 (Add Task)
File: backup.sh
Select Interval: 2 (Daily)
Priority: 5
[Response] Task added
```

#### 4. View All Tasks
```
> 1 (View Tasks)
[Response] Lists all scheduled tasks
```

## OS Concepts Demonstrated

### 1. Concurrency & Thread Management
- Multi-threaded server handles multiple clients simultaneously
- Each client connection spawns a new thread using `pthread_create()`
- Thread detaching with `pthread_detach()` for independent client threads

### 2. Synchronization & File Locking
- **File Locks**: Using `fcntl()` for advisory locking on data files
- Prevents race conditions when multiple processes access `tasks.txt` and `users.txt`
- Write locks (F_WRLCK) ensure atomic read-modify-write operations

### 3. Process & Signal Management
- Signal handler for graceful server shutdown (SIGINT)
- Proper resource cleanup and file closing

### 4. Inter-Process Communication (IPC)
- TCP socket communication between client and server
- Reliable message passing using TCP/IP protocol

### 5. Resource Allocation & Scheduling
- Priority-based scheduling algorithm
- Aging mechanism to prevent task starvation
- Dual-core CPU simulation for concurrent task execution

### 6. Authentication & Security
- MD5 password hashing (secure storage)
- Role-based access control
- Admin key validation

## Event Structure (CSV Format)

Tasks are stored in `data/tasks.txt` with the following format:

```
ID,filename,interval,created_at,last_run,priority,created_by
12345,backup.sh,86400,1719498048,1719498100,5,john
12346,cleanup.sh,3600,1719498050,1719498110,3,admin
```

**Fields:**
- `ID` - Unique task identifier (timestamp + random)
- `filename` - Script/file to execute
- `interval` - Execution interval in seconds
- `created_at` - Timestamp when task was created
- `last_run` - Last execution timestamp
- `priority` - Task priority (higher = executed first)
- `created_by` - Username of task creator

## Scheduling Algorithm

### How It Works

1. **Collection Phase**: Gather all tasks from file
2. **Selection Phase**: Find tasks ready to execute (current_time - last_run >= interval)
3. **Aging Phase**: Boost priority of waiting tasks (aging = waiting_time / 5)
4. **Sorting Phase**: Sort ready tasks by priority (descending)
5. **Execution Phase**: Execute up to 2 tasks concurrently
6. **Update Phase**: Update last_run timestamp and rewrite file with file locking

### Starvation Prevention

Tasks waiting longer than others receive a priority boost:
```c
int aging = waiting_time / 5;
ready[i].priority += aging;
```

This ensures no task waits indefinitely.

## Build Options

```bash
# Build all targets
make all

# Build only server
make scheduler_server

# Build only client
make client_app

# Remove all binaries
make clean
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Compilation fails with "OpenSSL not found" | Install: `sudo apt-get install libssl-dev` |
| Port 8080 already in use | Change PORT in `server/server.c` and rebuild |
| Connection refused | Ensure server is running: `./scheduler_server` |
| Authentication fails | Ensure `data/users.txt` exists and user is registered |
| File permission errors | Run: `chmod 755 data/` or recreate with `mkdir -p data/` |
| No response from server | Check if server process is alive: `ps aux \| grep scheduler_server` |

## File Operations

The server creates the following files in the `data/` directory:

- **users.txt** - User account storage (Role,Username,PasswordHash)
- **tasks.txt** - Task/event storage (CSV format)
- **events.log** - Server event log (CRUD operations)
- **temp.txt** - Temporary file for safe task deletion

## Testing Scenarios

### Scenario 1: Multi-User Concurrent Access
```bash
# Terminal 1
./scheduler_server

# Terminal 2
./client_app    # User john adds task

# Terminal 3
./client_app    # User jane adds task (concurrent)

# Terminal 4
./client_app    # GUEST views tasks
```

### Scenario 2: Task Scheduling
```bash
1. Add task with interval=5 seconds and priority=1
2. Wait 5 seconds
3. Check if task was executed (last_run updated)
4. Add high-priority task
5. Verify it executes before lower-priority tasks
```

### Scenario 3: Permission Testing
```bash
1. Login as USER
2. Try to delete another user's task -> Should fail
3. Login as ADMIN
4. Delete any user's task -> Should succeed
5. Try GUEST delete -> Should fail
```

## Important Notes

WARNING: **Admin Key** - The default admin key hash is hardcoded in `server.c`:
```c
#define ADMIN_KEY_HASH "5f4dcc3b5aa765d61d8327deb882cf99"
```
This is the MD5 hash of "password". **Change this for production!**

WARNING: **Password Storage** - Uses MD5 hashing (suitable for educational purposes only; use bcrypt/Argon2 for production)

WARNING: **Data Persistence** - Data persists in `data/` directory; delete files to reset

## References & Resources

- [POSIX Threads (pthreads) Programming](https://www.gnu.org/software/libc/manual/html_node/POSIX-Threads.html)
- [fcntl() File Locking](https://man7.org/linux/man-pages/man2/fcntl.2.html)
- [CPU Scheduling Algorithms](https://en.wikipedia.org/wiki/Scheduling_(computing))
- [OpenSSL MD5 Documentation](https://www.openssl.org/docs/man1.1.1/man3/MD5.html)
- [Socket Programming in C](https://beej.us/guide/bgnet/)

## Author

**HarshaVPolar**

## License

This project is provided as-is for educational purposes.

## Contributing

To contribute or suggest improvements:
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/YourFeature`)
3. Commit changes (`git commit -m "Add YourFeature"`)
4. Push to branch (`git push origin feature/YourFeature`)
5. Open a Pull Request

## Learning Outcomes

This project demonstrates:
- Multi-threaded server architecture
- Inter-process communication via sockets
- File locking and synchronization primitives
- CPU scheduling algorithms
- User authentication and authorization
- Event-driven programming
- Resource management and cleanup
- Starvation prevention techniques

---

**Last Updated**: June 2026
**Status**: Production-ready for educational purposes

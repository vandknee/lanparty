
## 1. Installation

Clone the repository:

```bash
git clone https://github.com/vandknee/lanparty.git
cd lanparty
```

### Fedora

Install the required development tools:

```bash
sudo dnf install gcc-c++ cmake ncurses-devel git
```

### Ubuntu

```bash
sudo apt update
sudo apt install g++ cmake libncurses-dev git
```

## 2. Build the Project Using CMake

LAN Party uses CMake to configure and compile the project.

### Step 1: Configure

Run this command from the project root:

```bash
cmake -S . -B build
```

This generates the required build configuration inside the `build/` directory.

### Step 2: Compile

```bash
cmake --build build
```

CMake automatically compiles all three executables:

- `server` — TCP chat server and UDP discovery service
- `client` — ncurses interface and automatic server discovery
- `discovery_client` — standalone UDP discovery tester

### Rebuilding After Code Changes

After modifying any C++ source files, run:

```bash
cmake --build build
```

There is no need to manually compile each file using g++.

### Building Individual Programs

To compile only the server:

```bash
cmake --build build --target server
```

To compile only the client:

```bash
cmake --build build --target client
```

To compile the standalone discovery tester:

```bash
cmake --build build --target discovery_client
```

### Clean Build (Optional)

If you encounter build configuration problems, you can perform a fresh build:

```bash
rm -rf build
cmake -S . -B build
cmake --build build
```

Only remove `build/` if it contains generated files and no files you need to preserve.

## 3. Running LAN Party

### Start the Server

Open a terminal in the project directory:

```bash
./build/server
```

Expected output:

```text
LAN Party server started!
Room management enabled.
Listening on port 5000...
UDP discovery running on port 5001
```

Keep the server running.

### Start a Client

Open a second terminal:

```bash
./build/client
```

The client automatically searches for available LAN Party servers.

Select a discovered server, enter your username, and the ncurses chat interface will open.

### Manual Server Connection

To connect directly without discovery:

```bash
./build/client 127.0.0.1
```

For another Linux device on the same LAN:

```bash
./build/client SERVER_IP
```

Replace `SERVER_IP` with the server computer's actual IP address.

### Test Multiple Clients

Run the server in one terminal and launch clients in separate terminals:

```bash
./build/client 127.0.0.1
```

You can run multiple client instances simultaneously, each with a different username.

## 4. Testing UDP Discovery

To test UDP discovery independently:

```bash
./build/discovery_client
```

The program searches for LAN Party servers and displays their IP addresses.

The main server must be running for it to respond.

## 5. Development Workflow

After cloning the repository for the first time:

```bash
cmake -S . -B build
cmake --build build
```

After making changes to C++ files:

```bash
cmake --build build
```

After testing your changes:

```bash
git add .
git commit -m "Describe your changes"
git push
```

The `build/` directory contains generated files and is excluded from Git using `.gitignore`.

# Y86 Assembly Language Server And Client

## Overview

This project simulates a Y86 Assembly architecture using a client-server model where clients can connect to a central server to execute Y86 assembly instructions. Built using C++ and employing socket programming, the project uses the TCP protocol to ensure reliable communication between clients and the server.

## Key Features

- **Multi-client Support**: Multiple clients can connect to the server to execute Y86 instructions.

- **TCP Communication**: Ensures reliable data transfer between client and server over socket connections.

- **Instruction Handler**: Simulates a Y86 processor, interpreting and executing the assembly code sent by clients.

## Project Structure

- `server.cpp`: Manages server operations, handling incoming client connections and requests.

- `client.cpp`: Client-side implementation, responsible for sending Y86 assembly instructions to the server.

- `y86_instruction_handler.cpp/h`: Implements the logic to process and simulate Y86 instructions on the server.

## Technologies

- **Languages**: C++

- **Networking**: TCP protocol with socket programming for client-server communication

- **Assembly Emulation**: Emulates Y86 instruction handling and execution.

## Build and Run

1. Navigate to the project directory. 

2. Compile the project using the provided `Makefile`:

```shell
make
```

3. Start the server:

```shell
./server
```

4. Run the client:

```shell
./client
```
# 🚢 Battleship Multiplayer (Qt/C++)

A highly responsive, asynchronous multiplayer Battleship game built using C++ and the Qt Framework. The project architecture utilizes a decoupled Client-Server model over TCP, supporting simultaneous multi-match lobbies, server-side state validation, and real-time live connection logging.

---

## 📂 Repository Structure

The project code and binary distributions are organized as follows:

```text
├── development/
│   ├── client/
│   │   └── BattleshipClient/      # Qt Client source code (MainWindow, GameTab, UI, Localization)
│   └── server/
│       └── BattleshipServer/      # C++ Server source code (NetworkServer, GameSession, Main loop)
└── Release/
    ├── Client.zip                 # Standalone production-ready Windows Client (includes all required Qt DLLs)
    └── Server.zip                 # Standalone production-ready Windows Server (includes all required Qt DLLs)
```

---

## ⚡ Quick Start & Execution Order

> ⚠️ **CRITICAL REQUIREMENT:** You MUST launch the server before starting any game clients. If a client is launched first, its connection attempt will fail because no server is listening for incoming connections.

### Step 1: Launch the Server

Navigate to `Release/` and extract `Server.zip`, or compile the project inside `development/server/BattleshipServer/`.

Execute:

```text
BattleshipServer.exe
```

inside a native command window (CMD or PowerShell).

#### Live Telemetry Interface

The server features a dedicated, thread-safe background input worker.

Press **[ENTER]** in the server console at any time to instantly print a live status report detailing:

- Connected clients
- Active matchmaking queues
- Running match UUIDs

---

### Step 2: Launch the Clients

Navigate to `Release/` and extract `Client.zip`, or compile the project inside `development/client/BattleshipClient/`.

Execute:

```text
BattleshipClient.exe
```

To test matchmaking locally on a single machine:

1. Open two separate client instances.
2. Click **"Find new match"** on both clients.
3. The server's FIFO matchmaking engine will immediately pair them and initialize a new match session.

---

## 🛠️ Technical Highlights & Architecture

### Packet Framing & Anti-Streaming

Network communication safely handles TCP packet fragmentation and packet coalescing through line-delimited JSON payloads wrapped in explicit newline (`\n`) boundaries.

Both client and server event loops process incoming data via:

```cpp
canReadLine()
```

stream guards to ensure complete packet handling.

---

### Concurrent Match Management

The client UI wraps autonomous game grids into custom `GameTab` modules.

This loose coupling allows a single client to safely participate in multiple independent matches simultaneously using dynamic tab controls without state bleeding.

---

### Server Enforcement (Anti-Cheat)

All core game logic is enforced exclusively on the server, including:

- Ship placement validation
- Damage tracking
- Turn management
- Win/loss determination

The client acts strictly as a presentation layer.

---

### Graceful Forfeit Cascade

The server continuously monitors connection state health.

If a player:

- Closes a live game tab
- Disconnects unexpectedly
- Loses network connectivity

the server immediately evaluates the match lifecycle and awards a victory via forfeit to the remaining player.

---

### Runtime Localization (i18n)

The client supports dynamic runtime localization.

At startup, the application automatically:

1. Detects the host operating system locale.
2. Loads the appropriate compiled translation (`.qm`) file.
3. Applies localized UI text without requiring recompilation.

---

## 🏗️ Development & Compiling

### Requirements

- Qt 5.15+
- MSVC 2019/2022 or MinGW
- Qt Creator

### Building the Project

1. Open Qt Creator.
2. Click **Open Project**.
3. Select the appropriate `.pro` file from either:

```text
development/client/BattleshipClient/
```

or

```text
development/server/BattleshipServer/
```

4. Select your desired compiler kit.
5. Change the build configuration from:

```text
Debug → Release
```

for optimized production performance.

# Threading and Synchronization on macOS

**Understanding Multi-Threading in olcPixelGameEngine3 macOS Implementation**

> This guide explains how threading works in the macOS implementation, why it's important, and how to work with it correctly.

---

## Table of Contents

1. [Why Threading Matters](#why-threading-matters)
2. [The Main Thread Rule](#main-thread-rule)
3. [Grand Central Dispatch (GCD)](#gcd)
4. [Threading in Our Implementation](#our-implementation)
5. [Common Patterns](#common-patterns)
6. [Thread Safety](#thread-safety)
7. [Debugging Threading Issues](#debugging)
8. [Best Practices](#best-practices)

---

<a name="why-threading-matters"></a>
## Why Threading Matters

### The Problem

Modern applications do multiple things at once:

```
┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐
│   Game Logic     │    │  Rendering       │    │  UI Updates      │
│   (Game Loop)    │    │  (OpenGL)        │    │  (Window Events) │
└──────────────────┘    └──────────────────┘    └──────────────────┘
```

If everything ran on one thread:
- UI would freeze during game logic
- Window wouldn't respond to events
- Poor performance

**Solution:** Use multiple threads!

---

### What is a Thread?

Think of a thread as a **separate worker** that can do tasks:

```
Main Thread: [→→→→→→→→→→→→→→→→→→→→→→]
              ↓
Game Thread: [→→→→→→→→→→→→→→→→→→→→→→]
              ↓
Render Thread: [→→→→→→→→→→→→→→→→→→→]
```

Each thread runs independently and can do work at the same time (concurrently).

---

<a name="main-thread-rule"></a>
## The Main Thread Rule

### The Most Important Rule

> **All UI operations on macOS MUST happen on the main thread!**

This means:
- ✅ Creating windows - main thread
- ✅ Updating window titles - main thread
- ✅ Showing/hiding cursor - main thread
- ✅ Moving windows - main thread
- ✅ Handling most events - main thread
- ❌ Game logic - can be any thread
- ❌ Math calculations - can be any thread
- ❌ File I/O - can be any thread

### Why This Rule Exists

macOS's UI framework (AppKit/Cocoa) is **not thread-safe**. If multiple threads tried to update the UI at once, you'd get:

- Crashes 💥
- Frozen windows 🧊
- Corrupted graphics 🎨❌
- Race conditions 🏁

**Apple's Solution:** Force all UI on one thread - the **main thread**.

---

### Checking Current Thread

You can check if you're on the main thread:

```cpp
#include <pthread.h>

bool isMainThread() {
    return pthread_main_np() != 0;
}

// Usage
if (isMainThread()) {
    // Safe to do UI operations
    window_setTitle(window, "New Title");
} else {
    // NOT safe! Need to dispatch to main thread
}
```

---

<a name="gcd"></a>
## Grand Central Dispatch (GCD)

### What is GCD?

**Grand Central Dispatch** is Apple's system for managing threads. It's much easier than manually creating threads!

```cpp
#include <dispatch/dispatch.h>
```

### Key Concepts

#### 1. Queues

A **queue** is a list of tasks to be executed:

```
Queue: [Task 1] → [Task 2] → [Task 3] → [Task 4]
       ↓          ↓          ↓          ↓
     Thread 1   Thread 2   Thread 1   Thread 3
```

#### 2. The Main Queue

The **main queue** is special - it runs on the main thread:

```cpp
// Get the main queue
dispatch_queue_t mainQueue = dispatch_get_main_queue();
```

#### 3. Global Queues

**Global queues** run on background threads:

```cpp
// Get a background queue
dispatch_queue_t backgroundQueue = dispatch_get_global_queue(
    DISPATCH_QUEUE_PRIORITY_DEFAULT,
    0
);
```

---

### Dispatch Functions

#### dispatch_async - Run Later (Non-Blocking)

Schedules a task to run on a queue and **returns immediately**:

```cpp
dispatch_async(dispatch_get_main_queue(), ^{
    // This code runs on the main thread
    // But we don't wait for it to finish
    window_setTitle(window, "New Title");
});

// Code here runs immediately, before the block above!
printf("This prints first!\n");
```

**Use when:**
- You don't need to wait for the result
- You want to update UI from a background thread
- You want to avoid blocking

---

#### dispatch_sync - Run Now (Blocking)

Schedules a task and **waits** for it to complete:

```cpp
dispatch_sync(dispatch_get_main_queue(), ^{
    // This code runs on the main thread
    window_setTitle(window, "New Title");
    // And we wait for it to finish
});

// Code here runs AFTER the block above completes
printf("This prints second!\n");
```

**Use when:**
- You need the result immediately
- You must wait for UI to update
- Order matters

**⚠️ Warning:** Never call `dispatch_sync` on your current queue - that's a deadlock!

---

#### Block Syntax (The ^ Symbol)

GCD uses **blocks** - think of them as inline functions:

```cpp
// Block with no parameters
^{
    printf("Hello from block!\n");
}

// Block with parameters
^(int x, int y) {
    printf("x = %d, y = %d\n", x, y);
}

// Block with return value
^int(int x) {
    return x * 2;
}
```

**Blocks can capture variables:**
```cpp
int value = 42;

dispatch_async(queue, ^{
    // Can use 'value' here
    printf("Value: %d\n", value);
});
```

---

<a name="our-implementation"></a>
## Threading in Our Implementation

### Thread Architecture

```
┌─────────────────────────────────────────────────────────┐
│                     Main Thread                         │
│  - NSApplication run loop                               │
│  - Window creation                                      │
│  - Event handling                                       │
│  - UI updates                                           │
└──────────┬──────────────────────────────────────────────┘
           │
           │ (Events dispatched via callbacks)
           │
           ↓
┌─────────────────────────────────────────────────────────┐
│                   Game Thread                           │
│  - Game logic (OnUserUpdate)                            │
│  - Drawing commands                                     │
│  - Input processing                                     │
└──────────┬──────────────────────────────────────────────┘
           │
           │ (OpenGL commands)
           │
           ↓
┌─────────────────────────────────────────────────────────┐
│                 OpenGL Context                          │
│  - Rendering                                            │
│  - Texture uploads                                      │
│  - Buffer updates                                       │
└─────────────────────────────────────────────────────────┘
```

---

### Application Startup Flow

Let's trace the threading during startup:

```cpp
int main() {
    // This is the main thread
    MyGame game;
    
    // Still main thread
    game.Construct(800, 600, 1, 1);
    
    // This call BLOCKS until app quits
    game.Start();  
    //   ↓
    //   └→ Host_Apple_MacOS::StartSystem()
    //       ↓
    //       └→ pMacApplication->run()
    //           ↓
    //           └→ [NSApplication run]  ← macOS takes over main thread
    //               ↓
    //               └→ Event loop runs on main thread
    //                   ↓
    //                   └→ Your game loop may run on different thread
    
    return 0;
}
```

**Key Points:**
- `main()` runs on the main thread
- `[NSApplication run]` takes over the main thread
- Never returns until app quits
- Events are processed on main thread
- Game logic may run elsewhere

---

<a name="common-patterns"></a>
## Common Patterns

### Pattern 1: UI Update from Game Thread

Your game is running, and you want to change the window title:

```cpp
// In your game (might be on game thread)
bool OnUserUpdate(float fElapsedTime) override {
    if (GetKey(olc::Key::SPACE).bPressed) {
        // Need to update window title
        // But we might not be on main thread!
        
        SetWindowTitle("Space Pressed!");
    }
    return true;
}
```

**Implementation in Host Layer:**
```cpp
bool Host_Apple_MacOS::UpdateWindowFrameTitle(olc::Window* pWindow) {
    if (!pMacOSWindow) return false;
    
    // Get title (safe on any thread - just reading a string)
    std::string title = pWindow->GetWindowTitle();
    
    // Update UI on main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        pMacOSWindow->setTitle(title.c_str());
    });
    
    return true;
}
```

**Why `dispatch_async`?**
- Game thread doesn't need to wait
- Title update can happen later
- Avoids blocking game loop

---

### Pattern 2: Getting Window Info Safely

Sometimes you need data from the window:

```cpp
bool Host_Apple_MacOS::SetMousePosition(olc::Window* pWindow, const olc::vi2d& vPos) {
    // Must move cursor on main thread
    // And we need to wait for it to complete
    
    dispatch_sync(dispatch_get_main_queue(), ^{
        pMacOSWindow->setCursorPosition(vPos.x, vPos.y);
    });
    
    return true;
}
```

**Why `dispatch_sync`?**
- We need cursor to move NOW
- Caller expects it to be done when we return
- Okay to wait (mouse operations are fast)

---

### Pattern 3: Event Callback to Game

Events come from macOS (main thread) to your game:

```cpp
// In api_macos.cpp (runs on main thread)
static void keyDown_handler(id self, SEL _cmd, id event) {
    // Extract key info on main thread
    unsigned short keyCode = (unsigned short)objc_msgSend(
        event,
        sel_registerName(kKeyCodeSel)
    );
    
    // Call callback (passes data to other thread)
    if (window->keyDownCallback) {
        window->keyDownCallback(
            keyCode,
            characters,
            modifierFlags,
            window->keyDownUserData
        );
    }
}
```

The callback then updates PGE state:

```cpp
// In host_apple_macos.cpp (callback on main thread)
pMacOSEventHandler->setKeyDownHandler(
    [this](uint16_t keyCode, const char* characters, uint32_t modifierFlags) {
        // This runs on main thread
        // But updating PGE state is thread-safe
        if (mapKeys.count(keyCode)) {
            pPGEwindow->olc_UpdateKeyState(mapKeys[keyCode], true);
        }
    }
);
```

**Thread Safety:**
- Event arrives on main thread
- Data extracted on main thread
- Callback runs on main thread
- But data is copied to game thread's memory
- Game loop reads it safely

---

### Pattern 4: Initialization on Main Thread

Creating windows must happen on main thread:

```cpp
bool Host_Apple_MacOS::StartSystem() {
    // We're already on main thread here (called from main())
    
    // Create application (main thread)
    pMacApplication = std::make_unique<olc::apis::macos::Application>();
    pMacApplication->initialize();
    pMacApplication->activate();
    
    // Create window (main thread)
    pMacOSWindow = std::make_unique<olc::apis::macos::Window>(
        frameBounds.width,
        frameBounds.height,
        "My Game"
    );
    
    // Show window (main thread)
    unsigned long styleMask = ConvertPGE2WindowStyle();
    pMacOSWindow->show(styleMask);
    
    // Start event loop (stays on main thread)
    pMacApplication->run();  // BLOCKS!
    
    return true;
}
```

---

<a name="thread-safety"></a>
## Thread Safety

### What is Thread Safety?

Code is **thread-safe** if it works correctly when accessed from multiple threads.

**Not thread-safe:**
```cpp
int counter = 0;

// Thread 1
counter++;  // Read, increment, write

// Thread 2  
counter++;  // Read, increment, write

// Result: Might be 1 instead of 2!
```

**Thread-safe with mutex:**
```cpp
std::mutex mtx;
int counter = 0;

// Thread 1
{
    std::lock_guard<std::mutex> lock(mtx);
    counter++;  // Safe!
}

// Thread 2
{
    std::lock_guard<std::mutex> lock(mtx);
    counter++;  // Safe!
}

// Result: Always 2
```

---

### Our Thread-Safe Patterns

#### 1. Immutable Data

Data that never changes is always thread-safe:

```cpp
// Set once during initialization
const int screenWidth = 800;
const int screenHeight = 600;

// Safe to read from any thread!
```

---

#### 2. Thread-Local Data

Each thread has its own copy:

```cpp
// Different for each thread
thread_local int threadID = 0;
```

---

#### 3. Atomic Operations

Operations that can't be interrupted:

```cpp
#include <atomic>

std::atomic<bool> shouldQuit{false};

// Thread 1
shouldQuit.store(true);  // Atomic write

// Thread 2
if (shouldQuit.load()) {  // Atomic read
    cleanup();
}
```

---

#### 4. Message Passing

Don't share data - send copies:

```cpp
// Instead of sharing:
std::string sharedTitle;  // Both threads access - DANGER!

// Send a copy:
dispatch_async(dispatch_get_main_queue(), ^{
    std::string titleCopy = title;  // Copy made
    window_setTitle(window, titleCopy.c_str());
});
```

---

### Our Callback Pattern is Thread-Safe

Here's why our event callbacks work:

```cpp
// Step 1: Event on main thread
static void keyDown_handler(id self, SEL _cmd, id event) {
    // Extract data (main thread)
    unsigned short keyCode = extractKeyCode(event);
    
    // Step 2: Call callback with COPY of data
    if (window->keyDownCallback) {
        window->keyDownCallback(
            keyCode,           // Copy of primitive
            charactersCopy,    // Copy of string
            modifierFlags,     // Copy of flags
            userData
        );
    }
}

// Step 3: Callback stores data
pMacOSEventHandler->setKeyDownHandler(
    [this](uint16_t keyCode, const char* chars, uint32_t flags) {
        // Data is copied into this lambda
        // Lambda runs on main thread
        // But updates PGE state which game thread reads
        pPGEwindow->olc_UpdateKeyState(mapKeys[keyCode], true);
    }
);

// Step 4: Game reads data
bool OnUserUpdate(float fElapsedTime) override {
    // Game thread reads the key state
    if (GetKey(olc::Key::SPACE).bPressed) {
        // ...
    }
}
```

**Why it works:**
- Data is copied at each step
- No shared mutable state
- PGE's key state is protected internally
- Event updates are atomic (set flag to true/false)

---

<a name="debugging"></a>
## Debugging Threading Issues

### Common Problems

#### 1. Deadlock

Thread waits for itself:

```cpp
// WRONG! Deadlock!
dispatch_sync(dispatch_get_main_queue(), ^{
    // Already on main queue - will wait forever!
});
```

**Fix:** Check if already on main thread:
```cpp
if (pthread_main_np()) {
    // Already on main thread - do it now
    doWork();
} else {
    // Not on main thread - dispatch
    dispatch_sync(dispatch_get_main_queue(), ^{
        doWork();
    });
}
```

---

#### 2. Race Condition

Multiple threads access same data:

```cpp
// Thread 1
windowTitle = "Game";

// Thread 2
windowTitle = "Paused";

// Result: Undefined! Could be either, or garbage!
```

**Fix:** Use `dispatch_sync` to serialize:
```cpp
dispatch_sync(dispatch_get_main_queue(), ^{
    windowTitle = "Game";
});

dispatch_sync(dispatch_get_main_queue(), ^{
    windowTitle = "Paused";
});

// Now guaranteed to be "Paused"
```

---

#### 3. UI Update on Background Thread

```cpp
// WRONG! Crashes or undefined behavior
std::thread([]{
    window_setTitle(window, "Background");  // NOT on main thread!
}).detach();
```

**Fix:** Always dispatch UI to main thread:
```cpp
std::thread([]{
    // Do background work...
    
    // Update UI on main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        window_setTitle(window, "Done!");
    });
}).detach();
```

---

### Debugging Tools

#### Print Thread Info

```cpp
void printThreadInfo(const char* where) {
    pthread_t thread = pthread_self();
    bool isMain = pthread_main_np() != 0;
    
    printf("[%s] Thread: %p, Main: %s\n",
           where,
           (void*)thread,
           isMain ? "YES" : "NO");
}

// Usage
printThreadInfo("StartSystem");
```

---

#### Thread Sanitizer

Use Xcode's Thread Sanitizer to find threading bugs:

```bash
# Build with thread sanitizer
clang++ -fsanitize=thread your_code.cpp
```

It will detect:
- Data races
- Deadlocks
- Use-after-free
- Double-locks

---

<a name="best-practices"></a>
## Best Practices

### ✅ DO:

1. **Always dispatch UI to main thread**
   ```cpp
   dispatch_async(dispatch_get_main_queue(), ^{
       window_setTitle(window, title);
   });
   ```

2. **Use `dispatch_async` for fire-and-forget**
   ```cpp
   // Don't need to wait
   dispatch_async(dispatch_get_main_queue(), ^{
       updateStatusBar();
   });
   ```

3. **Use `dispatch_sync` when you need results**
   ```cpp
   // Need to wait
   __block CGRect frame;
   dispatch_sync(dispatch_get_main_queue(), ^{
       frame = getWindowFrame();
   });
   return frame;
   ```

4. **Copy data when passing between threads**
   ```cpp
   std::string titleCopy = title;  // Copy
   dispatch_async(dispatch_get_main_queue(), ^{
       setTitle(titleCopy.c_str());
   });
   ```

5. **Use thread-safe containers**
   ```cpp
   std::mutex mtx;
   std::vector<Event> events;
   
   void addEvent(Event e) {
       std::lock_guard<std::mutex> lock(mtx);
       events.push_back(e);
   }
   ```

---

### ❌ DON'T:

1. **Never block main thread**
   ```cpp
   // WRONG!
   dispatch_sync(dispatch_get_main_queue(), ^{
       heavyComputation();  // Main thread frozen!
   });
   ```

2. **Never do UI off main thread**
   ```cpp
   // WRONG!
   std::thread([]{
       window_setTitle(window, "Title");  // Crash!
   }).detach();
   ```

3. **Never `dispatch_sync` to your own queue**
   ```cpp
   // WRONG! Deadlock!
   if (pthread_main_np()) {
       dispatch_sync(dispatch_get_main_queue(), ^{
           // Waiting for ourselves!
       });
   }
   ```

4. **Never share mutable state**
   ```cpp
   // WRONG!
   std::vector<int> sharedData;  // Both threads modify
   
   // Thread 1
   sharedData.push_back(1);
   
   // Thread 2
   sharedData.push_back(2);  // CRASH!
   ```

5. **Never access UIKit/AppKit without GCD**
   ```cpp
   // WRONG!
   std::thread([]{
       [window setTitle:@"Title"];  // Crash!
   }).detach();
   
   // RIGHT!
   std::thread([]{
       dispatch_async(dispatch_get_main_queue(), ^{
           [window setTitle:@"Title"];  // Safe!
       });
   }).detach();
   ```

---

## Summary

**Key Concepts:**

✅ **Main Thread Rule** - All UI on main thread  
✅ **GCD** - Apple's threading system  
✅ **dispatch_async** - Run later, don't wait  
✅ **dispatch_sync** - Run now, wait for result  
✅ **Blocks** - Inline functions with `^` syntax  
✅ **Thread Safety** - Protect shared data  
✅ **Message Passing** - Copy data, don't share  

**Remember:**
- UI operations → main thread (always!)
- Game logic → any thread (usually)
- OpenGL → needs context (be careful!)
- Copy data when sending between threads
- Use GCD instead of raw threads
- `dispatch_async` for UI updates
- `dispatch_sync` when you need results

**Next Steps:**
- Learn about the [Practical Examples](./05-Practical-Examples.md)
- Learn about the [Threading And GCD](./04-Threading-And-GCD)
- Learn about the [Objective-C Runtime](./03-Objective-C-Runtime.md)
- Learn about the [Function Reference Guide](./02-Function-Reference.md)
- Learn about the [Architecture](./01-Architecture-Overview.md)

---

**Happy Threading!** 🧵

*For more information, see Apple's [Concurrency Programming Guide](https://developer.apple.com/library/archive/documentation/General/Conceptual/ConcurrencyProgrammingGuide/Introduction/Introduction.html)*

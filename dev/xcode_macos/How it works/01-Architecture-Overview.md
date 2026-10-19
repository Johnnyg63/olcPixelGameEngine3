# Architecture Overview

**Understanding the Three-Layer Design of olcPixelGameEngine3 macOS Implementation**

> This document explains how the macOS platform code is organized and how the different layers work together.

---

## Table of Contents

1. [The Big Picture](#big-picture)
2. [Three-Layer Architecture](#three-layer)
3. [Layer 1: Host Interface](#layer1)
4. [Layer 2: C++ Wrapper](#layer2)
5. [Layer 3: Objective-C Bridge](#layer3)
6. [The Complete Flow](#complete-flow)
7. [Threading Model](#threading)
8. [Error Handling](#error-handling)
9. [Memory Ownership](#memory-ownership)
10. [Key Takeaways](#takeaways)

---

<a name="big-picture"></a>
## The Big Picture

Think of the macOS implementation like a restaurant:

- **Your Game** = The customer who orders food
- **Host_Apple_MacOS** = The waiter who takes your order
- **Wrapper Layer** = The kitchen manager who organizes things
- **api_macos.cpp** = The chef who cooks (talks to macOS)
- **macOS/OpenGL** = The ingredients and stove

Each layer has a specific job, and they all work together to make your game run!

<a name="three-layer"></a>
## Three-Layer Architecture

The macOS implementation uses three main layers:

```
┌────────────────────────────────────────┐
│  Layer 1: Host Interface               │
│  (host_apple_macos.h/cpp)              │
│  • Your game talks to this layer       │
│  • Hides all platform-specific code    │
│  • Same interface on Windows/Linux     │
└────────────────────────────────────────┘
              ↓
┌────────────────────────────────────────┐
│  Layer 2: C++ Wrapper                  │
│  (api_macos_wrapper.hpp)               │
│  • Modern C++ classes                  │
│  • RAII memory management              │
│  • Type safety                         │
└────────────────────────────────────────┘
              ↓
┌────────────────────────────────────────┐
│  Layer 3: Objective-C Bridge           │
│  (api_macos.h/cpp)                     │
│  • Direct Objective-C calls            │
│  • Memory management                   │
│  • Low-level Apple APIs                │
└────────────────────────────────────────┘
```

<a name="layer1"></a>
## Layer 1: Host Interface (`host_apple_macos.h/cpp`)

**Purpose**: Provide a clean, cross-platform interface for your game.

**In olcPixelGameEngine3.h**: Lines ~7800-8500

### Key Class: `Host_Apple_MacOS`

This is the main class that your game interacts with. It inherits from `olc::host::Host`, which means it follows the same pattern as Windows and Linux versions.

```cpp
class Host_Apple_MacOS : public olc::host::Host
{
    // Window management
    bool AddWindowFrame(...)
    bool CloseWindowFrame(...)
    
    // Mouse control
    bool SetMousePosition(...)
    bool SetMouseVisible(...)
    
    // Application lifecycle
    bool OnApplicationStart(...)
    bool StartSystem(...)
    bool StopSystem(...)
    
    // ... and more!
};
```

### What This Layer Does:

1. **Window Creation**: When you call `Construct()` in your game, this layer creates the window
2. **Event Routing**: Mouse clicks and key presses get routed to your game
3. **OpenGL Setup**: Prepares the graphics context
4. **Main Loop**: Runs the game loop (update → draw → repeat)

### Important Members:

```cpp
// The actual window you see on screen
std::unique_ptr<olc::apis::macos::Window> pMacOSWindow;

// Handles mouse/keyboard events
std::unique_ptr<olc::apis::macos::EventHandler> pMacOSEventHandler;

// Manages OpenGL rendering
std::shared_ptr<olc::apis::macos::OpenGLRenderer> pMacOSOpenGLRenderer;

// The macOS application instance
std::unique_ptr<olc::apis::macos::Application> pMacApplication;
```

<a name="layer2"></a>
## Layer 2: C++ Wrapper (`api_macos_wrapper.hpp`)

**Purpose**: Provide safe, modern C++ classes that wrap Objective-C objects.

**In olcPixelGameEngine3.h**: Lines ~7400-7800

This layer creates C++ classes that feel natural but talk to Objective-C under the hood.

### Key Classes:

#### 1. Application Class
```cpp
class Application {
    void initialize();
    void activate();
    void run();  // Starts the event loop
    void stop();
    std::string getSystemLocale();
};
```

**What it does**: Manages the macOS application instance (NSApplication). Every macOS app needs one.

#### 2. Window Class
```cpp
class Window {
    void create();
    void show();
    void setTitle(const std::string& title);
    void setSize(double width, double height);
    void setCursorVisible(bool visible);
    // ... and more!
};
```

**What it does**: Represents your game window (NSWindow). Handles window operations like resizing, moving, showing/hiding.

#### 3. OpenGLRenderer Class
```cpp
class OpenGLRenderer {
    void initialize(Window& window);
    void setupContext();
    void makeCurrentContext();
    void setVsync(bool enabled);
    void* getCGLContextObj();
    // ... and more!
};
```

**What it does**: Manages the OpenGL rendering context. This is what lets you draw graphics.

#### 4. EventHandler Class
```cpp
class EventHandler {
    void setKeyDownHandler(std::function<void(const KeyEvent&)> handler);
    void setMouseDownHandler(std::function<void(const MouseEvent&)> handler);
    // ... and more!
};
```

**What it does**: Routes keyboard, mouse, and touch events from macOS to your game.

### Why Use Wrappers?

**Problem**: Objective-C uses manual memory management (retain/release). If we forget to release something, we get memory leaks!

**Solution**: C++ wrappers use RAII (Resource Acquisition Is Initialization):

```cpp
{
    Window window;  // Constructor creates NSWindow
    window.show();
    // ... use window ...
}  // Destructor automatically releases NSWindow - no memory leaks!
```

<a name="layer3"></a>
## Layer 3: Objective-C Bridge (`api_macos.h/cpp`)

**Purpose**: Direct communication with macOS Appkit and OpenGL frameworks.

**In olcPixelGameEngine3.h**: Lines ~8800-18000+

This is the "dirty work" layer that speaks Objective-C. It's complex but you rarely need to modify it.

### How It Works:

#### C Structures
```cpp
// Plain C structures that hold Objective-C objects
struct Application {
    id nsApp;        // The actual NSApplication object
    id delegate;     // Application delegate
    
    // Function pointers for operations
    void (*initialize)(struct Application* self);
    void (*activate)(struct Application* self);
    void (*run)(struct Application* self);
};
```

**Why C structures?** They can cross the C++/Objective-C boundary safely!

#### Objective-C Runtime Calls

Instead of normal Objective-C syntax:
```objc
[window setTitle:@"Hello World"];
```

We use runtime calls:
```cpp
// Get the selector (method name)
SEL setTitleSel = sel_registerName("setTitle:");

// Create an NSString
id titleString = objc_msgSend(NSStringClass, 
                              sel_registerName("stringWithUTF8String:"),
                              "Hello World");

// Call the method
objc_msgSend(window, setTitleSel, titleString);
```

**Why so complex?** Because we're writing C++ code but calling Objective-C methods. The runtime lets us do this!

### Memory Management

This layer carefully manages autorelease pools:

```cpp
void someFunction() {
    AutoreleasePool pool;  // Create pool
    
    // Create Objective-C objects
    id string = createString();
    id window = createWindow();
    
    // ... use objects ...
    
    // Pool destructor automatically releases all objects
}
```

**Important**: Every function that creates Objective-C objects should have an autorelease pool!

<a name="complete-flow"></a>
## The Complete Flow

Let's trace what happens when you create a window:

### Step 1: Your Game Code
```cpp
// In your game's OnUserCreate()
olc::PixelGameEngine pge;
pge.Construct(800, 600, 2, 2);  // 800x600 window, 2x pixel scale
```

### Step 2: Host Layer (`host_apple_macos.cpp`)
```cpp
bool Host_Apple_MacOS::AddWindowFrame(...) {
    // Create C++ wrapper window
    pMacOSWindow = std::make_unique<olc::apis::macos::Window>(
        0, 0, vWindowSize.x, vWindowSize.y
    );
    
    // Tell wrapper to create actual macOS window
    pMacOSWindow->create();
    pMacOSWindow->show();
    
    return true;
}
```
*Found in olcPixelGameEngine3.h around line 7900*

### Step 3: C++ Wrapper Layer (`api_macos_wrapper.hpp`)
```cpp
void Window::create() {
    // Call the C bridge function
    window_create(pWindow);
}

void Window::show() {
    // Call the C bridge function
    window_show(pWindow);
}
```
*Found in olcPixelGameEngine3.h around line 7500*

### Step 4: C Bridge Layer (`api_macos.cpp`)
```cpp
void window_create(Window* self) {
    AutoreleasePool pool;  // Manage memory
    
    // Get NSWindow class
    Class NSWindowClass = objc_getClass("NSWindow");
    
    // Allocate window
    id windowAlloc = objc_msgSend(NSWindowClass, 
                                  sel_registerName("alloc"));
    
    // Initialize window with frame and style
    self->nsWindow = objc_msgSend(windowAlloc,
                                  sel_registerName("initWithContentRect:..."),
                                  self->windowFrame, styleMask, ...);
}

void window_show(Window* self) {
    // Call NSWindow's orderFrontRegardless method
    objc_msgSend(self->nsWindow, 
                 sel_registerName("orderFrontRegardless"));
}
```
*Found in olcPixelGameEngine3.h around line 10500*

### Step 5: macOS AppKit Framework
The actual NSWindow object is created and displayed on screen!

<a name="threading"></a>
## Threading Model

macOS has strict threading rules:

### Main Thread Rule
**All UI operations MUST happen on the main thread!**

We use Grand Central Dispatch (GCD) to ensure this:

```cpp
dispatch_async(dispatch_get_main_queue(), ^{
    // This code block runs on main thread
    // Safe for window operations
    createWindow();
    showWindow();
});
```

### Game Loop Thread
Your game's `OnUserUpdate()` runs on a separate thread. This prevents UI freezing!

```
Main Thread:              Game Thread:
  Window events ──────→   Process input
  Render display ←─────   Update game logic
                          Draw frame
```

<a name="error-handling"></a>
## Error Handling

Each layer has error handling:

### Host Layer
```cpp
HostError lastError = HostError::None;

bool AddWindowFrame(...) {
    if (!pMacOSWindow) {
        lastError = HostError::WindowCreationFailed;
        return false;
    }
    return true;
}
```

### Wrapper Layer
```cpp
void Window::create() {
    if (!pWindow) {
        throw std::runtime_error("Failed to create window");
    }
}
```

### Bridge Layer
```cpp
void window_create(Window* self) {
    if (!self) {
        printf("ERROR: Invalid window pointer\n");
        return;
    }
    // ... create window ...
}
```

<a name="memory-ownership"></a>
## Memory Ownership

Understanding who owns what prevents memory leaks:

```cpp
// Host Layer owns these (unique_ptr = exclusive ownership)
std::unique_ptr<Window> pMacOSWindow;
std::unique_ptr<EventHandler> pMacOSEventHandler;

// Host Layer shares this (shared_ptr = shared ownership)
std::shared_ptr<OpenGLRenderer> pMacOSOpenGLRenderer;

// Wrapper owns the C structures
std::unique_ptr<::Window> pWindow;      // C structure
std::unique_ptr<::Application> pApp;    // C structure

// C structures own Objective-C objects (must call release manually)
id nsWindow;    // NSWindow object
id delegate;    // Delegate object
```

<a name="takeaways"></a>
## Key Takeaways

1. **Three Layers**: Host → Wrapper → Bridge, each with a specific purpose
2. **Host Layer**: What your game sees, cross-platform interface
3. **Wrapper Layer**: Safe C++ classes using RAII
4. **Bridge Layer**: Talks directly to macOS using Objective-C runtime
5. **Main Thread**: All UI operations must happen there
6. **Memory Management**: AutoreleasePool for Objective-C, smart pointers for C++

## Next Steps

- Learn about the [Function Reference Guide](./02-Function-Reference.md)
- Learn about the [Architecture](./01-Architecture-Overview.md)

---

**Remember**: You can modify the Host layer for game-specific features, but usually don't need to touch the Bridge layer!

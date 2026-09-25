# Function Reference Guide

**Complete Function Flow Reference for olcPixelGameEngine3 macOS Implementation**

> This document traces every major function through all three layers, showing exactly how each operation flows from your game code to macOS and back.

---

## Table of Contents

1. [Application Lifecycle](#application-lifecycle)
2. [Window Management](#window-management)
3. [Event Handling](#event-handling)
4. [OpenGL Rendering](#opengl-rendering)
5. [Mouse and Cursor](#mouse-and-cursor)
6. [Image Loading](#image-loading)
7. [System Information](#system-information)

---

<a name="application-lifecycle"></a>
## Application Lifecycle

### Starting the Application

#### Flow Diagram
```
Your main() 
  → PGE.Start()
    → Host_Apple_MacOS::OnApplicationStart()
      → Host_Apple_MacOS::StartSystem()
        → Application wrapper constructor
          → application_init() [Bridge]
            → objc_msgSend(NSApplication, "sharedApplication")
```

#### Layer 1: Host (host_apple_macos.cpp)

```cpp
bool Host_Apple_MacOS::OnApplicationStart(olc::PixelGameEngine* pPrimary) {
    pPrimaryPGE = pPrimary;
    return true;
}

bool Host_Apple_MacOS::StartSystem() {
    // Create MacOS Application instance
    pMacApplication = std::make_unique<olc::apis::macos::Application>();

    // Set up application delegate event handlers
    MacApplicationEventsHandler();

    // Initialize and activate application
    pMacApplication->initialize();
    pMacApplication->activate();
    
    // Pre-context start hook
    pPrimaryPGE->OnPreContextStart();
    
    // Initialize the MacOS Window
    pMacOSWindow = std::make_unique<olc::apis::macos::Window>(
        frameBounds.width, 
        frameBounds.height, 
        "OLC PGE 3 MacOS Demo"
    );
    
    // ... setup event handlers and show window ...
    
    return true;
}
```

**Key Points:**
- `OnApplicationStart()` is called first, stores PGE pointer
- `StartSystem()` creates all macOS objects
- Must block until application exits
- Returns `true` on success

#### Layer 2: Wrapper (api_macos_wrapper.hpp)

```cpp
class Application {
private:
    struct ::Application* app_;  // C handle
    
public:
    explicit Application() : app_(nullptr) {
        app_ = application_init();
        if (!app_) {
            throw FrameworkException("Failed to initialize application");
        }
    }
    
    void initialize() noexcept {
        if (app_) application_initialize(app_);
    }
    
    void activate() noexcept {
        if (app_) application_activate(app_);
    }
    
    void run() noexcept {
        if (app_) application_run(app_);
    }
    
    ~Application() {
        if (app_) {
            application_destroy(app_);
            app_ = nullptr;
        }
    }
};
```

**Key Points:**
- Constructor calls `application_init()`
- Throws exception if initialization fails
- Destructor automatically cleans up
- Methods are simple wrappers around C functions

#### Layer 3: Bridge (api_macos.cpp)

```cpp
struct Application* application_init(void) {
    Application* self = new Application();
    
    // Get shared NSApplication instance
    Class NSAppClass = objc_getClass(kNSApplicationClass);
    id sharedApp = objc_msgSend(
        (id)NSAppClass, 
        sel_registerName(kSharedApplicationSel)
    );
    
    self->nsApplication = sharedApp;
    
    // Initialize callback pointers to null
    self->willFinishLaunchingCallback = nullptr;
    self->didFinishLaunchingCallback = nullptr;
    // ... etc ...
    
    return self;
}

void application_initialize(struct Application* self) {
    if (!self || !self->nsApplication) return;
    
    // Set activation policy to regular application
    objc_msgSend(
        self->nsApplication,
        sel_registerName(kSetActivationPolicySel),
        0  // NSApplicationActivationPolicyRegular
    );
}

void application_activate(struct Application* self) {
    if (!self || !self->nsApplication) return;
    
    // Activate the application
    objc_msgSend(
        self->nsApplication,
        sel_registerName(kActivateIgnoringOtherAppsSel),
        YES
    );
}
```

**Key Points:**
- Uses Objective-C runtime (`objc_msgSend`)
- Gets singleton NSApplication instance
- Stores callbacks for lifecycle events
- Sets activation policy for normal app

---

### Running the Application Loop

#### Flow Diagram
```
Host_Apple_MacOS::StartSystem()
  → pMacApplication->run()
    → application_run() [Bridge]
      → [NSApplication run]
        → Event loop starts
          → Events dispatched
```

#### Layer 1: Host

```cpp
bool Host_Apple_MacOS::StartSystem() {
    // ... window setup ...
    
    // Start system thread
    OnSystemThreadStart();
    
    // This blocks until app quits!
    pMacApplication->run();
    
    // Cleanup after app quits
    OnSystemThreadEnd();
    
    return true;
}
```

#### Layer 2: Wrapper

```cpp
void Application::run() noexcept {
    if (app_) application_run(app_);
}
```

#### Layer 3: Bridge

```cpp
void application_run(struct Application* self) {
    if (!self || !self->nsApplication) return;
    
    // Start the NSApplication run loop
    objc_msgSend(
        self->nsApplication,
        sel_registerName(kRunSel)
    );
}
```

**Key Points:**
- This function **blocks** until application quits
- macOS takes over the main thread
- Events are processed by macOS run loop

---

### Stopping the Application

#### Flow Diagram
```
User clicks close button
  → windowWillClose: delegate callback
    → Host_Apple_MacOS::CloseWindowFrame()
      → pWindow->olc_OnWindowClose()
        → PGE shutdown
          → application_stop() [Bridge]
            → [NSApplication stop:]
```

#### Layer 1: Host

```cpp
bool Host_Apple_MacOS::CloseWindowFrame(olc::Window* pWindow) {
    pWindow->olc_OnWindowClose();
    return true;
}

bool Host_Apple_MacOS::StopSystem() {
    if (pMacApplication) {
        pMacApplication->stop();
    }
    return true;
}
```

#### Layer 2: Wrapper

```cpp
void Application::stop() noexcept {
    if (app_) {
        application_stop(app_);
    }
}
```

#### Layer 3: Bridge

```cpp
void application_stop(struct Application* self) {
    if (!self || !self->nsApplication) return;
    
    // Stop the run loop
    objc_msgSend(
        self->nsApplication,
        sel_registerName(kStopSel),
        nil
    );
}
```

---

<a name="window-management"></a>
## Window Management

### Creating a Window

#### Flow Diagram
```
Host_Apple_MacOS::StartSystem()
  → new Window(width, height, title)
    → window_init() [Bridge]
      → Stores frame data
    → window->show(styleMask)
      → window_create() [Bridge]
        → objc_msgSend(NSWindow, "alloc")
        → objc_msgSend(window, "initWithContentRect:...")
      → window_show() [Bridge]
        → objc_msgSend(window, "makeKeyAndOrderFront:")
```

#### Layer 1: Host

```cpp
bool Host_Apple_MacOS::StartSystem() {
    // ... app setup ...
    
    // Create window
    pMacOSWindow = std::make_unique<olc::apis::macos::Window>(
        frameBounds.width, 
        frameBounds.height, 
        "OLC PGE 3 MacOS Demo"
    );
    
    pMacOSWindow->setPosition(frameBounds.x, frameBounds.y);
    pMacOSWindow->setContentViewPosition(0, 0);
    
    // ... event handler setup ...
    
    // Show window
    unsigned long styleMask = ConvertPGE2WindowStyle();
    pMacOSWindow->show(styleMask);
    
    return true;
}
```

#### Layer 2: Wrapper

```cpp
class Window {
private:
    struct ::Window* window_;
    
public:
    Window(double width, double height, const char* title) 
        : window_(nullptr) {
        // Initialize window
        window_ = window_init(0, 0, width, height);
        if (!window_) {
            throw FrameworkException("Failed to initialize window");
        }
        
        // Set title
        window_setTitle(window_, title);
    }
    
    void show(unsigned long styleMask) {
        if (!window_) return;
        
        // Create and show window
        window_create(window_, styleMask);
        window_show(window_);
    }
    
    ~Window() {
        if (window_) {
            window_destroy(window_);
            window_ = nullptr;
        }
    }
};
```

#### Layer 3: Bridge

```cpp
struct Window {
    id nsWindow;                // NSWindow object
    id contentView;             // NSView for content
    id windowDelegate;          // Delegate for events
    NSRect frame;               // Window frame
    // ... callback pointers ...
};

struct Window* window_init(double x, double y, double width, double height) {
    Window* self = new Window();
    
    // Store frame
    self->frame = NSRect{x, y, width, height};
    
    // Initialize to null
    self->nsWindow = nil;
    self->contentView = nil;
    self->windowDelegate = nil;
    
    // Initialize all callbacks to null
    self->keyDownCallback = nullptr;
    // ... etc ...
    
    return self;
}

void window_create(struct Window* self, unsigned long styleMask) {
    if (!self) return;
    
    // Get NSWindow class
    Class NSWindowClass = objc_getClass(kNSWindowClass);
    
    // Allocate window
    id window = objc_msgSend(
        (id)NSWindowClass,
        sel_registerName(kAllocSel)
    );
    
    // Initialize with frame and style
    window = objc_msgSend(
        window,
        sel_registerName(kInitWithContentRectSel),
        self->frame,
        styleMask,
        2,   // NSBackingStoreBuffered
        NO   // defer
    );
    
    self->nsWindow = window;
    
    // Set window to accept mouse moved events
    objc_msgSend(
        window,
        sel_registerName(kSetAcceptsMouseMovedEventsSel),
        YES
    );
}

void window_show(struct Window* self) {
    if (!self || !self->nsWindow) return;
    
    // Make window visible and key
    objc_msgSend(
        self->nsWindow,
        sel_registerName(kMakeKeyAndOrderFrontSel),
        nil
    );
}
```

**Key Points:**
- Window is created in two steps: `init` then `create`
- `init` just stores parameters
- `create` actually allocates NSWindow
- `show` makes it visible
- Style mask controls window appearance (titlebar, buttons, etc.)

---

### Updating Window Title

#### Flow Diagram
```
Your game: SetWindowTitle("New Title")
  → Host_Apple_MacOS::UpdateWindowFrameTitle()
    → dispatch_async(main_queue)
      → pMacOSWindow->setTitle()
        → window_setTitle() [Bridge]
          → objc_msgSend(window, "setTitle:")
```

#### Layer 1: Host

```cpp
bool Host_Apple_MacOS::UpdateWindowFrameTitle(olc::Window* pWindow) {
    if (!pMacOSWindow) return false;
    
    // Must run on main thread!
    dispatch_async(dispatch_get_main_queue(), ^{
        pMacOSWindow->setTitle(pWindow->GetWindowTitle().c_str());
    });
    
    return true;
}
```

**Key Point:** Uses GCD to ensure UI update happens on main thread

#### Layer 2: Wrapper

```cpp
void Window::setTitle(const char* title) {
    if (!window_) return;
    window_setTitle(window_, title);
}
```

#### Layer 3: Bridge

```cpp
void window_setTitle(struct Window* self, const char* title) {
    if (!self || !self->nsWindow || !title) return;
    
    // Convert C string to NSString
    Class NSStringClass = objc_getClass(kNSStringClass);
    id nsTitle = objc_msgSend(
        (id)NSStringClass,
        sel_registerName(kStringWithUTF8StringSel),
        title
    );
    
    // Set window title
    objc_msgSend(
        self->nsWindow,
        sel_registerName(kSetTitleSel),
        nsTitle
    );
}
```

**Key Points:**
- Must convert C string to NSString
- NSString is created using `stringWithUTF8String:`
- Then set using `setTitle:`

---

### Toggling Fullscreen

#### Layer 1: Host

```cpp
bool Host_Apple_MacOS::SetFullScreen(olc::Window* pWindow, const bool bFullScreen) {
    // Check if already in requested state
    if(pMacOSWindow->isFullScreen() == bFullScreen)
        return true;

    // Toggle fullscreen
    dispatch_async(dispatch_get_main_queue(), ^{
        pMacOSWindow->toggleFullScreen();
    });
    
    return true;
}
```

#### Layer 2: Wrapper

```cpp
void Window::toggleFullScreen() {
    if (!window_) return;
    window_toggleFullScreen(window_);
}

bool Window::isFullScreen() const {
    if (!window_) return false;
    return window_isFullScreen(window_);
}
```

#### Layer 3: Bridge

```cpp
void window_toggleFullScreen(struct Window* self) {
    if (!self || !self->nsWindow) return;
    
    // Toggle fullscreen mode
    objc_msgSend(
        self->nsWindow,
        sel_registerName(kToggleFullScreenSel),
        nil
    );
}

bool window_isFullScreen(struct Window* self) {
    if (!self || !self->nsWindow) return false;
    
    // Get window style mask
    unsigned long styleMask = (unsigned long)objc_msgSend(
        self->nsWindow,
        sel_registerName(kStyleMaskSel)
    );
    
    // Check if fullscreen bit is set (bit 14)
    return (styleMask & (1 << 14)) != 0;
}
```

---

<a name="event-handling"></a>
## Event Handling

### Keyboard Events (Key Down)

#### Flow Diagram
```
User presses key
  → macOS sends NSEvent
    → keyDown: delegate method
      → keyDown_handler() [Bridge]
        → Extracts key code, characters, modifiers
        → Calls window->keyDownCallback()
          → Lambda in EventHandler [Wrapper]
            → Calls handler function [Wrapper]
              → MacEventsHandler lambda [Host]
                → Updates PGE key state
```

#### Layer 1: Host

```cpp
void Host_Apple_MacOS::MacEventsHandler() {
    // Set up key down handler
    pMacOSEventHandler->setKeyDownHandler(
        [this](uint16_t keyCode, const char* characters, uint32_t modifierFlags) {
            // Map macOS key code to PGE key
            if (mapKeys.count(keyCode)) {
                pPGEwindow->olc_UpdateKeyState(mapKeys[keyCode], true);
            }
            
            // Handle modifier keys
            bool bShiftPressed = (modifierFlags & NSEventModifierFlagShift);
            bool bCtrlPressed = (modifierFlags & NSEventModifierFlagControl);
            bool bCmdPressed = (modifierFlags & NSEventModifierFlagCommand);
            
            pPGEwindow->olc_UpdateKeyState(Key::SHIFT, bShiftPressed);
            pPGEwindow->olc_UpdateKeyState(Key::CTRL, bCtrlPressed);
        }
    );
    
    // Similar for key up...
}
```

**Key Points:**
- Lambda captures `this` to access host members
- Converts macOS key codes to PGE keys using `mapKeys`
- Handles modifier keys separately
- Updates PGE window key state

#### Layer 2: Wrapper

```cpp
class EventHandler {
private:
    struct ::Window* window_;
    std::function<void(uint16_t, const char*, uint32_t)> keyDownHandler_;
    
public:
    void setKeyDownHandler(std::function<void(uint16_t, const char*, uint32_t)> handler) {
        keyDownHandler_ = std::move(handler);
        
        // Set C callback
        window_setKeyDownCallback(
            window_,
            [](unsigned short keyCode, const char* characters, 
               unsigned int modifierFlags, void* userData) {
                // Call stored C++ function
                auto* handler = static_cast<
                    std::function<void(uint16_t, const char*, uint32_t)>*
                >(userData);
                (*handler)(keyCode, characters, modifierFlags);
            },
            &keyDownHandler_
        );
    }
};
```

**Key Points:**
- Stores C++ function in member variable
- Creates C-compatible lambda as bridge
- Passes function address as `userData`
- Lambda casts `userData` back and calls function

#### Layer 3: Bridge

```cpp
void window_setKeyDownCallback(struct Window* self, KeyEventCallback callback, void* userData) {
    if (!self) return;
    
    self->keyDownCallback = callback;
    self->keyDownUserData = userData;
}

// This is called by macOS when key is pressed
static void keyDown_handler(id self, SEL _cmd, id event) {
    // Get window from self
    Window* window = getWindowFromSelf(self);
    if (!window || !window->keyDownCallback) return;
    
    // Extract key code
    unsigned short keyCode = (unsigned short)objc_msgSend(
        event,
        sel_registerName(kKeyCodeSel)
    );
    
    // Extract characters
    id charactersObj = objc_msgSend(
        event,
        sel_registerName(kCharactersSel)
    );
    const char* characters = (const char*)objc_msgSend(
        charactersObj,
        sel_registerName(kUTF8StringSel)
    );
    
    // Extract modifier flags
    unsigned int modifierFlags = (unsigned int)objc_msgSend(
        event,
        sel_registerName(kModifierFlagsSel)
    );
    
    // Call the callback
    window->keyDownCallback(keyCode, characters, modifierFlags, window->keyDownUserData);
}
```

**Key Points:**
- `keyDown_handler` is registered as Objective-C method
- Extracts data from NSEvent using `objc_msgSend`
- Calls stored C callback with extracted data
- `getWindowFromSelf` retrieves Window* from delegate

---

### Mouse Events (Mouse Down)

#### Flow Diagram
```
User clicks mouse
  → macOS sends NSEvent
    → mouseDown: delegate method
      → mouseDown_handler() [Bridge]
        → Extracts position, button, modifiers
        → Calls window->mouseDownCallback()
          → Lambda in EventHandler [Wrapper]
            → Calls handler function [Wrapper]
              → MacEventsHandler lambda [Host]
                → Updates PGE mouse state
```

#### Layer 1: Host

```cpp
void Host_Apple_MacOS::MacEventsHandler() {
    // Mouse button down
    pMacOSEventHandler->setMouseDownHandler(
        [this](double x, double y, int button, uint32_t modifierFlags) {
            pPGEwindow->olc_UpdateMouseState(button, true);
            pPGEwindow->olc_UpdateMouse(x, y);
        }
    );
    
    // Mouse button up
    pMacOSEventHandler->setMouseUpHandler(
        [this](double x, double y, int button, uint32_t modifierFlags) {
            pPGEwindow->olc_UpdateMouseState(button, false);
            pPGEwindow->olc_UpdateMouse(x, y);
        }
    );
    
    // Mouse moved (no button pressed)
    pMacOSEventHandler->setMouseMovedHandler(
        [this](double x, double y, int button, uint32_t modifierFlags) {
            pPGEwindow->olc_UpdateMouse(x, y);
        }
    );
}
```

#### Layer 2: Wrapper

```cpp
void EventHandler::setMouseDownHandler(
    std::function<void(double, double, int, uint32_t)> handler) {
    
    mouseDownHandler_ = std::move(handler);
    
    window_setMouseDownCallback(
        window_,
        [](double x, double y, int button, unsigned int flags, void* userData) {
            auto* h = static_cast<
                std::function<void(double, double, int, uint32_t)>*
            >(userData);
            (*h)(x, y, button, flags);
        },
        &mouseDownHandler_
    );
}
```

#### Layer 3: Bridge

```cpp
static void mouseDown_handler(id self, SEL _cmd, id event) {
    Window* window = getWindowFromSelf(self);
    if (!window || !window->mouseDownCallback) return;
    
    // Get mouse location in window
    CGPoint location = {0, 0};
    id locationObj = objc_msgSend(
        event,
        sel_registerName(kLocationInWindowSel)
    );
    // Extract x and y from NSPoint
    memcpy(&location, &locationObj, sizeof(CGPoint));
    
    // Convert to backing (retina) coordinates
    id contentView = objc_msgSend(
        window->nsWindow,
        sel_registerName(kContentViewSel)
    );
    
    CGPoint backingLocation = {0, 0};
    id backingPoint = objc_msgSend(
        contentView,
        sel_registerName(kConvertPointToBackingSel),
        locationObj
    );
    memcpy(&backingLocation, &backingPoint, sizeof(CGPoint));
    
    // Get button number
    int buttonNumber = (int)objc_msgSend(
        event,
        sel_registerName(kButtonNumberSel)
    );
    
    // Get modifier flags
    unsigned int modifierFlags = (unsigned int)objc_msgSend(
        event,
        sel_registerName(kModifierFlagsSel)
    );
    
    // Flip Y coordinate (macOS origin is bottom-left, we want top-left)
    double height = window->frame.height;
    double flippedY = height - backingLocation.y;
    
    // Call callback
    window->mouseDownCallback(
        backingLocation.x,
        flippedY,
        buttonNumber,
        modifierFlags,
        window->mouseDownUserData
    );
}
```

**Key Points:**
- Mouse coordinates need conversion for Retina displays
- Y coordinate needs flipping (macOS uses bottom-left origin)
- Button number: 0 = left, 1 = right, 2 = middle

---

<a name="opengl-rendering"></a>
## OpenGL Rendering

### Creating OpenGL Context

#### Flow Diagram
```
Host_Apple_MacOS::GetHostWindowDescriptor()
  → CreateCGLContextObj()
    → new OpenGLRenderer()
      → opengl_init() [Bridge]
    → renderer->initialize(window)
      → opengl_initialize() [Bridge]
        → Creates NSOpenGLView
        → Sets pixel format
        → Creates NSOpenGLContext
    → renderer->setupContext()
      → opengl_setupContext() [Bridge]
        → Makes context current
        → Sets up vsync
```

#### Layer 1: Host

```cpp
std::vector<void*> Host_Apple_MacOS::GetHostWindowDescriptor(olc::Window* pWindow) {
    // Create OpenGL context if needed
    if(pMacOSOpenGLRenderer == nullptr)
        CreateCGLContextObj();
    
    return vMacOSWindowDescriptors;
}

void Host_Apple_MacOS::CreateCGLContextObj() {
    // Create OpenGL renderer
    pMacOSOpenGLRenderer = std::make_shared<olc::apis::macos::OpenGLRenderer>();
    
    // Initialize with window
    pMacOSOpenGLRenderer->initialize(*pMacOSWindow);
    
    // Setup OpenGL context
    pMacOSOpenGLRenderer->setupContext();
    
    // Get CGL context object for PGE
    pMacGLConextObj = pMacOSOpenGLRenderer->getCGLContextObjPtr();
    
    // Store in descriptor list
    vMacOSWindowDescriptors.clear();
    vMacOSWindowDescriptors.push_back(pMacGLConextObj);
}
```

#### Layer 2: Wrapper

```cpp
class OpenGLRenderer {
private:
    struct ::OpenGLRenderer* renderer_;
    
public:
    OpenGLRenderer() : renderer_(nullptr) {
        renderer_ = opengl_init();
        if (!renderer_) {
            throw FrameworkException("Failed to initialize OpenGL renderer");
        }
    }
    
    void initialize(const Window& window) {
        if (!renderer_) return;
        opengl_initialize(renderer_, window.getCHandle());
    }
    
    void setupContext() {
        if (!renderer_) return;
        opengl_setupContext(renderer_);
    }
    
    void* getCGLContextObjPtr() {
        if (!renderer_) return nullptr;
        return opengl_getCGLContextObjPtr(renderer_);
    }
    
    ~OpenGLRenderer() {
        if (renderer_) {
            opengl_destroy(renderer_);
            renderer_ = nullptr;
        }
    }
};
```

#### Layer 3: Bridge

```cpp
struct OpenGLRenderer {
    id glView;              // NSOpenGLView
    id glContext;           // NSOpenGLContext
    id pixelFormat;         // NSOpenGLPixelFormat
    void* cglContextObj;    // CGLContextObj
};

struct OpenGLRenderer* opengl_init(void) {
    OpenGLRenderer* self = new OpenGLRenderer();
    
    self->glView = nil;
    self->glContext = nil;
    self->pixelFormat = nil;
    self->cglContextObj = nullptr;
    
    return self;
}

void opengl_initialize(struct OpenGLRenderer* self, struct Window* window) {
    if (!self || !window || !window->nsWindow) return;
    
    // Define OpenGL pixel format attributes
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        0
    };
    
    // Create pixel format
    Class NSOpenGLPixelFormatClass = objc_getClass(kNSOpenGLPixelFormatClass);
    id pixelFormat = objc_msgSend(
        (id)NSOpenGLPixelFormatClass,
        sel_registerName(kAllocSel)
    );
    pixelFormat = objc_msgSend(
        pixelFormat,
        sel_registerName(kInitWithAttributesSel),
        attrs
    );
    
    self->pixelFormat = pixelFormat;
    
    // Get content view frame
    id contentView = objc_msgSend(
        window->nsWindow,
        sel_registerName(kContentViewSel)
    );
    
    CGRect frame = {0, 0, 0, 0};
    id frameObj = objc_msgSend(
        contentView,
        sel_registerName(kBoundsSel)
    );
    memcpy(&frame, &frameObj, sizeof(CGRect));
    
    // Create NSOpenGLView
    Class NSOpenGLViewClass = objc_getClass(kNSOpenGLViewClass);
    id glView = objc_msgSend(
        (id)NSOpenGLViewClass,
        sel_registerName(kAllocSel)
    );
    glView = objc_msgSend(
        glView,
        sel_registerName(kInitWithFramePixelFormatSel),
        frame,
        pixelFormat
    );
    
    self->glView = glView;
    
    // Set as content view
    objc_msgSend(
        window->nsWindow,
        sel_registerName(kSetContentViewSel),
        glView
    );
    
    // Get OpenGL context
    id glContext = objc_msgSend(
        glView,
        sel_registerName(kOpenGLContextSel)
    );
    
    self->glContext = glContext;
}

void opengl_setupContext(struct OpenGLRenderer* self) {
    if (!self || !self->glContext) return;
    
    // Make context current
    objc_msgSend(
        self->glContext,
        sel_registerName(kMakeCurrentContextSel)
    );
    
    // Get CGL context object
    void* cglContext = (void*)objc_msgSend(
        self->glContext,
        sel_registerName(kCGLContextObjSel)
    );
    
    self->cglContextObj = cglContext;
}
```

**Key Points:**
- Pixel format specifies OpenGL version and capabilities
- NSOpenGLView handles OpenGL rendering
- CGL context is needed by PGE's OpenGL code
- Context must be made current before drawing

---

### Enabling VSync

#### Layer 1: Host

```cpp
bool Host_Apple_MacOS::SyncWithDesktopComposite() {
    // Only enable once
    if(!enableVSync) {
        pMacOSOpenGLRenderer->enableVsync();
        enableVSync = true;
    }
    return enableVSync;
}
```

#### Layer 2: Wrapper

```cpp
void OpenGLRenderer::enableVsync() {
    if (!renderer_) return;
    opengl_setVsync(renderer_, true);
}
```

#### Layer 3: Bridge

```cpp
void opengl_setVsync(struct OpenGLRenderer* self, BOOL enabled) {
    if (!self || !self->glContext) return;
    
    // Set swap interval
    GLint swapInterval = enabled ? 1 : 0;
    objc_msgSend(
        self->glContext,
        sel_registerName(kSetValuesSel),
        &swapInterval,
        222  // NSOpenGLCPSwapInterval
    );
}
```

**Key Points:**
- Swap interval 1 = sync to display refresh
- Swap interval 0 = no sync (unlimited FPS)
- Only needs to be set once

---

<a name="mouse-and-cursor"></a>
## Mouse and Cursor

### Setting Mouse Position

#### Layer 1: Host

```cpp
bool Host_Apple_MacOS::SetMousePosition(olc::Window* pWindow, const olc::vi2d& vPos) {
    // Must run on main thread, and wait for completion
    dispatch_sync(dispatch_get_main_queue(), ^{
        pMacOSWindow->setCursorPosition(vPos.x, vPos.y);
    });
    return true;
}
```

**Key Point:** Uses `dispatch_sync` to wait for position to be set

#### Layer 2: Wrapper

```cpp
void Window::setCursorPosition(double x, double y) {
    if (!window_) return;
    window_setCursorPosition(window_, x, y);
}
```

#### Layer 3: Bridge

```cpp
void window_setCursorPosition(struct Window* self, double x, double y) {
    if (!self || !self->nsWindow) return;
    
    // Get content view
    id contentView = objc_msgSend(
        self->nsWindow,
        sel_registerName(kContentViewSel)
    );
    
    // Flip Y coordinate
    double height = self->frame.height;
    double flippedY = height - y;
    
    // Convert to window coordinates
    CGPoint windowPoint = {x, flippedY};
    
    // Convert to screen coordinates
    CGPoint screenPoint = {0, 0};
    id screenPointObj = objc_msgSend(
        self->nsWindow,
        sel_registerName("convertPointToScreen:"),
        windowPoint
    );
    memcpy(&screenPoint, &screenPointObj, sizeof(CGPoint));
    
    // Move cursor
    CGWarpMouseCursorPosition(screenPoint);
}
```

**Key Points:**
- Must flip Y coordinate
- Convert window coords to screen coords
- Uses CoreGraphics function `CGWarpMouseCursorPosition`

---

### Hiding/Showing Cursor

#### Layer 1: Host

```cpp
bool Host_Apple_MacOS::SetMouseVisible(olc::Window* pWindow, const bool bVisible) {
    dispatch_sync(dispatch_get_main_queue(), ^{
        pMacOSWindow->setCursorVisibility(bVisible);
    });
    return true;
}
```

#### Layer 2: Wrapper

```cpp
void Window::setCursorVisibility(bool visible) {
    if (!window_) return;
    window_setCursorVisibility(window_, visible);
}
```

#### Layer 3: Bridge

```cpp
void window_setCursorVisibility(struct Window* self, BOOL visible) {
    if (!self) return;
    
    // Get NSCursor class
    Class NSCursorClass = objc_getClass(kNSCursorClass);
    
    if (visible) {
        // Check if currently hidden
        BOOL isHidden = (BOOL)objc_msgSend(
            (id)NSCursorClass,
            sel_registerName(kIsHiddenSel)
        );
        
        if (isHidden && bAllowHideCursor) {
            objc_msgSend(
                (id)NSCursorClass,
                sel_registerName(kUnhideSel)
            );
            bHideCursor = false;
        }
    } else {
        if (bAllowHideCursor && !bHideCursor) {
            objc_msgSend(
                (id)NSCursorClass,
                sel_registerName(kHideSel)
            );
            bHideCursor = true;
        }
    }
}
```

**Key Points:**
- NSCursor is a class method (no instance needed)
- Tracks hidden state to avoid multiple hide calls
- Has flags to prevent unwanted hiding

---

<a name="image-loading"></a>
## Image Loading

### Loading Image from File

#### Flow Diagram
```
PGE loads sprite
  → ImageLoader constructor
    → imageloader_init() [Bridge]
  → loader.loadFromFile(path)
    → imageloader_loadFromFile() [Bridge]
      → CGImageSourceCreateWithURL
      → CGImageSourceCreateImageAtIndex
      → CGBitmapContextCreate
      → CGContextDrawImage
      → Extract pixel data
```

#### Layer 2: Wrapper

```cpp
class ImageLoader {
private:
    struct ::ImageLoader* loader_;
    
public:
    ImageLoader() : loader_(nullptr) {
        loader_ = imageloader_init();
        if (!loader_) {
            throw FrameworkException("Failed to initialize image loader");
        }
    }
    
    bool loadFromFile(const std::string& filePath) {
        if (!loader_) return false;
        return imageloader_loadFromFile(loader_, filePath.c_str());
    }
    
    std::vector<uint8_t> getPixelData() const {
        if (!loader_) return {};
        
        int width, height, bpp;
        imageloader_getImageInfo(loader_, &width, &height, &bpp);
        
        unsigned char* data = imageloader_getPixelData(loader_);
        if (!data) return {};
        
        size_t size = width * height * bpp;
        return std::vector<uint8_t>(data, data + size);
    }
};
```

#### Layer 3: Bridge

```cpp
BOOL imageloader_loadFromFile(struct ImageLoader* self, const char* filePath) {
    if (!self || !filePath) return NO;
    
    // Create CFURL from file path
    CFStringRef pathString = CFStringCreateWithCString(
        kCFAllocatorDefault,
        filePath,
        kCFStringEncodingUTF8
    );
    
    CFURLRef url = CFURLCreateWithFileSystemPath(
        kCFAllocatorDefault,
        pathString,
        kCFURLPOSIXPathStyle,
        false
    );
    
    CFRelease(pathString);
    
    // Create image source
    CGImageSourceRef imageSource = CGImageSourceCreateWithURL(url, NULL);
    CFRelease(url);
    
    if (!imageSource) return NO;
    
    // Create image from source
    CGImageRef image = CGImageSourceCreateImageAtIndex(imageSource, 0, NULL);
    CFRelease(imageSource);
    
    if (!image) return NO;
    
    // Get image dimensions
    size_t width = CGImageGetWidth(image);
    size_t height = CGImageGetHeight(image);
    
    // Create RGBA bitmap context
    size_t bytesPerPixel = 4;
    size_t bytesPerRow = width * bytesPerPixel;
    size_t totalBytes = height * bytesPerRow;
    
    self->pixelData = (unsigned char*)malloc(totalBytes);
    if (!self->pixelData) {
        CGImageRelease(image);
        return NO;
    }
    
    // Create bitmap context
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(
        self->pixelData,
        width,
        height,
        8,                              // bits per component
        bytesPerRow,
        colorSpace,
        kCGImageAlphaPremultipliedLast  // RGBA format
    );
    
    CGColorSpaceRelease(colorSpace);
    
    if (!context) {
        free(self->pixelData);
        self->pixelData = NULL;
        CGImageRelease(image);
        return NO;
    }
    
    // Draw image into context (this extracts pixel data)
    CGRect rect = {{0, 0}, {(CGFloat)width, (CGFloat)height}};
    CGContextDrawImage(context, rect, image);
    
    CGContextRelease(context);
    CGImageRelease(image);
    
    // Store image info
    self->width = (int)width;
    self->height = (int)height;
    self->bytesPerPixel = (int)bytesPerPixel;
    self->bytesPerRow = (int)bytesPerRow;
    self->hasAlpha = YES;
    self->isLoaded = YES;
    
    return YES;
}
```

**Key Points:**
- Uses CoreGraphics image loading
- Creates bitmap context to extract pixels
- Converts to RGBA format
- Handles memory management carefully

---

<a name="system-information"></a>
## System Information

### Getting Keyboard Layout

#### Layer 1: Host

```cpp
olc::KeyboardLayout Host_Apple_MacOS::GetKeyboardLayout() const {
    std::string locale = pMacApplication->getSystemLocale();
    
    // Map locale to keyboard layout
    if (locale.find("en_US") != std::string::npos)
        return KeyboardLayout::US;
    else if (locale.find("en_GB") != std::string::npos)
        return KeyboardLayout::UK;
    else if (locale.find("de") != std::string::npos)
        return KeyboardLayout::DE;
    // ... etc ...
    
    return KeyboardLayout::UK;  // Default
}
```

#### Layer 2: Wrapper

```cpp
std::string Application::getSystemLocale() const {
    const char* localeC = application_getSystemLocale(app_);
    return localeC ? std::string(localeC) : std::string("en_GB");
}
```

#### Layer 3: Bridge

```cpp
const char* application_getSystemLocale(struct Application* self) {
    if (!self) return nullptr;
    
    // Get NSLocale class
    Class NSLocaleClass = objc_getClass(kNSLocaleClass);
    
    // Get current locale
    id currentLocale = objc_msgSend(
        (id)NSLocaleClass,
        sel_registerName(kCurrentLocaleSel)
    );
    
    // Get locale identifier
    id localeId = objc_msgSend(
        currentLocale,
        sel_registerName(kLocaleIdentifierSel)
    );
    
    // Convert to C string
    const char* localeStr = (const char*)objc_msgSend(
        localeId,
        sel_registerName(kUTF8StringSel)
    );
    
    return localeStr;
}
```

---

## Summary

This reference guide shows the complete flow of every major operation through all three layers:

1. **Host Layer** - Manages and coordinates
2. **Wrapper Layer** - Provides C++ interface
3. **Bridge Layer** - Calls macOS APIs

**Key Patterns:**
- ✅ Host uses wrapper classes
- ✅ Wrapper calls C bridge functions  
- ✅ Bridge uses Objective-C runtime
- ✅ Memory managed with RAII
- ✅ UI operations on main thread
- ✅ Callbacks connect layers

**Next Steps:**
- Learn about the [Objective-C Runtime](./03-Objective-C-Runtime.md)
- Learn about the [Function Reference Guide](./02-Function-Reference.md)
- Learn about the [Architecture](./01-Architecture-Overview.md)

---

**Remember:** Each layer has a specific job. Understanding how they work together makes the system much clearer! 🎮

# Challenge Solutions

**Complete Solutions with Detailed Explanations**

> This guide provides full solutions to the challenge projects from [05-Practical-Examples.md](./05-Practical-Examples.md), along with detailed explanations of why each solution works and what concepts it demonstrates.

---

## Table of Contents

1. [Challenge 1: Implement Window Minimize](#challenge1)
2. [Challenge 2: Add Drag-and-Drop Support](#challenge2)
3. [Challenge 3: Custom Cursor Images](#challenge3)
4. [Challenge 4: Fullscreen Transitions](#challenge4)
5. [Challenge 5: Multi-Window Support](#challenge5)

---

<a name="challenge1"></a>
## Challenge 1: Implement Window Minimize

### The Challenge

**Requirements:**
- Detect when window is minimized
- Pause game when minimized
- Resume when restored

### Complete Solution

All code goes in `olcPixelGameEngine3.h`. Search for the appropriate sections and add the code as indicated.

#### Step 1: Add Callback to Window Struct

Search for `struct Window` in the bridge layer:

```cpp
struct Window {
    // ... existing members ...
    
    // Add minimize callbacks
    void (*windowDidMiniaturizeCallback)(void* userData);
    void* windowDidMiniaturizeUserData;
    
    void (*windowDidDeminiaturizeCallback)(void* userData);
    void* windowDidDeminiaturizeUserData;
};
```

**Why this works:**
- We need separate callbacks for minimize and restore events
- C function pointers allow the bridge layer to call back into C++
- User data pointer lets us pass context (like `this` pointer)

---

#### Step 2: Create Event Handlers

Search for other delegate handlers (like `windowDidResize_handler`) and add nearby:

```cpp
static void windowDidMiniaturize_handler(id self, SEL _cmd, id notification) {
    Window* window = getWindowFromSelf(self);
    if (!window || !window->windowDidMiniaturizeCallback) return;
    
    // Call the callback
    window->windowDidMiniaturizeCallback(window->windowDidMiniaturizeUserData);
}

static void windowDidDeminiaturize_handler(id self, SEL _cmd, id notification) {
    Window* window = getWindowFromSelf(self);
    if (!window || !window->windowDidDeminiaturizeCallback) return;
    
    // Call the callback
    window->windowDidDeminiaturizeCallback(window->windowDidDeminiaturizeUserData);
}
```

**Why this works:**
- These are called by macOS when minimize/restore happens
- `getWindowFromSelf(self)` retrieves our Window struct from the delegate
- We check for null before calling to prevent crashes
- The notification parameter provides info but we don't need it here

---

#### Step 3: Register Handlers with Delegate

Search for `createWindowDelegateClass` and add methods:

```cpp
// Inside the createWindowDelegateClass function, after other class_addMethod calls:

class_addMethod(
    delegateClass,
    sel_registerName("windowDidMiniaturize:"),
    (IMP)windowDidMiniaturize_handler,
    "v@:@"  // void return, object, selector, object (notification)
);

class_addMethod(
    delegateClass,
    sel_registerName("windowDidDeminiaturize:"),
    (IMP)windowDidDeminiaturize_handler,
    "v@:@"
);
```

**Why this works:**
- `class_addMethod` dynamically adds methods to our delegate class
- The selector names (`windowDidMiniaturize:`) are standard macOS delegate methods
- `"v@:@"` is the type encoding: void return, self, selector, notification
- macOS will automatically call these methods when the window state changes

---

#### Step 4: Add C API Functions

Search for the C API section (near other `window_` functions):

```cpp
// Callback type definitions
typedef void (*WindowMiniaturizeCallback)(void* userData);
typedef void (*WindowDeminiaturizeCallback)(void* userData);

// Set minimize callback
void window_setMiniaturizeCallback(struct Window* self,
                                    WindowMiniaturizeCallback callback,
                                    void* userData) {
    if (!self) return;
    self->windowDidMiniaturizeCallback = callback;
    self->windowDidMiniaturizeUserData = userData;
}

// Set deminiaturize (restore) callback
void window_setDeminiaturizeCallback(struct Window* self,
                                      WindowDeminiaturizeCallback callback,
                                      void* userData) {
    if (!self) return;
    self->windowDidDeminiaturizeCallback = callback;
    self->windowDidDeminiaturizeUserData = userData;
}
```

**Why this works:**
- These are pure C functions that can be called from C++
- They store the callbacks in the Window struct
- The null check prevents crashes if called incorrectly

---

#### Step 5: Add C++ Wrapper Methods

Search for the `EventHandler` class in `namespace olc::apis::macos`:

```cpp
class EventHandler {
public:
    // ... existing methods ...
    
    void setWindowMiniaturizeHandler(std::function<void()> handler) {
        miniaturizeHandler_ = std::move(handler);
        
        window_setMiniaturizeCallback(
            window_,
            [](void* userData) {
                auto* h = static_cast<std::function<void()>*>(userData);
                if (h) (*h)();
            },
            &miniaturizeHandler_
        );
    }
    
    void setWindowDeminiaturizeHandler(std::function<void()> handler) {
        deminiaturizeHandler_ = std::move(handler);
        
        window_setDeminiaturizeCallback(
            window_,
            [](void* userData) {
                auto* h = static_cast<std::function<void()>*>(userData);
                if (h) (*h)();
            },
            &deminiaturizeHandler_
        );
    }
    
private:
    // ... existing members ...
    std::function<void()> miniaturizeHandler_;
    std::function<void()> deminiaturizeHandler_;
};
```

**Why this works:**
- C++ lambdas are more convenient than raw function pointers
- `std::move` transfers ownership efficiently
- The static lambda bridges C callbacks to C++ std::function
- We store the handler as a member to keep it alive

---

#### Step 6: Add to Host Layer

Search for `Host_Apple_MacOS` class and add a member variable:

```cpp
class Host_Apple_MacOS : public olc::host::Host {
private:
    // ... existing members ...
    std::atomic<bool> bGamePaused{false};
};
```

**Why use atomic:**
- The minimize callback runs on the main thread
- The game loop may run on a different thread
- `std::atomic<bool>` provides thread-safe access without locks
- It's a simple flag, so atomic is perfect (vs. mutex overhead)

---

#### Step 7: Set Up Event Handlers

Search for `MacEventsHandler` method and add:

```cpp
void Host_Apple_MacOS::MacEventsHandler() {
    // ... existing event handlers ...
    
    // Handle window minimize
    pMacOSEventHandler->setWindowMiniaturizeHandler([this]() {
        std::cout << "Window minimized - pausing game" << std::endl;
        bGamePaused = true;
    });
    
    // Handle window restore
    pMacOSEventHandler->setWindowDeminiaturizeHandler([this]() {
        std::cout << "Window restored - resuming game" << std::endl;
        bGamePaused = false;
    });
}
```

**Why this works:**
- Lambdas capture `this` to access the host's members
- Simple flag setting - no complex logic needed
- Console output helps with debugging
- The atomic bool is thread-safe

---

#### Step 8: Use in Game Loop

Search for the game loop (typically in `OnSystemTick` or similar):

```cpp
bool Host_Apple_MacOS::OnSystemTick() override {
    // Check if paused
    if (bGamePaused) {
        // Don't update game, but keep event loop running
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        return true;
    }
    
    // Normal game update
    // ... existing update code ...
    
    return true;
}
```

**Why this works:**
- The sleep prevents busy-waiting (CPU spinning)
- 16ms ≈ 60 FPS, keeps the loop responsive
- We still return true to keep the application running
- Game logic doesn't run, but the app stays alive

---

### Complete Understanding

**Flow when minimizing:**
1. User clicks minimize button
2. macOS calls `windowDidMiniaturize:` on our delegate
3. Our handler calls the C function pointer
4. C function calls the C++ lambda
5. Lambda sets `bGamePaused = true`
6. Game loop checks flag and sleeps instead of updating

**Flow when restoring:**
1. User clicks window in dock
2. macOS calls `windowDidDeminiaturize:` on our delegate
3. Same chain as above
4. Lambda sets `bGamePaused = false`
5. Game loop resumes normal updates

**Why this design is correct:**
- ✅ Thread-safe with atomic bool
- ✅ No memory leaks (RAII everywhere)
- ✅ Doesn't crash on rapid minimize/restore
- ✅ Game state is preserved (not destroyed)
- ✅ Follows the same pattern as other events
- ✅ Easy to extend (could add pause menu, etc.)

---

<a name="challenge2"></a>
## Challenge 2: Add Drag-and-Drop Support

### The Challenge

**Requirements:**
- Accept file drops on window
- Get file paths
- Load images/files in game

### Complete Solution

This is more complex as we need to implement the `NSDraggingDestination` protocol.

#### Step 1: Add Drag Callback to Window Struct

```cpp
struct Window {
    // ... existing members ...
    
    // Drag-and-drop callback
    void (*draggedFilesCallback)(const char** filePaths, int count, void* userData);
    void* draggedFilesUserData;
};
```

**Why this works:**
- Array of C strings for file paths
- Count tells us how many files were dropped
- Simple C types cross the C/C++ boundary easily

---

#### Step 2: Create Dragging Destination View Class

Add this near other class creation functions:

```cpp
static Class createDraggingDestinationViewClass() {
    static Class viewClass = nil;
    
    if (viewClass) return viewClass;
    
    // Create subclass of NSView
    Class NSViewClass = objc_getClass("NSView");
    viewClass = objc_allocateClassPair(NSViewClass, "PGEDraggingView", 0);
    
    if (!viewClass) return nil;
    
    // Add ivar to store Window pointer
    class_addIvar(viewClass, "pgeWindow", sizeof(void*), 
                  log2(sizeof(void*)), "^v");
    
    // Register dragging types
    SEL registerForDraggedTypesSel = sel_registerName("registerForDraggedTypes:");
    
    // Add dragging methods
    class_addMethod(viewClass, 
                   sel_registerName("draggingEntered:"),
                   (IMP)draggingEntered_handler,
                   "q@:@");  // NSInteger return
    
    class_addMethod(viewClass,
                   sel_registerName("performDragOperation:"),
                   (IMP)performDragOperation_handler,
                   "c@:@");  // BOOL return
    
    objc_registerClassPair(viewClass);
    
    return viewClass;
}
```

**Why this works:**
- We create a custom NSView subclass at runtime
- `class_addIvar` stores our Window pointer in the view
- We implement the minimum required drag protocol methods
- Type encodings: `q` = NSInteger, `c` = BOOL, `@` = object
- Must register the class before using it

---

#### Step 3: Implement Drag Handlers

```cpp
// Helper to get Window from view
static Window* getWindowFromView(id view) {
    Window* window = nullptr;
    object_getInstanceVariable(view, "pgeWindow", (void**)&window);
    return window;
}

// Called when drag enters the view
static NSInteger draggingEntered_handler(id self, SEL _cmd, id sender) {
    // Return NSDragOperationCopy to show we accept the drag
    return 1;  // NSDragOperationCopy
}

// Called when user releases the drag
static BOOL performDragOperation_handler(id self, SEL _cmd, id sender) {
    Window* window = getWindowFromView(self);
    if (!window || !window->draggedFilesCallback) return NO;
    
    // Get the pasteboard
    id pasteboard = objc_msgSend(sender, sel_registerName("draggingPasteboard"));
    
    // Get file URLs
    Class NSURLClass = objc_getClass("NSURL");
    id fileURLs = objc_msgSend(pasteboard,
                               sel_registerName("readObjectsForClasses:options:"),
                               objc_msgSend((id)objc_getClass("NSArray"),
                                          sel_registerName("arrayWithObject:"),
                                          NSURLClass),
                               nil);
    
    if (!fileURLs) return NO;
    
    // Get count
    NSUInteger count = (NSUInteger)objc_msgSend(fileURLs, sel_registerName("count"));
    if (count == 0) return NO;
    
    // Convert to C strings
    const char** paths = new const char*[count];
    
    for (NSUInteger i = 0; i < count; i++) {
        id url = objc_msgSend(fileURLs, sel_registerName("objectAtIndex:"), i);
        id path = objc_msgSend(url, sel_registerName("path"));
        paths[i] = (const char*)objc_msgSend(path, sel_registerName("UTF8String"));
    }
    
    // Call callback
    window->draggedFilesCallback(paths, (int)count, window->draggedFilesUserData);
    
    // Cleanup
    delete[] paths;
    
    return YES;
}
```

**Why this works:**
- `draggingEntered` returning 1 (NSDragOperationCopy) shows we accept files
- `performDragOperation` is called when user drops files
- We get the pasteboard (clipboard-like object) from the drag sender
- `readObjectsForClasses` gets file URLs as an NSArray
- We convert NSURL objects to C strings
- Temporary array is cleaned up after callback

**Critical details:**
- The UTF8String pointers are only valid during the callback
- If you need to keep paths, copy them in the callback
- We use `new[]` and `delete[]` for the temporary array
- Returning YES tells macOS the drop was successful

---

#### Step 4: Modify Window Creation to Support Drag

Search for `window_create` and modify:

```cpp
void window_create(struct Window* self, unsigned long styleMask) {
    if (!self) return;
    
    AutoreleasePool pool;
    
    // ... existing window creation code ...
    
    // Create custom dragging view instead of regular content view
    Class viewClass = createDraggingDestinationViewClass();
    id contentView = objc_msgSend((id)viewClass, sel_registerName("alloc"));
    contentView = objc_msgSend(contentView, sel_registerName("init"));
    
    // Store Window pointer in view
    object_setInstanceVariable(contentView, "pgeWindow", self);
    
    // Register for file drag types
    Class NSArrayClass = objc_getClass("NSArray");
    Class NSStringClass = objc_getClass("NSString");
    
    // Create NSFilenamesPboardType string (deprecated but works)
    // Better: use UTType for modern macOS
    id fileType = objc_msgSend((id)NSStringClass,
                              sel_registerName("stringWithUTF8String:"),
                              "NSFilenamesPboardType");
    
    id typesArray = objc_msgSend((id)NSArrayClass,
                                sel_registerName("arrayWithObject:"),
                                fileType);
    
    // Register dragging types
    objc_msgSend(contentView,
                sel_registerName("registerForDraggedTypes:"),
                typesArray);
    
    // Set as window's content view
    objc_msgSend(self->nsWindow,
                sel_registerName("setContentView:"),
                contentView);
}
```

**Why this works:**
- We create an instance of our custom view class
- Store the Window pointer so handlers can access it
- Register for filename drag types
- Replace the window's content view with our custom view
- All existing functionality still works (view is still an NSView)

**Important note:**
- `NSFilenamesPboardType` is deprecated on modern macOS
- For macOS 11+, use `UTType` and uniform type identifiers
- This example uses the older API for simplicity

---

#### Step 5: Add C API Function

```cpp
typedef void (*DraggedFilesCallback)(const char** filePaths, 
                                     int count, 
                                     void* userData);

void window_setDraggedFilesCallback(struct Window* self,
                                     DraggedFilesCallback callback,
                                     void* userData) {
    if (!self) return;
    self->draggedFilesCallback = callback;
    self->draggedFilesUserData = userData;
}
```

---

#### Step 6: Add C++ Wrapper

Search for `Window` class in `namespace olc::apis::macos`:

```cpp
class Window {
public:
    // ... existing methods ...
    
    void setDraggedFilesHandler(std::function<void(const std::vector<std::string>&)> handler) {
        draggedFilesHandler_ = std::move(handler);
        
        window_setDraggedFilesCallback(
            window_,
            [](const char** paths, int count, void* userData) {
                auto* h = static_cast<std::function<void(const std::vector<std::string>&)>*>(userData);
                if (!h || !paths) return;
                
                // Convert to vector of strings
                std::vector<std::string> filePaths;
                filePaths.reserve(count);
                for (int i = 0; i < count; i++) {
                    if (paths[i]) {
                        filePaths.emplace_back(paths[i]);
                    }
                }
                
                // Call handler
                (*h)(filePaths);
            },
            &draggedFilesHandler_
        );
    }
    
private:
    std::function<void(const std::vector<std::string>&)> draggedFilesHandler_;
};
```

**Why this works:**
- Converts C string array to C++ vector for convenience
- `emplace_back` constructs strings in place (efficient)
- `reserve` pre-allocates space to avoid reallocations
- The lambda bridges C callback to C++ std::function
- Null checks prevent crashes on bad data

---

#### Step 7: Use in Host Layer

```cpp
void Host_Apple_MacOS::MacEventsHandler() {
    // ... existing handlers ...
    
    pMacOSWindow->setDraggedFilesHandler([this](const std::vector<std::string>& paths) {
        std::cout << "Files dropped: " << paths.size() << std::endl;
        
        for (const auto& path : paths) {
            std::cout << "  - " << path << std::endl;
            
            // Check file extension
            if (path.ends_with(".png") || path.ends_with(".jpg")) {
                // Load image
                LoadImageFromFile(path);
            }
        }
    });
}
```

**Why this works:**
- Lambda captures `this` to access host methods
- Range-based for loop is clean and efficient
- `ends_with` is C++20 (or use `path.substr()` for older C++)
- You can filter by extension or other criteria

---

### Complete Understanding

**Flow when files are dragged:**
1. User drags files over window
2. macOS calls `draggingEntered:` on our view
3. We return NSDragOperationCopy to accept
4. User releases drag
5. macOS calls `performDragOperation:` on our view
6. We extract file paths from pasteboard
7. Convert NSURL → NSString → const char*
8. Call our callback with the paths
9. Callback converts to std::vector<std::string>
10. Your game code receives the file paths

**Why this design is correct:**
- ✅ Follows macOS drag-and-drop protocol correctly
- ✅ Properly manages memory (temporary array)
- ✅ Type-safe conversion (C strings → std::string)
- ✅ Flexible (can handle any file type)
- ✅ Thread-safe (drag happens on main thread)
- ✅ No memory leaks (RAII for strings)

**Common use cases:**
- Load texture files dragged onto window
- Open level files
- Import custom assets
- Quick testing during development

---

<a name="challenge3"></a>
## Challenge 3: Custom Cursor Images

### The Challenge

**Requirements:**
- Load custom cursor from image file
- Set as window cursor
- Restore system cursor

### Complete Solution

#### Step 1: Add Cursor Management to Window Struct

```cpp
struct Window {
    // ... existing members ...
    
    id customCursor;      // NSCursor object
    BOOL usingCustomCursor;
};
```

**Why this works:**
- We store the cursor to keep it alive
- Boolean tracks whether custom cursor is active
- Will be initialized to nil/false automatically

---

#### Step 2: Create Custom Cursor from Image File

Add this C function to the bridge layer:

```cpp
BOOL window_setCustomCursorFromFile(struct Window* self, 
                                     const char* imagePath,
                                     double hotspotX,
                                     double hotspotY) {
    if (!self || !imagePath) return NO;
    
    AutoreleasePool pool;
    
    // Create NSString for path
    Class NSStringClass = objc_getClass("NSString");
    id pathString = objc_msgSend((id)NSStringClass,
                                sel_registerName("stringWithUTF8String:"),
                                imagePath);
    
    // Create NSImage from file
    Class NSImageClass = objc_getClass("NSImage");
    id image = objc_msgSend((id)NSImageClass, sel_registerName("alloc"));
    image = objc_msgSend(image,
                        sel_registerName("initWithContentsOfFile:"),
                        pathString);
    
    if (!image) {
        printf("ERROR: Failed to load cursor image: %s\n", imagePath);
        return NO;
    }
    
    // Create NSPoint for hotspot
    typedef struct { double x; double y; } NSPoint;
    NSPoint hotspot = {hotspotX, hotspotY};
    
    // Create NSCursor with image and hotspot
    Class NSCursorClass = objc_getClass("NSCursor");
    id cursor = objc_msgSend((id)NSCursorClass, sel_registerName("alloc"));
    cursor = objc_msgSend(cursor,
                         sel_registerName("initWithImage:hotSpot:"),
                         image,
                         hotspot);
    
    if (!cursor) {
        printf("ERROR: Failed to create cursor\n");
        objc_msgSend(image, sel_registerName("release"));
        return NO;
    }
    
    // Release old custom cursor if exists
    if (self->customCursor) {
        objc_msgSend(self->customCursor, sel_registerName("release"));
    }
    
    // Store new cursor (retain it)
    self->customCursor = objc_msgSend(cursor, sel_registerName("retain"));
    
    // Release local references
    objc_msgSend(cursor, sel_registerName("release"));
    objc_msgSend(image, sel_registerName("release"));
    
    return YES;
}
```

**Why this works:**
- `initWithContentsOfFile:` loads image from disk
- Hotspot determines the "click point" of the cursor
- We retain the cursor to keep it alive beyond this function
- Proper memory management: release old cursor, retain new one
- Image is released after cursor creation (cursor retains it internally)

**Important details:**
- Hotspot coordinates are relative to image
- (0,0) is top-left of image
- For a crosshair, hotspot might be center
- For an arrow, hotspot is the tip

---

#### Step 3: Set/Restore Cursor Functions

```cpp
void window_setCustomCursor(struct Window* self) {
    if (!self || !self->customCursor) return;
    
    // This must run on main thread!
    dispatch_async(dispatch_get_main_queue(), ^{
        // Make custom cursor active
        objc_msgSend(self->customCursor, sel_registerName("set"));
        self->usingCustomCursor = YES;
    });
}

void window_restoreSystemCursor(struct Window* self) {
    if (!self) return;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        // Restore arrow cursor
        Class NSCursorClass = objc_getClass("NSCursor");
        id arrowCursor = objc_msgSend((id)NSCursorClass,
                                     sel_registerName("arrowCursor"));
        objc_msgSend(arrowCursor, sel_registerName("set"));
        self->usingCustomCursor = NO;
    });
}
```

**Why this works:**
- Cursor changes must happen on main thread (UI operation)
- `set` method activates the cursor globally
- `arrowCursor` is a class method that returns the default arrow
- We track state so we know what's active

---

#### Step 4: Add Cleanup to Window Destroy

Search for `window_destroy` and modify:

```cpp
void window_destroy(struct Window* self) {
    if (!self) return;
    
    AutoreleasePool pool;
    
    // Restore system cursor if we set a custom one
    if (self->usingCustomCursor) {
        Class NSCursorClass = objc_getClass("NSCursor");
        id arrowCursor = objc_msgSend((id)NSCursorClass,
                                     sel_registerName("arrowCursor"));
        objc_msgSend(arrowCursor, sel_registerName("set"));
    }
    
    // Release custom cursor
    if (self->customCursor) {
        objc_msgSend(self->customCursor, sel_registerName("release"));
        self->customCursor = nil;
    }
    
    // ... existing cleanup code ...
    
    delete self;
}
```

**Why this is critical:**
- Must restore system cursor before destroying window
- Memory leak if we don't release custom cursor
- Leaving custom cursor active causes confusion after window closes

---

#### Step 5: Add C++ Wrapper Methods

```cpp
class Window {
public:
    // ... existing methods ...
    
    bool loadCustomCursor(const std::string& imagePath, 
                          double hotspotX = 0.0, 
                          double hotspotY = 0.0) {
        if (!window_) return false;
        return window_setCustomCursorFromFile(window_, 
                                              imagePath.c_str(),
                                              hotspotX, 
                                              hotspotY);
    }
    
    void useCustomCursor() {
        if (!window_) return;
        window_setCustomCursor(window_);
    }
    
    void useSystemCursor() {
        if (!window_) return;
        window_restoreSystemCursor(window_);
    }
};
```

**Why this works:**
- Clean C++ interface with std::string
- Default hotspot at (0,0) for convenience
- Returns bool so caller knows if load succeeded
- Short, memorable method names

---

#### Step 6: Use in Your Game

```cpp
class MyGame : public olc::PixelGameEngine {
public:
    bool OnUserCreate() override {
        // Access the host (would need to expose this properly)
        // For this example, assuming we can access the host
        
        // Load a custom cursor (e.g., crosshair)
        if (host->GetWindow()->loadCustomCursor("assets/crosshair.png", 16, 16)) {
            std::cout << "Custom cursor loaded" << std::endl;
            host->GetWindow()->useCustomCursor();
        } else {
            std::cout << "Failed to load cursor, using default" << std::endl;
        }
        
        return true;
    }
    
    bool OnUserUpdate(float fElapsedTime) override {
        // Toggle cursor with key press
        if (GetKey(olc::Key::C).bPressed) {
            static bool useCustom = true;
            if (useCustom) {
                host->GetWindow()->useSystemCursor();
            } else {
                host->GetWindow()->useCustomCursor();
            }
            useCustom = !useCustom;
        }
        
        return true;
    }
};
```

**Note:** You'd need to expose window access properly through the host interface.

---

### Alternative: Programmatically Created Cursor

Instead of loading from file, you can create a cursor from pixel data:

```cpp
BOOL window_setCustomCursorFromPixels(struct Window* self,
                                       const unsigned char* pixels,
                                       int width,
                                       int height,
                                       double hotspotX,
                                       double hotspotY) {
    if (!self || !pixels || width <= 0 || height <= 0) return NO;
    
    AutoreleasePool pool;
    
    // Create NSBitmapImageRep from raw pixels
    Class NSBitmapImageRepClass = objc_getClass("NSBitmapImageRep");
    
    // Allocate
    id imageRep = objc_msgSend((id)NSBitmapImageRepClass,
                              sel_registerName("alloc"));
    
    // Initialize with pixel data (RGBA format)
    imageRep = objc_msgSend(imageRep,
                           sel_registerName("initWithBitmapDataPlanes:pixelsWide:pixelsHigh:bitsPerSample:samplesPerPixel:hasAlpha:isPlanar:colorSpaceName:bytesPerRow:bitsPerPixel:"),
                           &pixels,           // planes
                           width,             // pixelsWide
                           height,            // pixelsHigh
                           8,                 // bitsPerSample
                           4,                 // samplesPerPixel (RGBA)
                           YES,               // hasAlpha
                           NO,                // isPlanar
                           objc_msgSend((id)objc_getClass("NSString"),
                                       sel_registerName("stringWithUTF8String:"),
                                       "NSDeviceRGBColorSpace"),
                           width * 4,         // bytesPerRow
                           32);               // bitsPerPixel
    
    if (!imageRep) return NO;
    
    // Create NSImage and add representation
    Class NSImageClass = objc_getClass("NSImage");
    id image = objc_msgSend((id)NSImageClass, sel_registerName("alloc"));
    
    typedef struct { double width; double height; } NSSize;
    NSSize size = {(double)width, (double)height};
    
    image = objc_msgSend(image,
                        sel_registerName("initWithSize:"),
                        size);
    
    objc_msgSend(image,
                sel_registerName("addRepresentation:"),
                imageRep);
    
    // Create cursor
    typedef struct { double x; double y; } NSPoint;
    NSPoint hotspot = {hotspotX, hotspotY};
    
    Class NSCursorClass = objc_getClass("NSCursor");
    id cursor = objc_msgSend((id)NSCursorClass, sel_registerName("alloc"));
    cursor = objc_msgSend(cursor,
                         sel_registerName("initWithImage:hotSpot:"),
                         image,
                         hotspot);
    
    if (!cursor) {
        objc_msgSend(image, sel_registerName("release"));
        objc_msgSend(imageRep, sel_registerName("release"));
        return NO;
    }
    
    // Release old cursor
    if (self->customCursor) {
        objc_msgSend(self->customCursor, sel_registerName("release"));
    }
    
    // Store new cursor
    self->customCursor = objc_msgSend(cursor, sel_registerName("retain"));
    
    // Cleanup
    objc_msgSend(cursor, sel_registerName("release"));
    objc_msgSend(image, sel_registerName("release"));
    objc_msgSend(imageRep, sel_registerName("release"));
    
    return YES;
}
```

**Why this is useful:**
- Can generate cursors dynamically
- Useful for UI frameworks
- Can create animated cursors by updating periodically
- Pixel data is RGBA format (4 bytes per pixel)

---

### Complete Understanding

**Flow for custom cursor:**
1. Load image file into NSImage
2. Create NSCursor with image and hotspot
3. Store cursor (retain it)
4. Call `set` method to activate cursor
5. macOS changes cursor appearance
6. On cleanup, restore system cursor and release

**Why this design is correct:**
- ✅ Proper memory management (retain/release)
- ✅ Cleanup on window destruction
- ✅ Main thread operations via GCD
- ✅ Error handling (returns BOOL)
- ✅ Can switch cursors at runtime
- ✅ Supports both file-based and programmatic creation

**Tips for cursor images:**
- PNG format with transparency works best
- Typical size: 32×32 or 16×16 pixels
- Hotspot should match visual "point"
- Test with different backgrounds
- Can create multiple cursors for different modes

---

<a name="challenge4"></a>
## Challenge 4: Fullscreen Transitions

### The Challenge

**Requirements:**
- Smooth animation to fullscreen
- Handle resolution changes
- Restore window position after fullscreen

### Complete Solution

This challenge requires managing window state through fullscreen transitions.

#### Step 1: Add State Tracking to Window Struct

```cpp
struct Window {
    // ... existing members ...
    
    // Fullscreen state
    BOOL isFullscreen;
    NSRect savedWindowFrame;  // Save position/size before fullscreen
    
    // Fullscreen callbacks
    void (*willEnterFullscreenCallback)(void* userData);
    void* willEnterFullscreenUserData;
    
    void (*didEnterFullscreenCallback)(void* userData);
    void* didEnterFullscreenUserData;
    
    void (*willExitFullscreenCallback)(void* userData);
    void* willExitFullscreenUserData;
    
    void (*didExitFullscreenCallback)(void* userData);
    void* didExitFullscreenUserData;
};
```

**Why this works:**
- `isFullscreen` tracks current state
- `savedWindowFrame` preserves window position/size
- Four callbacks cover the complete transition lifecycle
- "will" callbacks fire before animation
- "did" callbacks fire after animation completes

---

#### Step 2: Create Fullscreen Delegate Handlers

```cpp
static void windowWillEnterFullScreen_handler(id self, SEL _cmd, id notification) {
    Window* window = getWindowFromSelf(self);
    if (!window) return;
    
    // Save current window frame
    NSRect frame = {0};
    id frameValue = objc_msgSend(window->nsWindow, sel_registerName("frame"));
    memcpy(&frame, &frameValue, sizeof(NSRect));
    window->savedWindowFrame = frame;
    
    printf("Entering fullscreen - saved frame: (%.0f, %.0f, %.0f×%.0f)\n",
           frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
    
    if (window->willEnterFullscreenCallback) {
        window->willEnterFullscreenCallback(window->willEnterFullscreenUserData);
    }
}

static void windowDidEnterFullScreen_handler(id self, SEL _cmd, id notification) {
    Window* window = getWindowFromSelf(self);
    if (!window) return;
    
    window->isFullscreen = YES;
    printf("Entered fullscreen\n");
    
    if (window->didEnterFullscreenCallback) {
        window->didEnterFullscreenCallback(window->didEnterFullscreenUserData);
    }
}

static void windowWillExitFullScreen_handler(id self, SEL _cmd, id notification) {
    Window* window = getWindowFromSelf(self);
    if (!window) return;
    
    printf("Exiting fullscreen\n");
    
    if (window->willExitFullscreenCallback) {
        window->willExitFullscreenCallback(window->willExitFullscreenUserData);
    }
}

static void windowDidExitFullScreen_handler(id self, SEL _cmd, id notification) {
    Window* window = getWindowFromSelf(self);
    if (!window) return;
    
    window->isFullscreen = NO;
    
    // Restore saved frame
    dispatch_async(dispatch_get_main_queue(), ^{
        objc_msgSend(window->nsWindow,
                    sel_registerName("setFrame:display:"),
                    window->savedWindowFrame,
                    YES);
        
        printf("Exited fullscreen - restored frame: (%.0f, %.0f, %.0f×%.0f)\n",
               window->savedWindowFrame.origin.x,
               window->savedWindowFrame.origin.y,
               window->savedWindowFrame.size.width,
               window->savedWindowFrame.size.height);
    });
    
    if (window->didExitFullscreenCallback) {
        window->didExitFullscreenCallback(window->didExitFullscreenUserData);
    }
}
```

**Why this works:**
- `windowWillEnterFullScreen:` saves the current frame before animation
- `windowDidEnterFullScreen:` marks us as fullscreen after animation
- `windowWillExitFullScreen:` called before restore animation
- `windowDidExitFullScreen:` restores saved frame after animation
- `setFrame:display:` restores exact position and size
- `YES` parameter makes the window update immediately

**Important timing:**
- "Will" handlers: Setup/preparation phase
- Animation happens (managed by macOS)
- "Did" handlers: Finalization phase
- Restore must happen in "did exit", not "will exit"

---

#### Step 3: Register Handlers

Search for `createWindowDelegateClass` and add:

```cpp
class_addMethod(
    delegateClass,
    sel_registerName("windowWillEnterFullScreen:"),
    (IMP)windowWillEnterFullScreen_handler,
    "v@:@"
);

class_addMethod(
    delegateClass,
    sel_registerName("windowDidEnterFullScreen:"),
    (IMP)windowDidEnterFullScreen_handler,
    "v@:@"
);

class_addMethod(
    delegateClass,
    sel_registerName("windowWillExitFullScreen:"),
    (IMP)windowWillExitFullScreen_handler,
    "v@:@"
);

class_addMethod(
    delegateClass,
    sel_registerName("windowDidExitFullScreen:"),
    (IMP)windowDidExitFullScreen_handler,
    "v@:@"
);
```

---

#### Step 4: Add Toggle Fullscreen Function

```cpp
void window_toggleFullscreen(struct Window* self) {
    if (!self || !self->nsWindow) return;
    
    // This MUST run on main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        // Call toggleFullScreen: method
        objc_msgSend(self->nsWindow,
                    sel_registerName("toggleFullScreen:"),
                    nil);
    });
}

BOOL window_isFullscreen(struct Window* self) {
    if (!self) return NO;
    return self->isFullscreen;
}
```

**Why this works:**
- `toggleFullScreen:` is a standard NSWindow method
- It handles the animation automatically
- Passing `nil` as the sender is fine
- macOS calls our delegate methods during the transition
- Must be on main thread (window operation)

---

#### Step 5: Add Callback Setters

```cpp
void window_setWillEnterFullscreenCallback(struct Window* self,
                                            void (*callback)(void*),
                                            void* userData) {
    if (!self) return;
    self->willEnterFullscreenCallback = callback;
    self->willEnterFullscreenUserData = userData;
}

void window_setDidEnterFullscreenCallback(struct Window* self,
                                           void (*callback)(void*),
                                           void* userData) {
    if (!self) return;
    self->didEnterFullscreenCallback = callback;
    self->didEnterFullscreenUserData = userData;
}

void window_setWillExitFullscreenCallback(struct Window* self,
                                           void (*callback)(void*),
                                           void* userData) {
    if (!self) return;
    self->willExitFullscreenCallback = callback;
    self->willExitFullscreenUserData = userData;
}

void window_setDidExitFullscreenCallback(struct Window* self,
                                          void (*callback)(void*),
                                          void* userData) {
    if (!self) return;
    self->didExitFullscreenCallback = callback;
    self->didExitFullscreenUserData = userData;
}
```

---

#### Step 6: Add C++ Wrapper Methods

```cpp
class Window {
public:
    // ... existing methods ...
    
    void toggleFullscreen() {
        if (!window_) return;
        window_toggleFullscreen(window_);
    }
    
    bool isFullscreen() const {
        if (!window_) return false;
        return window_isFullscreen(window_);
    }
    
    void setWillEnterFullscreenHandler(std::function<void()> handler) {
        willEnterFullscreenHandler_ = std::move(handler);
        window_setWillEnterFullscreenCallback(
            window_,
            [](void* userData) {
                auto* h = static_cast<std::function<void()>*>(userData);
                if (h) (*h)();
            },
            &willEnterFullscreenHandler_
        );
    }
    
    void setDidEnterFullscreenHandler(std::function<void()> handler) {
        didEnterFullscreenHandler_ = std::move(handler);
        window_setDidEnterFullscreenCallback(
            window_,
            [](void* userData) {
                auto* h = static_cast<std::function<void()>*>(userData);
                if (h) (*h)();
            },
            &didEnterFullscreenHandler_
        );
    }
    
    void setWillExitFullscreenHandler(std::function<void()> handler) {
        willExitFullscreenHandler_ = std::move(handler);
        window_setWillExitFullscreenCallback(
            window_,
            [](void* userData) {
                auto* h = static_cast<std::function<void()>*>(userData);
                if (h) (*h)();
            },
            &willExitFullscreenHandler_
        );
    }
    
    void setDidExitFullscreenHandler(std::function<void()> handler) {
        didExitFullscreenHandler_ = std::move(handler);
        window_setDidExitFullscreenCallback(
            window_,
            [](void* userData) {
                auto* h = static_cast<std::function<void()>*>(userData);
                if (h) (*h)();
            },
            &didExitFullscreenHandler_
        );
    }
    
private:
    std::function<void()> willEnterFullscreenHandler_;
    std::function<void()> didEnterFullscreenHandler_;
    std::function<void()> willExitFullscreenHandler_;
    std::function<void()> didExitFullscreenHandler_;
};
```

---

#### Step 7: Use in Host Layer

```cpp
void Host_Apple_MacOS::MacEventsHandler() {
    // ... existing handlers ...
    
    // Fullscreen enter
    pMacOSWindow->setWillEnterFullscreenHandler([this]() {
        std::cout << "About to enter fullscreen..." << std::endl;
        // Could pause rendering momentarily
    });
    
    pMacOSWindow->setDidEnterFullscreenHandler([this]() {
        std::cout << "Entered fullscreen!" << std::endl;
        // Could adjust rendering resolution
        // Could hide UI elements
    });
    
    // Fullscreen exit
    pMacOSWindow->setWillExitFullscreenHandler([this]() {
        std::cout << "About to exit fullscreen..." << std::endl;
    });
    
    pMacOSWindow->setDidExitFullscreenHandler([this]() {
        std::cout << "Exited fullscreen!" << std::endl;
        // Restore window-based rendering
        // Show UI elements again
    });
}

// Add public method to toggle
bool Host_Apple_MacOS::ToggleFullscreen() {
    if (!pMacOSWindow) return false;
    pMacOSWindow->toggleFullscreen();
    return true;
}
```

---

#### Step 8: Use in Game

```cpp
bool OnUserUpdate(float fElapsedTime) override {
    // Press F11 to toggle fullscreen
    if (GetKey(olc::Key::F11).bPressed) {
        // Access host (would need proper interface)
        host->ToggleFullscreen();
    }
    
    // Check if fullscreen
    if (host->IsFullscreen()) {
        // Maybe render differently in fullscreen
        // Hide mouse cursor, etc.
    }
    
    return true;
}
```

---

### Advanced: Handle Resolution Changes

For games that want to change rendering resolution in fullscreen:

```cpp
class Host_Apple_MacOS : public olc::host::Host {
private:
    olc::vi2d windowedSize;
    olc::vi2d fullscreenSize;
    
public:
    void MacEventsHandler() {
        pMacOSWindow->setWillEnterFullscreenHandler([this]() {
            // Save windowed size
            double w, h;
            pMacOSWindow->getSize(&w, &h);
            windowedSize = {(int)w, (int)h};
            
            // Get screen size
            // (Would need to implement window_getScreenSize)
            fullscreenSize = GetScreenSize();
            
            std::cout << "Switching from " << windowedSize.x << "×" << windowedSize.y
                      << " to " << fullscreenSize.x << "×" << fullscreenSize.y << std::endl;
        });
        
        pMacOSWindow->setDidEnterFullscreenHandler([this]() {
            // Resize rendering to screen size
            pPGEwindow->olc_UpdateWindowSize(fullscreenSize.x, fullscreenSize.y);
        });
        
        pMacOSWindow->setDidExitFullscreenHandler([this]() {
            // Restore windowed size
            pPGEwindow->olc_UpdateWindowSize(windowedSize.x, windowedSize.y);
        });
    }
};
```

**Why this works:**
- Saves windowed size before transition
- Gets screen size for fullscreen
- Updates PGE's internal size tracking
- Restores windowed size on exit
- Keeps aspect ratio or stretches as needed

---

### Complete Understanding

**Flow for fullscreen transition:**
1. User presses F11 (or clicks fullscreen button)
2. `toggleFullscreen()` called
3. macOS calls `windowWillEnterFullScreen:` on delegate
4. We save current window frame
5. macOS animates window to fullscreen (smooth zoom)
6. macOS calls `windowDidEnterFullScreen:` on delegate
7. We mark `isFullscreen = true`
8. Game is now fullscreen

**Flow for exiting fullscreen:**
1. User presses F11 again (or Escape)
2. `toggleFullscreen()` called
3. macOS calls `windowWillExitFullScreen:` on delegate
4. macOS animates window back to windowed mode
5. macOS calls `windowDidExitFullScreen:` on delegate
6. We restore saved frame
7. Window is back to original position and size

**Why this design is correct:**
- ✅ Smooth animation (handled by macOS)
- ✅ Preserves window position exactly
- ✅ Callbacks allow game to react
- ✅ Can adjust resolution if needed
- ✅ Thread-safe (main thread operations)
- ✅ No flicker or jarring transitions
- ✅ Works with multiple displays

**Common issues and solutions:**
- **Black screen in fullscreen**: Check OpenGL context is current
- **Wrong resolution**: Need to update framebuffer size
- **Window doesn't restore**: Saved frame is wrong - check when it's saved
- **Animation stutters**: Don't do heavy work in "will" handlers
- **Can't exit fullscreen**: Check F11 key is not blocked

---

<a name="challenge5"></a>
## Challenge 5: Multi-Window Support

### The Challenge

**Requirements:**
- Create multiple game windows
- Each with own OpenGL context
- Separate input handling

### Complete Solution

This is the most complex challenge as it requires significant Host layer changes.

**Important Note:** This solution shows the pattern but would require extensive refactoring of the current Host implementation. Consider this a learning exercise rather than a drop-in solution.

#### Step 1: Refactor Host to Support Multiple Windows

```cpp
class Host_Apple_MacOS : public olc::host::Host {
private:
    // Instead of single window, use a vector
    struct WindowContext {
        std::unique_ptr<olc::apis::macos::Window> window;
        std::unique_ptr<olc::apis::macos::EventHandler> eventHandler;
        std::shared_ptr<olc::apis::macos::OpenGLRenderer> renderer;
        olc::Window* pPGEWindow;  // Associated PGE window
        int windowID;
    };
    
    std::vector<WindowContext> windows_;
    int nextWindowID_ = 0;
    
    // Still need shared application
    std::unique_ptr<olc::apis::macos::Application> pMacApplication;
    
public:
    int CreateGameWindow(const olc::vi2d& size, const std::string& title) {
        WindowContext ctx;
        ctx.windowID = nextWindowID_++;
        
        // Create window
        ctx.window = std::make_unique<olc::apis::macos::Window>(
            size.x, size.y, title.c_str()
        );
        
        // Create OpenGL renderer
        ctx.renderer = std::make_shared<olc::apis::macos::OpenGLRenderer>();
        ctx.renderer->initialize(*ctx.window);
        
        // Create event handler
        ctx.eventHandler = std::make_unique<olc::apis::macos::EventHandler>(
            ctx.window->getCHandle()
        );
        
        // Setup events for this window
        SetupWindowEvents(ctx);
        
        // Show window
        ctx.window->show();
        
        windows_.push_back(std::move(ctx));
        
        return ctx.windowID;
    }
    
    void SetupWindowEvents(WindowContext& ctx) {
        // Each window gets its own event handlers
        ctx.eventHandler->setKeyDownHandler(
            [this, windowID = ctx.windowID](uint16_t key, const char* chars, uint32_t flags) {
                // Find the window
                auto it = std::find_if(windows_.begin(), windows_.end(),
                                      [windowID](const WindowContext& w) {
                                          return w.windowID == windowID;
                                      });
                
                if (it != windows_.end() && it->pPGEWindow) {
                    // Update key state for this window's PGE instance
                    if (mapKeys.count(key)) {
                        it->pPGEWindow->olc_UpdateKeyState(mapKeys[key], true);
                    }
                }
            }
        );
        
        // Similar for other events (mouse, resize, etc.)
        // ... (implementation similar to above)
    }
};
```

**Why this works:**
- `std::vector` holds multiple window contexts
- Each window has its own renderer and event handler
- Window ID lets us identify which window events belong to
- Lambda captures window ID to route events correctly

---

#### Step 2: OpenGL Context Switching

The tricky part: each window needs its own OpenGL context, and you must switch before rendering.

```cpp
class Host_Apple_MacOS : public olc::host::Host {
public:
    void MakeWindowCurrent(int windowID) {
        auto it = std::find_if(windows_.begin(), windows_.end(),
                              [windowID](const WindowContext& w) {
                                  return w.windowID == windowID;
                              });
        
        if (it != windows_.end() && it->renderer) {
            // Switch to this window's OpenGL context
            it->renderer->makeCurrentContext();
        }
    }
    
    void RenderWindow(int windowID) {
        auto it = std::find_if(windows_.begin(), windows_.end(),
                              [windowID](const WindowContext& w) {
                                  return w.windowID == windowID;
                              });
        
        if (it != windows_.end()) {
            // Make this window's context current
            it->renderer->makeCurrentContext();
            
            // Render this window's PGE instance
            if (it->pPGEWindow) {
                // PGE render calls go here
                // Would need to call appropriate PGE render methods
            }
            
            // Swap buffers
            it->renderer->swapBuffers();
        }
    }
    
    void RenderAllWindows() {
        for (auto& ctx : windows_) {
            RenderWindow(ctx.windowID);
        }
    }
};
```

**Why this is critical:**
- OpenGL is stateful - current context matters
- Each window has its own framebuffers, textures
- Must call `makeCurrentContext()` before drawing
- Mixing contexts causes crashes or corruption

---

#### Step 3: Per-Window Game Instances

For truly independent windows, each needs its own game instance:

```cpp
class MultiWindowGame {
public:
    struct GameWindow {
        std::unique_ptr<olc::PixelGameEngine> game;
        int hostWindowID;
    };
    
    std::vector<GameWindow> gameWindows_;
    Host_Apple_MacOS* host_;
    
    void CreateNewGameWindow(const std::string& title) {
        GameWindow gw;
        
        // Create host window
        gw.hostWindowID = host_->CreateGameWindow({800, 600}, title);
        
        // Create game instance
        gw.game = std::make_unique<MyGame>();
        
        // Connect to host window
        // (This would require exposing more of the internal API)
        
        gameWindows_.push_back(std::move(gw));
    }
    
    void UpdateAll(float dt) {
        for (auto& gw : gameWindows_) {
            // Make this window current
            host_->MakeWindowCurrent(gw.hostWindowID);
            
            // Update game
            gw.game->OnUserUpdate(dt);
        }
    }
    
    void RenderAll() {
        for (auto& gw : gameWindows_) {
            // Make this window current
            host_->MakeWindowCurrent(gw.hostWindowID);
            
            // Render game
            gw.game->OnUserDraw();
            
            // Swap buffers
            host_->RenderWindow(gw.hostWindowID);
        }
    }
};
```

**Why this works:**
- Each window has completely independent game state
- No shared state means no conflicts
- Can run different games in different windows
- Context switching ensures correct rendering

---

#### Step 4: Shared Resources (Advanced)

For efficiency, you might want to share resources between windows:

```cpp
class SharedResourceManager {
private:
    // Textures keyed by name
    std::map<std::string, GLuint> sharedTextures_;
    
    // Mutex for thread safety
    std::mutex resourceMutex_;
    
public:
    GLuint GetOrLoadTexture(const std::string& path) {
        std::lock_guard<std::mutex> lock(resourceMutex_);
        
        auto it = sharedTextures_.find(path);
        if (it != sharedTextures_.end()) {
            return it->second;  // Already loaded
        }
        
        // Load texture
        GLuint textureID = LoadTextureFromFile(path);
        sharedTextures_[path] = textureID;
        
        return textureID;
    }
    
    void Cleanup() {
        std::lock_guard<std::mutex> lock(resourceMutex_);
        
        for (auto& pair : sharedTextures_) {
            glDeleteTextures(1, &pair.second);
        }
        sharedTextures_.clear();
    }
};
```

**Why this is useful:**
- Avoids loading same texture multiple times
- Saves memory
- Mutex ensures thread safety
- Cleanup is centralized

**Caution:**
- OpenGL textures are context-specific
- Sharing requires contexts to be in the same share group
- Would need to create contexts with sharing enabled

---

### Complete Understanding

**Why multi-window is complex:**
1. **OpenGL Context Management**
   - Each window needs its own context
   - Must switch before rendering
   - Shared resources need special handling

2. **Event Routing**
   - Must know which window got the event
   - Each window needs independent input state
   - Keyboard focus management

3. **Resource Management**
   - Memory grows linearly with window count
   - Need to clean up properly
   - Shared resources save memory but add complexity

4. **Threading**
   - Main thread still handles all UI
   - Could have separate render threads per window
   - Synchronization becomes critical

**Complete flow for multi-window:**
1. Create host
2. Create first window with game
3. Create second window with different (or same) game
4. Main loop:
   - Process events for all windows
   - Update all games
   - For each window:
     - Make context current
     - Render game
     - Swap buffers

**Why this design is correct:**
- ✅ Each window is fully independent
- ✅ Proper OpenGL context switching
- ✅ No state leakage between windows
- ✅ Can close windows independently
- ✅ Scales to N windows
- ✅ Optional resource sharing for efficiency

**Limitations:**
- High memory use (N contexts, N framebuffers)
- Complex state management
- Requires careful OpenGL usage
- Main thread can become bottleneck

**Real-world use cases:**
- Level editor (game view + tool windows)
- Debug views (multiple perspectives)
- Split-screen preparation
- Multi-monitor spanning
- Testing different rendering modes

---

## Summary

All five challenges demonstrate key concepts:

**Challenge 1 (Minimize):**
- ✅ Delegate methods
- ✅ Event callbacks
- ✅ Thread-safe state (atomic)
- ✅ Game loop control

**Challenge 2 (Drag-and-Drop):**
- ✅ Protocol implementation
- ✅ Custom view classes
- ✅ Pasteboard handling
- ✅ File path extraction

**Challenge 3 (Custom Cursors):**
- ✅ NSImage/NSCursor creation
- ✅ Memory management (retain/release)
- ✅ Resource cleanup
- ✅ State restoration

**Challenge 4 (Fullscreen):**
- ✅ Animation handling
- ✅ State preservation
- ✅ Resolution management
- ✅ Transition callbacks

**Challenge 5 (Multi-Window):**
- ✅ Resource management
- ✅ OpenGL context switching
- ✅ Event routing
- ✅ Architecture refactoring

**General Patterns Used Throughout:**
- Layer separation (Bridge → Wrapper → Host)
- RAII for memory safety
- Callbacks for event handling
- Main thread for UI operations
- Atomic types for thread safety
- Error checking and validation

**Next Steps:**
1. Try implementing these solutions
2. Test edge cases (rapid toggling, etc.)
3. Add error handling
4. Optimize performance
5. Extend with your own features!

---

*Remember: These solutions prioritize clarity over maximum efficiency. In production code, you might optimize further, but understanding the pattern is most important!*

---

**Happy Coding!** 🚀

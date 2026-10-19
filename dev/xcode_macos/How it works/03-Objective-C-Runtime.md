# Understanding the Objective-C Runtime

**A Beginner's Guide to How C++ Talks to macOS**

> This guide explains how the "magic" of `objc_msgSend` works and why we need it to talk to macOS from C++.

---

## Table of Contents

1. [The Problem We're Solving](#the-problem)
2. [What is the Objective-C Runtime?](#what-is-it)
3. [Key Concepts](#key-concepts)
4. [Understanding objc_msgSend](#objc-msgsend)
5. [Common Patterns in Our Code](#common-patterns)
6. [Step-by-Step Examples](#examples)
7. [Memory Management](#memory-management)
8. [Troubleshooting](#troubleshooting)

---

<a name="the-problem"></a>
## The Problem We're Solving

### Two Different Worlds

**Objective-C (macOS):**
```objective-c
// Creating a window in Objective-C
NSWindow* window = [[NSWindow alloc] init];
[window setTitle:@"My Window"];
[window makeKeyAndOrderFront:nil];
```

**C++ (Our Game):**
```cpp
// We want to write code like this
Window window(800, 600, "My Window");
window.show();
```

**The Challenge:**
- C++ doesn't understand Objective-C syntax
- We can't directly call `[window setTitle:]` from C++
- macOS APIs are **only** available in Objective-C

**The Solution:**
Use the **Objective-C Runtime** - a C library that lets us call Objective-C methods from C/C++!

---

<a name="what-is-it"></a>
## What is the Objective-C Runtime?

The Objective-C Runtime is a **C library** included with macOS that provides functions for:

1. **Getting Classes:** Finding Objective-C classes by name
2. **Creating Objects:** Allocating and initializing instances
3. **Calling Methods:** Sending messages to objects
4. **Inspecting Types:** Querying object information

### Key Header Files

```cpp
#include <objc/objc.h>           // Basic types (id, SEL)
#include <objc/runtime.h>        // Runtime functions
#include <objc/message.h>        // objc_msgSend
#include <objc/NSObjCRuntime.h>  // Foundation types
```

### Why This Works

```
┌─────────────────────────────────────────┐
│           Your C++ Code                 │
│   window_setTitle(window, "Title");     │
└──────────────┬──────────────────────────┘
               │
               ↓
┌─────────────────────────────────────────┐
│      Objective-C Runtime (C API)        │
│   objc_msgSend(window, "setTitle:", s); │
└──────────────┬──────────────────────────┘
               │
               ↓
┌─────────────────────────────────────────┐
│        macOS System (Objective-C)       │
│       [window setTitle:@"Title"];       │
└─────────────────────────────────────────┘
```

---

<a name="key-concepts"></a>
## Key Concepts

### 1. id - The Universal Object Type

`id` is like a `void*` for Objective-C objects - it can point to any object:

```cpp
id object;  // Can be any Objective-C object
id window;  // Points to NSWindow
id string;  // Points to NSString
id view;    // Points to NSView
```

**Important:**
- `id` is an opaque pointer
- You can't directly access members
- Must use runtime functions to interact with it

---

### 2. Class - Represents Objective-C Classes

```cpp
// Get a class by name
Class NSWindowClass = objc_getClass("NSWindow");
Class NSStringClass = objc_getClass("NSString");

// Check if an object is of a certain class
if (object_isKindOfClass(myObject, NSWindowClass)) {
    // myObject is a window
}
```

---

### 3. SEL - Method Selectors

A **selector** is the name of a method:

```cpp
// Get selector for a method
SEL allocSelector = sel_registerName("alloc");
SEL initSelector = sel_registerName("init");
SEL setTitleSelector = sel_registerName("setTitle:");
```

**Naming Convention:**
- No parameters: `"methodName"`
- With parameters: `"methodName:"` or `"method:withParam:"`
- Colons indicate parameters!

**Examples:**
```objective-c
// Objective-C          →  Selector
[obj init]              →  "init"
[obj setTitle:title]    →  "setTitle:"
[obj setValue:v forKey:k] → "setValue:forKey:"
```

---

### 4. objc_msgSend - The Core Function

`objc_msgSend` is how you call methods on objects:

```cpp
// Basic pattern
objc_msgSend(object, selector);
objc_msgSend(object, selector, argument);
objc_msgSend(object, selector, arg1, arg2);
```

**Think of it as:**
```
objc_msgSend(receiver, method, ...arguments)
```

---

<a name="objc-msgsend"></a>
## Understanding objc_msgSend

### The Function Signature

```cpp
id objc_msgSend(id self, SEL op, ...);
```

- `id self` - The object receiving the message
- `SEL op` - The selector (method name)
- `...` - Variable arguments for the method

### How It Works Internally

When you call:
```cpp
objc_msgSend(window, sel_registerName("setTitle:"), title);
```

The runtime:
1. Looks up the method in the object's class
2. Finds the implementation (function pointer)
3. Calls that function with the arguments
4. Returns the result

### Return Types

`objc_msgSend` returns `id`, but you can cast to the actual type:

```cpp
// Returns an object
id result = objc_msgSend(object, selector);

// Returns a number
int number = (int)objc_msgSend(object, selector);

// Returns BOOL
BOOL flag = (BOOL)objc_msgSend(object, selector);

// Returns a struct (needs special handling)
NSRect frame = {0};
id frameObj = objc_msgSend(object, sel_registerName("frame"));
memcpy(&frame, &frameObj, sizeof(NSRect));
```

### Argument Types

```cpp
// No arguments
objc_msgSend(object, sel_registerName("init"));

// Object argument
objc_msgSend(window, sel_registerName("setTitle:"), nsString);

// Number argument
objc_msgSend(window, sel_registerName("setLevel:"), 5);

// BOOL argument
objc_msgSend(window, sel_registerName("setVisible:"), YES);

// Struct argument (pass by value)
NSRect frame = {0, 0, 800, 600};
objc_msgSend(window, sel_registerName("setFrame:"), frame);

// Multiple arguments
objc_msgSend(window, 
             sel_registerName("initWithContentRect:styleMask:backing:defer:"),
             frame, styleMask, 2, NO);
```

---

<a name="common-patterns"></a>
## Common Patterns in Our Code

### Pattern 1: Getting a Class

```cpp
// Get NSWindow class
Class NSWindowClass = objc_getClass("NSWindow");

// Check if class exists
if (NSWindowClass == nil) {
    // Class not found - error!
}
```

**Real Example from our code:**
```cpp
// From api_macos.cpp
Class NSWindowClass = objc_getClass(kNSWindowClass);  // kNSWindowClass = "NSWindow"
```

---

### Pattern 2: Creating an Object

**Two-Step Process:**
1. Allocate memory: `alloc`
2. Initialize object: `init`

```cpp
// Step 1: Allocate
id window = objc_msgSend(
    (id)NSWindowClass,           // Send to class, not instance!
    sel_registerName("alloc")
);

// Step 2: Initialize
window = objc_msgSend(
    window,                       // Send to instance
    sel_registerName("init")
);
```

**Why two steps?**
- `alloc` reserves memory
- `init` sets up the object's initial state
- Some classes have custom `init` methods with parameters

**Real Example:**
```cpp
// From api_macos.cpp - Creating NSWindow
id window = objc_msgSend((id)NSWindowClass, sel_registerName(kAllocSel));
window = objc_msgSend(
    window,
    sel_registerName(kInitWithContentRectSel),  // Custom initializer
    frame, styleMask, backing, defer
);
```

---

### Pattern 3: Calling Methods

```cpp
// Method with no arguments
objc_msgSend(window, sel_registerName("makeKeyAndOrderFront:"), nil);

// Method with arguments
objc_msgSend(window, sel_registerName("setTitle:"), nsString);

// Method that returns a value
id contentView = objc_msgSend(window, sel_registerName("contentView"));
```

---

### Pattern 4: Creating NSString from C String

```cpp
// Convert C string to NSString
const char* cString = "My Window";

Class NSStringClass = objc_getClass("NSString");
id nsString = objc_msgSend(
    (id)NSStringClass,
    sel_registerName("stringWithUTF8String:"),
    cString
);

// Now use it
objc_msgSend(window, sel_registerName("setTitle:"), nsString);
```

**Real Example:**
```cpp
// From api_macos.cpp
void window_setTitle(struct Window* self, const char* title) {
    Class NSStringClass = objc_getClass(kNSStringClass);
    id nsTitle = objc_msgSend(
        (id)NSStringClass,
        sel_registerName(kStringWithUTF8StringSel),
        title
    );
    
    objc_msgSend(
        self->nsWindow,
        sel_registerName(kSetTitleSel),
        nsTitle
    );
}
```

---

### Pattern 5: Getting String from NSString

```cpp
// NSString → C string
id nsString = /* ... */;

const char* cString = (const char*)objc_msgSend(
    nsString,
    sel_registerName("UTF8String")
);

// Now you can use it in C/C++
printf("String: %s\n", cString);
```

---

### Pattern 6: Extracting Struct Values

Structs can't be returned directly from `objc_msgSend`, so we use `memcpy`:

```cpp
// Get frame (NSRect)
id frameObj = objc_msgSend(
    window,
    sel_registerName("frame")
);

NSRect frame = {0};
memcpy(&frame, &frameObj, sizeof(NSRect));

// Now use the struct
double width = frame.width;
double height = frame.height;
```

**Real Example:**
```cpp
// From api_macos.cpp - Getting mouse position
id locationObj = objc_msgSend(
    event,
    sel_registerName(kLocationInWindowSel)
);

CGPoint location = {0, 0};
memcpy(&location, &locationObj, sizeof(CGPoint));

double x = location.x;
double y = location.y;
```

---

<a name="examples"></a>
## Step-by-Step Examples

### Example 1: Creating and Showing a Window

Let's create a window step by step:

```cpp
// Step 1: Get NSWindow class
Class NSWindowClass = objc_getClass("NSWindow");

// Step 2: Define window frame
NSRect frame = {100, 100, 800, 600};  // x, y, width, height

// Step 3: Define style (titlebar, close button, etc.)
unsigned long styleMask = 
    (1 << 0) |  // Titled
    (1 << 1) |  // Closable
    (1 << 2) |  // Miniaturizable
    (1 << 3);   // Resizable

// Step 4: Allocate window
id window = objc_msgSend(
    (id)NSWindowClass,
    sel_registerName("alloc")
);

// Step 5: Initialize window
window = objc_msgSend(
    window,
    sel_registerName("initWithContentRect:styleMask:backing:defer:"),
    frame,      // NSRect frame
    styleMask,  // unsigned long style
    2,          // NSBackingStoreBuffered
    NO          // BOOL defer
);

// Step 6: Set title
Class NSStringClass = objc_getClass("NSString");
id title = objc_msgSend(
    (id)NSStringClass,
    sel_registerName("stringWithUTF8String:"),
    "My Window"
);
objc_msgSend(window, sel_registerName("setTitle:"), title);

// Step 7: Show window
objc_msgSend(
    window,
    sel_registerName("makeKeyAndOrderFront:"),
    nil
);
```

**What This Does:**
1. Gets the NSWindow class
2. Defines where and how big the window should be
3. Defines what features it has (close button, etc.)
4. Allocates memory for the window
5. Initializes it with our settings
6. Sets the window title
7. Makes it visible and brings it to front

---

### Example 2: Handling a Mouse Click

When user clicks, macOS calls our handler:

```cpp
// This function is registered as a method handler
static void mouseDown_handler(id self, SEL _cmd, id event) {
    // Step 1: Get window pointer from self
    // (We stored it when creating the delegate)
    Window* window = getWindowFromSelf(self);
    
    // Step 2: Extract mouse position from event
    id locationObj = objc_msgSend(
        event,
        sel_registerName("locationInWindow")
    );
    
    CGPoint location = {0, 0};
    memcpy(&location, &locationObj, sizeof(CGPoint));
    
    // Step 3: Extract button number
    int button = (int)objc_msgSend(
        event,
        sel_registerName("buttonNumber")
    );
    
    // Step 4: Extract modifier keys (Shift, Ctrl, etc.)
    unsigned int modifiers = (unsigned int)objc_msgSend(
        event,
        sel_registerName("modifierFlags")
    );
    
    // Step 5: Call our C callback
    if (window->mouseDownCallback) {
        window->mouseDownCallback(
            location.x,
            location.y,
            button,
            modifiers,
            window->mouseDownUserData
        );
    }
}
```

---

### Example 3: Creating Custom Delegate Class

We create custom classes to handle events:

```cpp
// Step 1: Define callback storage
static std::unordered_map<id, Window*> delegateToWindow;

// Step 2: Create delegate class
Class createWindowDelegateClass() {
    // Check if class already exists
    Class cls = objc_getClass("CustomWindowDelegate");
    if (cls != nil) return cls;
    
    // Create new class inheriting from NSObject
    Class NSObjectClass = objc_getClass("NSObject");
    cls = objc_allocateClassPair(NSObjectClass, "CustomWindowDelegate", 0);
    
    // Add method for window close
    class_addMethod(
        cls,
        sel_registerName("windowWillClose:"),
        (IMP)windowWillClose_handler,
        "v@:@"  // Return void, takes id, SEL, id
    );
    
    // Add method for window resize
    class_addMethod(
        cls,
        sel_registerName("windowDidResize:"),
        (IMP)windowDidResize_handler,
        "v@:@"
    );
    
    // Register the class
    objc_registerClassPair(cls);
    
    return cls;
}

// Step 3: Create and attach delegate
void attachDelegateToWindow(id window, Window* windowData) {
    // Get our custom class
    Class delegateClass = createWindowDelegateClass();
    
    // Create instance
    id delegate = objc_msgSend(
        (id)delegateClass,
        sel_registerName("alloc")
    );
    delegate = objc_msgSend(
        delegate,
        sel_registerName("init")
    );
    
    // Store window pointer for later retrieval
    delegateToWindow[delegate] = windowData;
    
    // Set as window's delegate
    objc_msgSend(
        window,
        sel_registerName("setDelegate:"),
        delegate
    );
}
```

**What This Does:**
1. Creates a new Objective-C class at runtime
2. Adds methods to handle events
3. Registers the class with the runtime
4. Creates an instance and attaches it to the window
5. Stores our C data so handlers can access it

---

<a name="memory-management"></a>
## Memory Management

### Retain/Release (Manual Reference Counting)

Objective-C uses reference counting for memory management:

```cpp
// When you create an object, retain count = 1
id object = objc_msgSend(..., sel_registerName("alloc"));
object = objc_msgSend(object, sel_registerName("init"));

// Increase retain count
objc_msgSend(object, sel_registerName("retain"));

// Decrease retain count
objc_msgSend(object, sel_registerName("release"));

// Decrease and deallocate when count reaches 0
objc_msgSend(object, sel_registerName("release"));
```

### Autorelease Pools

Autorelease pools manage temporary objects:

```cpp
// Create pool
void* pool = objc_autoreleasePoolPush();

// Create temporary objects
// They'll be released when pool is popped
id temp = objc_msgSend(...);

// Release all objects in pool
objc_autoreleasePoolPop(pool);
```

**Real Example from our code:**
```cpp
class AutoreleasePool {
private:
    void* pool_;
    
public:
    AutoreleasePool() noexcept 
        : pool_(objc_autoreleasePoolPush()) {}
    
    ~AutoreleasePool() noexcept {
        if (pool_) {
            objc_autoreleasePoolPop(pool_);
        }
    }
};

// Usage
{
    AutoreleasePool pool;
    // Create temporary objects...
} // Pool automatically released when scope ends
```

### When to Release

**DO release:**
- Objects created with `alloc`/`init`
- Objects created with `copy`
- Objects you explicitly `retain`

**DON'T release:**
- Objects created with convenience methods (`stringWithUTF8String:`)
- Objects you didn't create
- Objects already managed by autorelease pool

---

<a name="troubleshooting"></a>
## Troubleshooting

### Common Errors

#### 1. Selector Not Found

```cpp
// ERROR: Typo in selector name
objc_msgSend(window, sel_registerName("setTitel:"), title);
//                                      ^^^^^^^^ Wrong!
```

**Fix:**
```cpp
objc_msgSend(window, sel_registerName("setTitle:"), title);
//                                      ^^^^^^^^ Correct
```

#### 2. Wrong Number of Arguments

```cpp
// ERROR: Missing argument
objc_msgSend(window, sel_registerName("setFrame:display:"), frame);
//                                                          Missing second arg!
```

**Fix:**
```cpp
objc_msgSend(window, sel_registerName("setFrame:display:"), frame, YES);
```

#### 3. Sending to Class Instead of Instance

```cpp
// ERROR: Should send to instance
objc_msgSend((id)NSWindowClass, sel_registerName("setTitle:"), title);
//           ^^^^^^^^^^^^^^^^^^^ Class, not instance!
```

**Fix:**
```cpp
objc_msgSend(window, sel_registerName("setTitle:"), title);
//           ^^^^^^ Instance
```

#### 4. Memory Leak

```cpp
// ERROR: Creating object without releasing
for (int i = 0; i < 1000; i++) {
    id obj = objc_msgSend(..., sel_registerName("alloc"));
    obj = objc_msgSend(obj, sel_registerName("init"));
    // No release!
}
```

**Fix:**
```cpp
for (int i = 0; i < 1000; i++) {
    id obj = objc_msgSend(..., sel_registerName("alloc"));
    obj = objc_msgSend(obj, sel_registerName("init"));
    // ... use object ...
    objc_msgSend(obj, sel_registerName("release"));
}
```

---

### Debugging Tips

#### Print Object Description

```cpp
// Get description of any object
id description = objc_msgSend(object, sel_registerName("description"));
const char* str = (const char*)objc_msgSend(
    description,
    sel_registerName("UTF8String")
);
printf("Object: %s\n", str);
```

#### Check if Object Responds to Selector

```cpp
BOOL responds = (BOOL)objc_msgSend(
    object,
    sel_registerName("respondsToSelector:"),
    selector
);

if (!responds) {
    printf("Object doesn't respond to this selector!\n");
}
```

#### Check Object Class

```cpp
Class cls = (Class)objc_msgSend(object, sel_registerName("class"));
const char* className = class_getName(cls);
printf("Object class: %s\n", className);
```

---

## Quick Reference

### Essential Functions

| Function | Purpose | Example |
|----------|---------|---------|
| `objc_getClass()` | Get a class by name | `objc_getClass("NSWindow")` |
| `sel_registerName()` | Get selector by name | `sel_registerName("init")` |
| `objc_msgSend()` | Call a method | `objc_msgSend(obj, sel)` |
| `class_getName()` | Get class name | `class_getName(cls)` |
| `objc_allocateClassPair()` | Create new class | Used for delegates |
| `class_addMethod()` | Add method to class | Add event handlers |

### Common Selectors

| Selector | Purpose |
|----------|---------|
| `"alloc"` | Allocate object |
| `"init"` | Initialize object |
| `"release"` | Decrease retain count |
| `"retain"` | Increase retain count |
| `"description"` | Get string description |
| `"class"` | Get object's class |
| `"respondsToSelector:"` | Check if method exists |

---

## Summary

**You Now Know:**

✅ What the Objective-C runtime is and why we need it  
✅ How to get classes with `objc_getClass`  
✅ How to create objects with `alloc`/`init`  
✅ How to call methods with `objc_msgSend`  
✅ How to work with selectors  
✅ How to handle memory with retain/release  
✅ Common patterns used in our code  
✅ How to debug runtime issues  

**Key Takeaways:**
- `objc_msgSend` is the bridge between C++ and Objective-C
- Always use selectors for method names
- Remember to manage memory (retain/release)
- Cast return values to appropriate types
- Use `memcpy` for struct returns

**Next Steps:**
- Learn about the [Threading And GCD](./04-Threading-And-GCD.md)
- Learn about the [Objective-C Runtime](./03-Objective-C-Runtime.md)
- Learn about the [Function Reference Guide](./02-Function-Reference.md)
- Learn about the [Architecture](./01-Architecture-Overview.md)

---

**Happy Coding!** 🚀

*For more information, see Apple's [Objective-C Runtime Programming Guide](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ObjCRuntimeGuide/Introduction/Introduction.html)*

## Random notes related to vidglue (mostly code-level decisions)

### Opening inputs and using emplace_back

- in code ``inputs.emplace_back(vid)`` is used for creating input directly into vector. Originally ``vid`` used 
raw pointers. These were changed to unique_ptr for following reason 
(this was part of main.cpp before I removed it there for being too long):

>Here it's very important input pointer fields are unique or are otherwise handled properly.
> - f is passed as lvalue because it's an already defined object
> - lvalue calls copy constructor for emplace_back 
> - emplace_back directly constructs a new object inside it by using copied values of f.
> - notably this process uses shallow copies which means all raw pointer fields all also copied -> each raw pointer gets
> pointed at by two different resources
>
> But here's the problem: emplace_back moves vector objects around and this process might temporarily reallocate 
> resources by creating object copies, move them around then call destructors on original object. Because pointers are
> shallow, new one still points to old source which is now nulled/invalid. Then program would crash at v.getDuration 
> when trying to access video format stream of this invalid pointer.
>
>Solution is to use one of the following:
> - unique_ptr fields instead of raw pointers (which is the solution here) to avoid copying,
> - use std::move(f) rvalue conversion to explicitly call move constructor instead of copy constructor,
> - define custom copy and move constructors to avoid this behavior
> - copying and moving is not allowed at all. This is ensured by creating each input as unique_ptr itself into a one 
> vector and passing only its pointer references around inside another vector.
>    - as a safety feature, both move and copy constructors of VideoInput should be disabled

So current solution just uses the first e.g. raw pointer replaced with unique_ptr. And because videoInput also defines
custom destructor, its move constructor is not auto-generated and must be set manually to follow default behavior. 
Copy is disabled altogether as a safety measure. This prevents any attempts to copy unique_ptr and causing a crash.


### GPU decoding

- current GPU decoding is not very efficient because it encodes on GPU then transfers frames back to CPU
- changing this would require a larger rework and I'm not sure if I want to pursue that, would rather keep program 
compact/less convoluted

So for now better keep the current implementation inaccessible


### Cross-platform compatibility

- ffmpeg libraries are compiled directly for each OS if user wants to run this on other than Win platforms (currently 
only Windows .exe is provided)
- GPU CUDA support -> only Nvidia GPUs are supported. Mac doesn't support this at all. If CUDA is not detected, keeps
using CPU
- file paths are ok if videos are placed directly under .exe directory


### Threading

- tested threading with std::thread with mutex and locks, but this didn't improve performance. In fact made it worse
- on top of this, trying to match input video speed became a horrendous mess which eventually made x1.0 speed multiplier
break: it failed to keep track of decoded frames and would render either a single frame or anything between that
depending how decoder calls would mess with one another

Could try again at some point. But knowing how much work it requires vs how much performance increase it's likely to
give, might also just ignore this entirely


### Exceptions

- codebase includes a lot of std::runtime_error exceptions. Normally these are costly to use and slow program down when
caught and handled
- however in vidglue this is not an issue. They are pretty much tied to ffmpeg logic: when ffmpeg function returns 
value < 0, it almost always signals a fatal error (e.g. output video will not work) and processes should stop at once.
- so runtime errors **should** crash the tool and no catching is needed

### Pointer syntax

- raw pointers use syntax `type* var` i.e. pointer is on the side of type
- this clearly differentiates use of `*` symbol in 3 different cases:
    1. raw pointers: left side of expression, tied to type i.e. `a* b`
    2. product operator: in the middle of type and name i.e. `a * b`
    3. pointer dereferencing: right side of expression, tied to name i.e. `a *b`

    Examples:  
    1. `int* a` is a int pointer
    2. `a * b` is a product of a and b when both a, b are numerical types
    3. if `int* a` then `*a` dereferences and produces int type
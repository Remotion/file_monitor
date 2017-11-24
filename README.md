# file_monitor
Lean library to observe file changes in a specific directory path. Primarily meant for asset hotloading in games and 3D engines.

## Usage
You can instantiate a file monitor by using the factory function `file_monitor::make_monitor()`:

```c++
#include <file_monitor/factory.hpp>

int main(int argc, char** argv)
{
  auto monitor = file_monitor::make_monitor();
  /* ... */
}
```

To monitor file changes, you need to call `monitor::start()` with path and then periodically call `poll()`, e.g. in your main loop:
```c++
monitor->start(root / "somefolder");

while (keep_running)
{
  updateYourProgram();
  monitor->poll([](auto const& base_path, auto const& files)
  {
    // base_path is the path that the monitor was started in
    // files is a std::vector of files that have changed
  }
}
```

Note that the monitor will only detect changes on files that existed when it was started. File renaming and creation is not detected.

## Platform support
The file monitor works on Windows, Linux and Mac OS X. It can be built with VC++ 2017, and recent g++ and clang versions. It requires C++14.

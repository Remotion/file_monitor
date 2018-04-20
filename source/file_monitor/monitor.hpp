#pragma once

#include <boost/filesystem/path.hpp>
#include <functional>
#include <vector>

namespace file_monitor
{
using path_t = boost::filesystem::path;
using file_list_t = std::vector<path_t>;

class monitor
{
public:
  using change_event_t = std::function<void(file_list_t const& files)>;

  monitor();
  virtual ~monitor();

  virtual void stop() = 0;
  virtual void start(path_t const& base_path) = 0;

  virtual path_t base_path() const = 0;

  virtual void poll(change_event_t const& consumer) = 0;
};
} // namespace file_monitor

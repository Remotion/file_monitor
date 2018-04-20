#include "win_monitor.hpp"
// #include <array>

using namespace file_monitor;

namespace
{
auto const RESULT_BUFFER_SIZE = 4096;
}

win_monitor::win_monitor()
{
  m_result_buffer.resize(RESULT_BUFFER_SIZE);
}

win_monitor::~win_monitor()
{
  if (m_directory_handle != INVALID_HANDLE_VALUE)
    CloseHandle(m_directory_handle);

  if (m_notify_event != nullptr)
    CloseHandle(m_notify_event);
}

void win_monitor::stop()
{
}

path_t win_monitor::base_path() const
{
  return m_base_path;
}

void win_monitor::start(path_t const& base_path)
{
  if (m_directory_handle != INVALID_HANDLE_VALUE)
    throw std::runtime_error("FileMonitor already started.");

  m_base_path = base_path;

  // Get a handle for the directory to watch
  m_directory_handle = CreateFileA(base_path.string().c_str(), // TODO(Remo 20.04.18): if UNICODE is defined then CreateFile will be replaced by CreateFileW and we get compilation error. 
                                  FILE_LIST_DIRECTORY,
                                  FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
                                  nullptr,
                                  OPEN_EXISTING,
                                  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                  nullptr);

  // Create an event for the polling
  m_notify_event = CreateEventA(nullptr, FALSE, FALSE, "FileChangedEvent"); // TODO(Remo 20.04.18): if UNICODE is defined then CreateEvent will be replaced by CreateEventW and we get compilation error. 

  if (m_notify_event == nullptr)
    throw std::runtime_error("FileMonitor failed to create event.");

  m_overlapped_io.hEvent = m_notify_event;

  listen();
}

void win_monitor::poll(change_event_t const& consumer)
{
  DWORD bytes_written = 0;
  const BOOL result = GetOverlappedResult(m_notify_event, &m_overlapped_io, &bytes_written, FALSE);

  // No results yet?
  if (result == FALSE)
  {
    assert(GetLastError() == ERROR_IO_INCOMPLETE);
    return;
  }

  // We got something
  m_files_changed.clear();

  const char* current_entry = m_result_buffer.data();
  while (true)
  {
    const auto file_info = reinterpret_cast<FILE_NOTIFY_INFORMATION const*>(current_entry);

    if (file_info->Action == FILE_ACTION_MODIFIED ||
        file_info->Action == FILE_ACTION_RENAMED_NEW_NAME)
    {
      // Note that FileName uses 16-bit chars, while the FileNameLength is in bytes!
      const auto character_count = file_info->FileNameLength / 2;
      m_files_changed.push_back({ file_info->FileName, file_info->FileName + character_count });
    }

    // If there's another one, go there
    if (file_info->NextEntryOffset == 0)
      break;

    current_entry += file_info->NextEntryOffset;
  }

  consumer(m_files_changed);

  listen();
}

void file_monitor::win_monitor::listen()
{
  DWORD unused = 0;

  const BOOL result = ReadDirectoryChangesW(m_directory_handle,
                                            m_result_buffer.data(),
                                            DWORD(m_result_buffer.size()),
                                            TRUE,
                                            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
                                            &unused,
                                            &m_overlapped_io,
                                            nullptr);

  if (result == 0)
  {
    LPVOID buffer;

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                         FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr,
                       GetLastError(),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR)&buffer,
                       0,
                       nullptr))
    {
      throw std::runtime_error("Could not format error message for ReadDirectoryChangesW failure");
    }

    const auto message = std::string((const char*)buffer);
    LocalFree(buffer);

    throw std::runtime_error("ReadDirectoryChangesW failed with " + message);
  }
}

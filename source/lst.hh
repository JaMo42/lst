#pragma once
#include "args.hh"
#include "unicode.hh"
#include "options.hh"

enum class FileType
{
  unknown,
  regular,
  directory,
  symlink,
  block,
  character,
  fifo,
  socket,
  // These are special cases for regular files
  executable,  // On Linux: Files with executable permission
               // On Windows: '.exe', '.bat', and '.cmd' files
  temporary  // Files ending with '~', '.tmp', or '.bak'
};

static const char file_type_letters[] = "?-dlbcps--";

struct FileInfo
{
  struct link_target_tag {};

  FileInfo (const fs::path &p, const fs::file_status &s, link_target_tag);

public:
  FileInfo (const fs::path &p, const fs::file_status &in_s);

  ~FileInfo ();

  arena::string name {};
  // Target of link or shortcut
  FileInfo *target { nullptr };
  arena::string owner { "?" };
  arena::string group { "?" };
  std::uintmax_t size {0};
  std::time_t time {0};
  FileType type { FileType::unknown };
  unsigned link_count {0};
  fs::perms perms { fs::perms::none };
  // Used for sorting
  const fs::path _path;
  bool status_failed { false };
};

using FileList = std::list<FileInfo>;

extern std::time_t G_six_months_ago;

#ifdef _WIN32
extern IShellLink *G_sl;
extern IPersistFile *G_pf;
extern bool G_has_shortcut_interfaces;
#endif // _WIN32

extern FileList G_singles;
extern std::list<std::pair<fs::path, FileList>> G_directories;

def list_file (const fs::path &path) -> void;

def list_dir (const fs::path &path) -> void;

def get_owner_and_group (const fs::path &path, arena::string &owner_out,
                         arena::string &group_out) -> bool;

def get_file_time (const fs::path &path, std::time_t &out) -> bool;

#ifdef _WIN32
def get_shortcut_target (const fs::path &path, arena::string &target_out) -> bool;
#endif // _WIN32

def file_type (const fs::file_status &s) -> FileType;

def file_time_to_time_t (fs::file_time_type ft) -> std::time_t;

def sort_files (FileList &files) -> void;

def file_indicator (const FileInfo &f) -> char;

def print_file_name (const FileInfo &f, int width = 0) -> void;

def print_single_column (const FileList &files) -> void;

def print_long (const FileList &files) -> void;

def print_columns (const FileList &files) -> void;

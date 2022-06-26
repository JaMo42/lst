#pragma once
#include "args.hh"
#include "unicode.hh"
#include "options.hh"

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
  fs::file_type type { fs::file_type::unknown };
  unsigned link_count {0};
  fs::perms perms { fs::perms::none };
  // Used for sorting
  const fs::path _path;
  bool status_failed { false };
  bool is_executable { false };
  bool is_temporary { false };
};

using FileList = std::list<FileInfo>;

extern std::time_t G_six_months_ago;

#ifdef _WIN32
extern IShellLink *G_sl;
extern IPersistFile *G_pf;
extern bool G_has_shortcut_interfaces;
#else
extern std::map<pid_t, arena::string> G_users;
extern std::map<uid_t, arena::string> G_groups;
#endif // _WIN32

extern FileList G_singles;
extern std::list<std::pair<fs::path, FileList>> G_directories;

def list_file (const fs::path &path) -> void;

def list_dir (const fs::path &path) -> void;

#ifdef _WIN32
def get_owner_and_group (HANDLE file_handle, arena::string &owner_out,
                         arena::string &group_out) -> bool;

def get_file_time (HANDLE file_handle, std::time_t &out) -> bool;

def get_shortcut_target (const fs::path &path, arena::string &target_out) -> bool;

def get_file_size (BY_HANDLE_FILE_INFORMATION *file_info) -> std::uintmax_t;

def get_link_count (BY_HANDLE_FILE_INFORMATION *file_info) -> unsigned;

#else // _WIN32

def get_owner_and_group (struct stat *sb, arena::string &owner_out,
                         arena::string &group_out) -> bool;

def get_file_time (struct stat *sb, std::time_t &out) -> bool;

def get_file_size (struct stat *sb) -> std::uintmax_t;

def get_link_count (struct stat *sb) -> unsigned;
#endif // _WIN32

def sort_files (FileList &files) -> void;

def file_indicator (const FileInfo &f) -> char;

def print_file_name (const FileInfo &f, int width = 0) -> void;

def print_single_column (const FileList &files) -> void;

def print_long (const FileList &files) -> void;

def print_columns (const FileList &files) -> void;

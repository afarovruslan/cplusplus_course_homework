#pragma once
#include <climits>
#include <dirent.h>
#include <string>
#include <stack>
#include <sys/stat.h>
#include <memory>

enum class directory_options : unsigned char {
  none = 0,
  follow_directory_symlink = 1,
  skip_permission_denied = 2
};

directory_options operator|(const directory_options& first, const directory_options& other);
directory_options& operator|=(directory_options& first, const directory_options& other);
directory_options operator&(const directory_options& first, const directory_options& other);
directory_options& operator&=(directory_options& first, const directory_options& other);
directory_options operator^(const directory_options& first, const directory_options& other);
directory_options& operator^=(directory_options& first, const directory_options& other);
directory_options operator~(const directory_options& value);

class directory_entry {
 private:
  std::string path_;
  const char* name_;
  struct stat lstat_;
  struct stat stat_;

  static const size_t kCountPermissionBits = 9;

 public:
  directory_entry() = default;
  directory_entry(std::string&& path, const char* name);
  [[nodiscard]] const char* path() const;
  [[nodiscard]] const char* name() const;
  [[nodiscard]] bool is_directory() const;
  [[nodiscard]] bool is_symlink() const;
  [[nodiscard]] bool is_regular_file() const;
  [[nodiscard]] bool is_block_file() const;
  [[nodiscard]] bool is_character_file() const;
  [[nodiscard]] bool is_socket() const;
  [[nodiscard]] bool is_fifo() const;
  [[nodiscard]] std::uintmax_t file_size() const;
  [[nodiscard]] std::uintmax_t hard_link_count() const;
  [[nodiscard]] std::uintmax_t last_write_time() const;
  [[nodiscard]] unsigned int permissions() const;
};

class recursive_directory_iterator {
 private:
  struct ControlBlock {
    directory_options flags;
    size_t init_realpath_len;
    const char* init_path;
    char curr_path[PATH_MAX]{};
    std::stack<std::pair<size_t, DIR*>> stack;

    ControlBlock(const char* init_path, directory_options flags);
    ControlBlock(const ControlBlock& cb);
  };
  std::shared_ptr<ControlBlock> cb_;
  directory_entry curr_entry_;

  void UpdateEntry();
  void Pop();
  void OpenDir(size_t len_path);
  static void Deleter(ControlBlock* cb);
  bool CheckStack();

 public:
  using value_type = directory_entry;
  using difference_type = std::ptrdiff_t;
  using pointer = const directory_entry*;
  using reference = const directory_entry&;
  using iterator_category = std::input_iterator_tag;

  recursive_directory_iterator() = default;
  recursive_directory_iterator(const recursive_directory_iterator& it) = default;
  recursive_directory_iterator(const char* path, directory_options flags = directory_options::none);
  recursive_directory_iterator& operator++();
  recursive_directory_iterator operator++(int);
  reference operator*() const;
  pointer operator->() const;
  bool operator==(const recursive_directory_iterator& rdi) const;
  bool operator!=(const recursive_directory_iterator& rdi) const;
  void pop();
  [[nodiscard]] int depth() const;
};

recursive_directory_iterator begin(recursive_directory_iterator iter);
recursive_directory_iterator end(recursive_directory_iterator);

std::ostream& operator<<(std::ostream& out, const directory_entry& de);
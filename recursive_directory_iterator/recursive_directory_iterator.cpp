#include "recursive_directory_iterator.h"
#include <cstring>
#include <cstdlib>
#include <iostream>

recursive_directory_iterator begin(recursive_directory_iterator iter) {
  return iter;
}

recursive_directory_iterator end(recursive_directory_iterator) {
  return {};
}

bool directory_entry::is_block_file() const {
  return S_ISBLK(stat_.st_mode);
}

bool directory_entry::is_character_file() const {
  return S_ISCHR(stat_.st_mode);
}

bool directory_entry::is_directory() const {
  return S_ISDIR(stat_.st_mode);
}

bool directory_entry::is_fifo() const {
  return S_ISFIFO(stat_.st_mode);
}

bool directory_entry::is_regular_file() const {
  return S_ISREG(stat_.st_mode);
}

bool directory_entry::is_socket() const {
  return S_ISSOCK(stat_.st_mode);
}

bool directory_entry::is_symlink() const {
  return S_ISLNK(lstat_.st_mode);
}

const char* directory_entry::path() const {
  return path_.c_str();
}

std::ostream& operator<<(std::ostream& out, const directory_entry& de) {
  out << de.path();
  return out;
}

directory_options operator|(const directory_options& first, const directory_options& other) {
  return static_cast<directory_options>(static_cast<unsigned char>(first) | static_cast<unsigned char>(other));
}

directory_options& operator|=(directory_options& first, const directory_options& other) {
  first = first | other;
  return first;
}

directory_options operator&(const directory_options& first, const directory_options& other) {
  return static_cast<directory_options>(static_cast<unsigned char>(first) & static_cast<unsigned char>(other));
}

directory_options& operator&=(directory_options& first, const directory_options& other) {
  first = first & other;
  return first;
}

directory_options operator^(const directory_options& first, const directory_options& other) {
  return static_cast<directory_options>(static_cast<unsigned char>(first) ^ static_cast<unsigned char>(other));
}

directory_options& operator^=(directory_options& first, const directory_options& other) {
  first = first ^ other;
  return first;
}

directory_options operator~(const directory_options& value) {
  return value ^ (directory_options::skip_permission_denied |  directory_options::follow_directory_symlink);
}

recursive_directory_iterator::ControlBlock::ControlBlock(const char *init_path, directory_options flags)
    : flags(flags), init_path(init_path) {
  realpath(init_path, curr_path);
  init_realpath_len = strlen(curr_path);
}

recursive_directory_iterator::ControlBlock::ControlBlock(const recursive_directory_iterator::ControlBlock& cb)
    : flags(cb.flags),
      init_realpath_len(cb.init_realpath_len),
      init_path(cb.init_path),
      stack(cb.stack) {
  std::strcpy(curr_path, cb.curr_path);
}

bool recursive_directory_iterator::operator==(const recursive_directory_iterator& rdi) const {
  return cb_ == rdi.cb_;
}

bool recursive_directory_iterator::operator!=(const recursive_directory_iterator& rdi) const {
  return cb_ != rdi.cb_;
}

void recursive_directory_iterator::pop() {
  Pop();
  ++*this;
}

const char* directory_entry::name() const {
  return name_;
}

recursive_directory_iterator& recursive_directory_iterator::operator++() {
  if (curr_entry_.is_directory() and std::strcmp(curr_entry_.name(), ".") != 0 and
      std::strcmp(curr_entry_.name(), "..") != 0 and
      ((cb_->flags & directory_options::follow_directory_symlink) == directory_options::follow_directory_symlink or
      !curr_entry_.is_symlink())) {
    auto j = cb_->stack.top().first;
    size_t i = 0;
    cb_->curr_path[i + j] = '/';
    ++j;
    while (curr_entry_.name()[i] != '\0') {
      cb_->curr_path[i + j] = curr_entry_.name()[i];
      ++i;
    }
    cb_->curr_path[i + j] = curr_entry_.name()[i];
    OpenDir(i + j);
  }
  UpdateEntry();
  return *this;
}

recursive_directory_iterator recursive_directory_iterator::operator++(int) {
  auto copy = *this;
  ++*this;
  return copy;
}

int recursive_directory_iterator::depth() const {
  return static_cast<int>(cb_->stack.size() - 1);
}

recursive_directory_iterator::recursive_directory_iterator(const char* path, directory_options flags)
    : cb_(new ControlBlock(path, flags), Deleter) {
  cb_->stack.emplace(strlen(cb_->curr_path), opendir(cb_->curr_path));
  if (cb_->stack.top().second == nullptr) {
    auto err_copy = errno;
    cb_.reset();
    if (err_copy == ENOTDIR) {
      throw std::runtime_error(std::string("no such directory ") + path);
    }
    if (err_copy == ENOENT) {
      throw std::runtime_error(std::string("Directory does not exist, or name is an empty string ") + path);
    }
    if (err_copy == EACCES and
        (flags & directory_options::skip_permission_denied) != directory_options::skip_permission_denied) {
      throw std::runtime_error("permission denied");
    }
  } else {
    UpdateEntry();
  }
}

bool recursive_directory_iterator::CheckStack() {
  bool ans = cb_->stack.empty();
  if (ans) {
    cb_.reset();
  }
  return ans;
}

void recursive_directory_iterator::Pop() {
  closedir(cb_->stack.top().second);
  cb_->stack.pop();
}

void recursive_directory_iterator::UpdateEntry() {
  if (CheckStack()) {
    return;
  }
  auto new_dirent = readdir(cb_->stack.top().second);
  if (new_dirent == nullptr) {
    Pop();
    if (CheckStack()) {
      return;
    }
    cb_->curr_path[cb_->stack.top().first] = '\0';
    return UpdateEntry();
  }
  if (std::strcmp(new_dirent->d_name, ".") == 0 or std::strcmp(new_dirent->d_name, "..") == 0) {
    return UpdateEntry();
  }
  std::string path_to_file(cb_->init_path);
  path_to_file += (cb_->curr_path + cb_->init_realpath_len);
  path_to_file.push_back('/');
  curr_entry_ = directory_entry(std::move(path_to_file), new_dirent->d_name);
}

std::uintmax_t directory_entry::file_size() const {
  return stat_.st_size;
}

std::uintmax_t directory_entry::hard_link_count() const {
  return stat_.st_nlink;
}

unsigned int directory_entry::permissions() const {
  return stat_.st_mode % (1 << kCountPermissionBits);
}

std::uintmax_t directory_entry::last_write_time() const {
  return stat_.st_mtim.tv_sec;
}

void recursive_directory_iterator::OpenDir(size_t len_path) {
  cb_->stack.emplace(len_path, opendir(cb_->curr_path));
  if (cb_->stack.top().second == nullptr) {
    cb_->stack.pop();
    if (CheckStack()) {
      return;
    }
    cb_->curr_path[cb_->stack.top().first] = '\0';
    if (errno == EACCES and
        (cb_->flags & directory_options::skip_permission_denied) != directory_options::skip_permission_denied) {
      throw std::runtime_error("permission denied");
    }
  }
}

recursive_directory_iterator::pointer recursive_directory_iterator::operator->() const {
  return &curr_entry_;
}

recursive_directory_iterator::reference recursive_directory_iterator::operator*() const {
  return curr_entry_;
}

directory_entry::directory_entry(std::string&& path, const char* name)
    : path_(path), name_(name) {
  char* full_path = realpath(path.c_str(), nullptr);
  std::string str(full_path);
  free(full_path);
  path_ += name;
  str.push_back('/');
  str += name;
  lstat(str.c_str(), &lstat_);
  stat(str.c_str(), &stat_);
}

void recursive_directory_iterator::Deleter(recursive_directory_iterator::ControlBlock *cb) {
  while (!cb->stack.empty()) {
    closedir(cb->stack.top().second);
    cb->stack.pop();
  }
  delete cb;
}
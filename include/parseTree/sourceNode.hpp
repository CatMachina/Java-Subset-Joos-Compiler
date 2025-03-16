#pragma once

#include <iostream>
#include <unordered_map>

namespace source {
// struct SourceLocation
// {
//   int fileID; // Unique ID assigned by SourceManager
//   int line;
//   int column;

//   bool operator==(const SourceLocation &other) const
//   {
//     return fileID == other.fileID && line == other.line && column ==
//     other.column;
//   }
// };

struct SourceRange {
  int fileID; // Unique ID assigned by SourceManager
  int first_line;
  int last_line;
  int first_column;
  int last_column;

  SourceRange()
      : fileID(0), first_line(0), last_line(0), first_column(0),
        last_column(0) {}

  friend std::ostream &operator<<(std::ostream &os, const SourceRange &range) {
    os << "File ID: " << range.fileID << ", "
       << "Lines: " << range.first_line << "-" << range.last_line << ", "
       << "Columns: " << range.first_column << "-" << range.last_column;
    return os;
  }

  bool operator<=(const SourceRange &other) const {
    if (fileID != other.fileID)
      return false;
    // // if (fileID != other.fileID) return fileID < other.fileID;
    if (first_line != other.first_line)
      return first_line <= other.first_line;
    if (last_line != other.last_line)
      return last_line <= other.last_line;
    if (first_column != other.first_column)
      return first_column <= other.first_column;
    return last_column <= other.last_column;
  }
};

class SourceManager {
  std::unordered_map<int, std::string> fileTable; // fileID → filename
  int nextFileID = 1;                             // Auto-incremented file ID

public:
  int addFile(const std::string &filename) {
    int id = nextFileID++;
    fileTable[id] = filename;
    return id;
  }

  std::string getFilename(int fileID) const {
    auto it = fileTable.find(fileID);
    return it != fileTable.end() ? it->second : "<unknown>";
  }

  int getFileID() { return nextFileID; }

  void printRange(const SourceRange &loc) const {
    std::cout << getFilename(loc.fileID) << " " << loc.first_line << ":"
              << loc.first_column << " - " << loc.last_line << ":"
              << loc.last_column << "\n";
  }
};
} // namespace source
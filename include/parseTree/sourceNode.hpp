#pragma once

#include <unordered_map>

namespace source
{
  // struct SourceLocation
  // {
  //   int fileID; // Unique ID assigned by SourceManager
  //   int line;
  //   int column;

  //   bool operator==(const SourceLocation &other) const
  //   {
  //     return fileID == other.fileID && line == other.line && column == other.column;
  //   }
  // };

  struct SourceRange
  {
    int fileID; // Unique ID assigned by SourceManager
    int first_line;
    int last_line;
    int first_column;
    int last_column;
  };

  class SourceManager
  {
    std::unordered_map<int, std::string> fileTable; // fileID → filename
    int nextFileID = 1;                             // Auto-incremented file ID

  public:
    int addFile(const std::string &filename)
    {
      int id = nextFileID++;
      fileTable[id] = filename;
      return id;
    }

    std::string getFilename(int fileID) const
    {
      auto it = fileTable.find(fileID);
      return it != fileTable.end() ? it->second : "<unknown>";
    }

    void printRange(const SourceRange &loc) const
    {
      std::cout << getFilename(loc.fileID) << " " << loc.first_line << ":" << loc.first_column << " - " << loc.last_line << ":" << loc.last_column << "\n";
    }
  };
} // sourcenode
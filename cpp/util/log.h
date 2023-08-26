#ifndef DECODER_UTIL_LOG_H_
#define DECODER_UTIL_LOG_H_

#include <iostream>
#include <string>

static inline void log(std::string file, int line, std::string msg, std::ostream& out = std::cout)
{
    out << file << ":" << line << ": " <<  msg << '\n';
}

static inline std::string log_str(std::string file, int line, std::string msg)
{
  return std::string(file) + ": " + std::to_string(line) + ": " + std::string(msg);
}

#define LOG(x) log(__FILE__, __LINE__, x)
#define LOG_STRING(x) log_str(__FILE__, __LINE__, x)
#define LOG_OSTREAM(x, y) log(__FILE__, __LINE__, x, y)
#define LOG_ERROR(x) LOG_OSTREAM(x, std::cerr)

#endif // DECODER_UTIL_LOG_H_
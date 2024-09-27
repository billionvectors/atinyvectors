// ErrorCode.hpp
#ifndef __ATINYVECTORS_ERROR_CODE_HPP__
#define __ATINYVECTORS_ERROR_CODE_HPP__

enum class ATVErrorCode {
    JSON_PARSE_ERROR = 1001,
    SQLITE_ERROR = 1002,
    MEMORY_ALLOCATION_ERROR = 1003,
    UNKNOWN_ERROR = 1099,
    // Add other error codes as needed
};

#endif // ERROR_CODE_HPP

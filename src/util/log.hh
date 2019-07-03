#ifndef _AXN_LOG_HH_
#define _AXN_LOG_HH_

#include <cstdlib>
#include <boost/log/common.hpp>

namespace axn {

enum class SevLevel {
    kDebug,
    kInfo,
    kWarn,
    kError,
    kFatal
};

extern boost::log::sources::severity_logger_mt<SevLevel>& axn_logger;

#define LOG_DEBUG BOOST_LOG_SEV(axn::axn_logger, axn::SevLevel::kDebug)
#define LOG_INFO BOOST_LOG_SEV(axn::axn_logger, axn::SevLevel::kInfo)
#define LOG_WARN BOOST_LOG_SEV(axn::axn_logger, axn::SevLevel::kWarn)
#define LOG_ERROR BOOST_LOG_SEV(axn::axn_logger, axn::SevLevel::kError)
#define LOG_FATAL \
    for (;; LogFlush(), std::abort()) BOOST_LOG_SEV(axn::axn_logger, axn::SevLevel::kFatal)

void LogFlush();

// Wrapper of strerror_r().
const char* StrError(int err_num);

}
#endif

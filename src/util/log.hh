#ifndef _AXS_LOG_HH_
#define _AXS_LOG_HH_

#include <cstdlib>
#include <boost/log/common.hpp>

namespace axs {

enum class SevLevel {
    kDebug,
    kInfo,
    kWarn,
    kError,
    kFatal
};

extern boost::log::sources::severity_logger_mt<SevLevel>& axs_logger;

#define LOG_DEBUG BOOST_LOG_SEV(axs::axs_logger, axs::SevLevel::kDebug)
#define LOG_INFO BOOST_LOG_SEV(axs::axs_logger, axs::SevLevel::kInfo)
#define LOG_WARN BOOST_LOG_SEV(axs::axs_logger, axs::SevLevel::kWarn)
#define LOG_ERROR BOOST_LOG_SEV(axs::axs_logger, axs::SevLevel::kError)
#define LOG_FATAL \
    for (;; LogFlush(), std::abort()) BOOST_LOG_SEV(axs::axs_logger, axs::SevLevel::kFatal)

void LogFlush();

// Wrapper of strerror_r().
const char* StrError(int err_num);

}
#endif

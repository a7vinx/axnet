#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/log/core.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/support/date_time.hpp>

#include "log.hh"

namespace axs {

namespace log = boost::log;
namespace log_src = boost::log::sources;
namespace log_sink = boost::log::sinks;
namespace log_kw = boost::log::keywords;
namespace log_expr = boost::log::expressions;
namespace log_attr = boost::log::attributes;

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(axs_logger_tag,
                                       log_src::severity_logger_mt<SevLevel>)

log_src::severity_logger_mt<SevLevel>& axs_logger = axs_logger_tag::get();

log::formatting_ostream& operator<<(
    log::formatting_ostream& strm,
    log::to_log_manip<SevLevel, void> const& manip) {
    static const char* sev_strs[] = {
        "DEBUG",
        "INFO ",
        "WARN ",
        "ERROR",
        "FATAL"
    };
    SevLevel level = manip.get();
    int level_int = static_cast<int>(level);
    if (level_int < sizeof(sev_strs) / sizeof(*sev_strs)) {
        strm << sev_strs[level_int];
    } else {
        strm << level_int;
    }
    return strm;
}

void LogFlush() {
    log::core::get()->flush();
}

namespace {
// Wrap initialization and cleaning operation of logging in this RAII-style class.
class LogCore {
public:
    LogCore() {
        using sink_t = log_sink::asynchronous_sink<log_sink::text_file_backend>;
        auto backendp = boost::make_shared<log_sink::text_file_backend>(
                            log_kw::file_name = "axs_%Y_%m_%d.log",
                            log_kw::rotation_size = 5 * 1024 * 2014,
                            log_kw::time_based_rotation =
                                log_sink::file::rotation_at_time_point(12, 0, 0)
                        );
        auto sinkp = boost::make_shared<sink_t>(backendp);
        sinkp->set_formatter(
            log_expr::stream
                << log_expr::format_date_time<boost::posix_time::ptime>(
                       "TimeStamp", "%Y-%m-%d %H:%M:%S") << " "
                << log_expr::attr<log_attr::current_thread_id::value_type>(
                       "Tid") << " "
                << log_expr::attr<SevLevel>("Severity") << " "
                << log_expr::smessage
        );
        auto corep = log::core::get();
        corep->add_global_attribute("Tid", log_attr::current_thread_id());
        corep->add_global_attribute("TimeStamp", log_attr::local_clock());
        corep->add_sink(sinkp);
    }
    ~LogCore() { LogFlush(); }
};

LogCore log_core{};
}  // unnamed namespace

}

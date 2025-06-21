// Modules/Utils/DateUtils.cpp
#include "DateUtils.h"
#include "Logger.h" // Standard includes

// Only include QDateTime if QT_CORE_LIB is defined (e.g., in a Qt project)
#ifdef QT_CORE_LIB
#include <QDateTime>
#endif

namespace ERP {
    namespace Utils {

        std::chrono::system_clock::time_point DateUtils::now() {
            return std::chrono::system_clock::now();
        }

        std::string DateUtils::formatDateTime(const std::chrono::system_clock::time_point& time, const std::string& format) {
            std::time_t tt = std::chrono::system_clock::to_time_t(time);
            std::tm tm_struct = *std::localtime(&tt);
            std::stringstream ss;
            ss << std::put_time(&tm_struct, format.c_str());
            return ss.str();
        }

        std::optional<std::chrono::system_clock::time_point> DateUtils::parseDateTime(const std::string& dateTimeString, const std::string& format) {
            std::tm tm_struct = {}; // Initialize to all zeros
            std::stringstream ss(dateTimeString);
            ss >> std::get_time(&tm_struct, format.c_str());

            if (ss.fail()) {
                ERP::Logger::Logger::getInstance().error("DateUtils: Failed to parse datetime string '" + dateTimeString + "' with format '" + format + "'.");
                return std::nullopt;
            }

            // Convert tm_struct to time_t, then to time_point
            std::time_t tt = std::mktime(&tm_struct);
            if (tt == -1) { // mktime returns -1 on error (e.g., invalid date values)
                ERP::Logger::Logger::getInstance().error("DateUtils: Invalid date/time values after parsing '" + dateTimeString + "'.");
                return std::nullopt;
            }
            return std::chrono::system_clock::from_time_t(tt);
        }

#ifdef QT_CORE_LIB
        std::chrono::system_clock::time_point DateUtils::qDateTimeToTimePoint(const QDateTime& qDateTime) {
            // QDateTime::toSecsSinceEpoch() returns qint64, which is long long.
            // std::chrono::seconds can be constructed from long long.
            // Then from_time_t converts to time_point.
            return std::chrono::system_clock::from_time_t(qDateTime.toSecsSinceEpoch());
        }

        QDateTime DateUtils::timePointToQDateTime(const std::chrono::system_clock::time_point& timePoint) {
            std::time_t tt = std::chrono::system_clock::to_time_t(timePoint);
            return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(tt));
        }
#endif

    } // namespace Utils
} // namespace ERP
// Modules/Utils/DateUtils.h
#ifndef MODULES_UTILS_DATEUTILS_H
#define MODULES_UTILS_DATEUTILS_H

#include <string>       // For std::string
#include <chrono>       // For std::chrono::system_clock::time_point
#include <optional>     // For std::optional
#include <iomanip>      // For std::put_time
#include <sstream>      // For std::stringstream
#include <ctime>        // For std::mktime, std::tm

// Rút gọn includes
#include "Common.h"     // For ERP::Common::DATETIME_FORMAT

namespace ERP {
    namespace Utils {

        /**
         * @brief The DateUtils class provides utility functions for date and time manipulation.
         */
        class DateUtils {
        public:
            /**
             * @brief Gets the current system time as a time_point.
             * @return A std::chrono::system_clock::time_point representing the current time.
             */
            static std::chrono::system_clock::time_point now();

            /**
             * @brief Formats a time_point into a string according to a specified format.
             * @param time The time_point to format.
             * @param format The format string (e.g., "%Y-%m-%d %H:%M:%S").
             * @return The formatted date/time string.
             */
            static std::string formatDateTime(const std::chrono::system_clock::time_point& time, const std::string& format);

            /**
             * @brief Parses a date/time string into a time_point according to a specified format.
             * @param dateTimeString The date/time string to parse.
             * @param format The format string used in dateTimeString.
             * @return An optional time_point if parsing is successful, std::nullopt otherwise.
             */
            static std::optional<std::chrono::system_clock::time_point> parseDateTime(const std::string& dateTimeString, const std::string& format);

            /**
             * @brief Converts a QDateTime object to a std::chrono::system_clock::time_point.
             * This function provides interoperability with Qt's datetime objects.
             * @param qDateTime The QDateTime object to convert.
             * @return The corresponding std::chrono::system_clock::time_point.
             * @note This requires Qt headers in the .cpp, but keeps this header clean.
             */
             // Declared only if Qt is definitely available and needed for core utilities.
             // For now, it's safer to keep Qt-specific conversions in UI layer or separate Qt-specific utils file.
             // However, for project's consistency, it was defined in .cpp for now.
#ifdef QT_CORE_LIB // Only declare if Qt is enabled
            static std::chrono::system_clock::time_point qDateTimeToTimePoint(const QDateTime& qDateTime);
            static QDateTime timePointToQDateTime(const std::chrono::system_clock::time_point& timePoint);
#endif


        private:
            // Private constructor/destructor to make it a purely static utility class
            DateUtils() = delete;
            ~DateUtils() = delete;
        };

    } // namespace Utils
} // namespace ERP

#endif // MODULES_UTILS_DATEUTILS_H
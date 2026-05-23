#ifndef GRID_GAME_LOG_HPP_INCLUDED
#define GRID_GAME_LOG_HPP_INCLUDED

#include "libsim/types.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace grid::libsim {

    /**
     * @brief A single entry in the game's event log.
     *
     * Captures user-facing game events (communications, actions, etc.)
     * for later recall, display, or scrollback.
     */
    struct LogEntry {
        std::chrono::steady_clock::time_point timestamp;
        std::string source;    ///< Who created the event (agent name, etc.)
        std::string location;  ///< Where it happened (string representation)
        Senses sense;          ///< Which sense channel (Hearing, Sight, etc.)
        std::string message;   ///< The content of the event

        /// Format this entry as a human-readable string.
        std::string format() const
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                timestamp.time_since_epoch());
            double secs = elapsed.count() / 1000.0;

            std::ostringstream oss;
            oss << "[" << std::fixed << std::setprecision(3) << secs << "s] "
                << "[" << sense << "] "
                << source << " @ " << location << ": "
                << message;
            return oss.str();
        }
    };

    /**
     * @brief Thread-safe log of user-facing game events.
     *
     * This is NOT a developer debug log. It stores in-world events
     * (agent speech, environmental happenings, etc.) that the player
     * will eventually be able to scroll back through.
     *
     * Optionally writes formatted entries to a log file when a path
     * is provided via setLogFile().
     */
    class GameLog {
    public:
        ~GameLog()
        {
            if (m_file.is_open()) {
                m_file.close();
            }
        }

        /// Open (or replace) the file that entries are appended to.
        /// Pass an empty string to disable file output.
        void setLogFile(const std::string& path)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_file.is_open()) {
                m_file.close();
            }
            if (!path.empty()) {
                m_file.open(path, std::ios::out | std::ios::trunc);
            }
        }

        void log(const std::string& source,
                 const std::string& location,
                 Senses sense,
                 const std::string& message)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            LogEntry entry{
                std::chrono::steady_clock::now(),
                source, location, sense, message
            };
            if (m_file.is_open()) {
                m_file << entry.format() << "\n";
                m_file.flush();
            }
            m_entries.push_back(std::move(entry));
        }

        /// Return a snapshot of all entries (thread-safe copy).
        std::vector<LogEntry> getEntries() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_entries;
        }

        /// Return the most recent entry, or std::nullopt if empty.
        std::optional<LogEntry> getLastEntry() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_entries.empty()) return std::nullopt;
            return m_entries.back();
        }

        /// Return the last @p n entries (oldest first), or fewer if
        /// the log has fewer than @p n total.
        std::vector<LogEntry> getLastEntries(size_t n) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (n >= m_entries.size()) return m_entries;
            return std::vector<LogEntry>(m_entries.end() - n, m_entries.end());
        }

        /// Return the number of entries.
        size_t size() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_entries.size();
        }

        /// Clear all entries.
        void clear()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_entries.clear();
        }

    private:
        mutable std::mutex m_mutex;
        std::vector<LogEntry> m_entries;
        std::ofstream m_file;
    };

} // namespace grid::libsim

#endif // GRID_GAME_LOG_HPP_INCLUDED

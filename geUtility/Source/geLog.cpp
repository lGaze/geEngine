/*****************************************************************************/
/**
 * @file    geLog.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2016/09/19
 * @brief   Used for logging messages
 *
 * Can categorize messages according to channels, save the log to a
 * file and send out callbacks when a new message is added
 *
 * @bug	    No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
* Includes
*/
/*****************************************************************************/
#include "geLog.h"
#include "geException.h"

namespace geEngineSDK {
  Log::~Log() {
    clear();
  }

  void
  Log::logMsg(const String& message, uint32 channel) {
    RecursiveLock lock(m_mutex);
    m_unreadEntries.push(LogEntry(message, channel));
  }

  void
  Log::clear() {
    RecursiveLock lock(m_mutex);
    m_entries.clear();

    while (!m_unreadEntries.empty()) {
      m_unreadEntries.pop();
    }

    ++m_hash;
  }

  void
  Log::clear(uint32 channel) {
    RecursiveLock lock(m_mutex);

    Vector<LogEntry> newEntries;
    for (auto& entry : m_entries) {
      if (entry.getLogChannel() == channel) {
        continue;
      }

      newEntries.push_back(entry);
    }

    m_entries = newEntries;

    Queue<LogEntry> newUnreadEntries;
    while (!m_unreadEntries.empty()) {
      LogEntry entry = m_unreadEntries.front();
      m_unreadEntries.pop();

      if (entry.getLogChannel() == channel) {
        continue;
      }

      newUnreadEntries.push(entry);
    }

    m_unreadEntries = newUnreadEntries;
    ++m_hash;
  }

  bool
  Log::getUnreadEntry(LogEntry& entry) {
    RecursiveLock lock(m_mutex);

    if (m_unreadEntries.empty()) {
      return false;
    }

    entry = m_unreadEntries.front();
    m_unreadEntries.pop();
    m_entries.push_back(entry);
    ++m_hash;

    return true;
  }

  bool
  Log::getLastEntry(LogEntry& entry) {
    if (m_entries.empty()) {
      return false;
    }

    entry = m_entries.back();

    return true;
  }

  Vector<LogEntry>
  Log::getEntries() const {
    RecursiveLock lock(m_mutex);
    return m_entries;
  }

  Vector<LogEntry>
  Log::getAllEntries() const {
    Vector<LogEntry> entries;
    
    {
      RecursiveLock lock(m_mutex);

      for (auto& entry : m_entries) {
        entries.push_back(entry);
      }

      Queue<LogEntry> unreadEntries = m_unreadEntries;
      while (!unreadEntries.empty()) {
        entries.push_back(unreadEntries.front());
        unreadEntries.pop();
      }
    }
    
    return entries;
  }
}

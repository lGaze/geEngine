/*****************************************************************************/
/**
 * @file    geStringTableManager.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/11/01
 * @brief   Manages string tables used for localizing text.
 *
 * Manages string tables used for localizing text. Allows you to add and remove
 * different tables and change the active language.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geStringTableManager.h"

namespace geEngineSDK {
  void
  StringTableManager::setActiveLanguage(Language language) {
    if (language != m_activeLanguage) {
      m_activeLanguage = language;

      for (auto& tablePair : m_tables) {
        tablePair.second->setActiveLanguage(language);
      }
    }
  }

  HStringTable
  StringTableManager::getTable(uint32 id) {
    auto iterFind = m_tables.find(id);
    if (iterFind != m_tables.end()) {
      return iterFind->second;
    }

    HStringTable newTable = StringTable::create();
    setTable(id, newTable);

    return newTable;
  }

  void
  StringTableManager::removeTable(uint32 id) {
    m_tables.erase(id);
  }

  void
  StringTableManager::setTable(uint32 id, const HStringTable& table) {
    m_tables[id] = table;

    if (table != nullptr) {
      table->setActiveLanguage(m_activeLanguage);
    }
  }

  StringTableManager&
  g_stringTableManager() {
    return StringTableManager::instance();
  }
}

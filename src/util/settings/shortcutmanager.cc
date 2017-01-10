/*
 * Copyright 2016 CodiLime
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <QSettings>
#include <QDebug>

#include "util/settings/shortcutmanager.h"

namespace veles {
namespace util {
namespace settings {


ShortcutManager::ShortcutManager() {}
ShortcutManager::~ShortcutManager() {}

QShortcut *ShortcutManager::managedShortcut(const QString &name,
					    const QString &whatsThis,
					    const QString &dfltkey,
					    QWidget *parent,
					    Qt::ShortcutContext context) {

  QSettings settings;
  QShortcut *shortcut = new QShortcut(parent);

  shortcut->setWhatsThis(whatsThis);
  shortcut->setContext(context);

  QVariant val = settings.value("shortcut/" + name, dfltkey);
  QKeySequence keysequence = QKeySequence(val.toString());

  shortcut->setKey(keysequence);


  shortcutList.append(shortcut);

  return shortcut;
}


// Default
  ShortcutManager *shortcutManager = new ShortcutManager();

}  // namespace settings
}  // namespace util
}  // namespace veles

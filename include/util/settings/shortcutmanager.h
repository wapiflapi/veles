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
#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QShortcut>

namespace veles {
namespace util {
namespace settings {

class ShortcutManager : public QObject {

  Q_OBJECT
 public:

  explicit ShortcutManager();
  ~ShortcutManager();

  QShortcut *managedShortcut(const QString &name,
			     const QString &whatsThis,
			     const QString &key,
  			     QWidget *parent,
  			     Qt::ShortcutContext context = Qt::WindowShortcut);



  template <typename T, typename TM>
  QShortcut *managedShortcut(const QString &name,
			     const QString &whatsThis,
			     const QString &key,
  			     T *parent,
			     void (TM::*member)() = Q_NULLPTR,
			     void (TM::*ambiguousMember)() = Q_NULLPTR,
  			     Qt::ShortcutContext context = Qt::WindowShortcut) {

    QShortcut *shortcut = managedShortcut(name, whatsThis, key, parent, context);

    if (member) {
      connect(shortcut, &QShortcut::activated, parent, member);
    }
    if (ambiguousMember) {
      connect(shortcut, &QShortcut::activatedAmbiguously, parent, ambiguousMember);
    }

    return shortcut;
  }

  QList<QShortcut *> shortcutList;

};

extern ShortcutManager *shortcutManager;

}  // namespace settings
}  // namespace util
}  // namespace veles


#endif

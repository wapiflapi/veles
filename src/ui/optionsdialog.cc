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
#include "include/ui/optionsdialog.h"
#include <QMessageBox>
#include "ui_optionsdialog.h"
#include "util/settings/hexedit.h"
#include "util/settings/theme.h"
#include "util/settings/shortcutmanager.h"

#include <QDebug>

namespace veles {
namespace ui {

OptionsDialog::OptionsDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::OptionsDialog) {
  ui->setupUi(this);
  ui->colorsBox->addItems(util::settings::theme::availableIds());

  connect(ui->hexColumnsAutoCheckBox, &QCheckBox::stateChanged,
          [this](int state) {
            ui->hexColumnsSpinBox->setEnabled(state != Qt::Checked);
          });

  connect(ui->keySequenceEdit, &QKeySequenceEdit::keySequenceChanged,
	  [this](const QKeySequence &keysequence){

	    // FIXME: settings.setValue() but we dont have it.
	    int index = ui->tableWidget->currentRow();
	    if (keysequence.isEmpty()) return;

	    veles::util::settings::shortcutManager
	      ->shortcutList.at(index)->setKey(keysequence);
	    ui->tableWidget->item(index, 0)
	      ->setText(keysequence.toString());
	    ui->keySequenceEdit->clear();

	  });
}

OptionsDialog::~OptionsDialog() { delete ui; }

void OptionsDialog::show() {


  // do this by hand because main window does the opposite.
  ui->tabWidget->setTabsClosable(false);
  qDebug() << ui;

  ui->tableWidget->verticalHeader()->setVisible(false);
  ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);


  int nbshortcuts = veles::util::settings::shortcutManager->shortcutList.size();

  ui->tableWidget->setRowCount(nbshortcuts);
  ui->tableWidget->setSortingEnabled(false);

  // FIXME: getting those from the manager doesn't really work.
  // it only works if the visualisation is running already.
  // we should really generate a list of available shortcuts AT COMPILE TIME.
  for (int i = 0; i < nbshortcuts; ++i) {
    QShortcut *shortcut = veles::util::settings::shortcutManager->shortcutList.at(i);
    QTableWidgetItem *item = new QTableWidgetItem(shortcut->key().toString());
    item->setTextAlignment(Qt::AlignCenter);
    ui->tableWidget->setItem(i, 0, item);
    item = new QTableWidgetItem(shortcut->whatsThis());
    ui->tableWidget->setItem(i, 1, item);
  }

  ui->colorsBox->setCurrentText(util::settings::theme::currentId());
  Qt::CheckState checkState = Qt::Unchecked;
  if (util::settings::hexedit::resizeColumnsToWindowWidth()) {
    checkState = Qt::Checked;
  }
  ui->hexColumnsAutoCheckBox->setCheckState(checkState);
  ui->hexColumnsSpinBox->setValue(util::settings::hexedit::columnsNumber());
  ui->hexColumnsSpinBox->setEnabled(checkState != Qt::Checked);
  QWidget::show();
}

void OptionsDialog::accept() {
  QString newTheme = ui->colorsBox->currentText();
  if (newTheme != util::settings::theme::currentId()) {
    veles::util::settings::theme::setCurrentId(newTheme);
    QMessageBox::about(
        this, tr("Theme change"),
        tr("To apply theme change setting please restart application"));
  }

  util::settings::hexedit::setResizeColumnsToWindowWidth(
      ui->hexColumnsAutoCheckBox->checkState() == Qt::Checked);
  util::settings::hexedit::setColumnsNumber(ui->hexColumnsSpinBox->value());

  emit accepted();
  QDialog::hide();
}

}  // namespace ui
}  // namespace veles

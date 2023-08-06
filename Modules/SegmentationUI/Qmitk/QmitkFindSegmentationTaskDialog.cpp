/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include <QmitkFindSegmentationTaskDialog.h>
#include <ui_QmitkFindSegmentationTaskDialog.h>

QmitkFindSegmentationTaskDialog::QmitkFindSegmentationTaskDialog(QWidget* parent)
  : QDialog(parent),
    m_Ui(new Ui::QmitkFindSegmentationTaskDialog)
{
  this->setWindowFlag(Qt::WindowContextHelpButtonHint, false);

  m_Ui->setupUi(this);

  using Self = QmitkFindSegmentationTaskDialog;
  connect(m_Ui->tableWidget, &QTableWidget::itemSelectionChanged, this, &Self::OnItemSelectionChanged);
  connect(m_Ui->tableWidget, &QTableWidget::itemDoubleClicked, this, &Self::OnItemDoubleClicked);
}

QmitkFindSegmentationTaskDialog::~QmitkFindSegmentationTaskDialog()
{
  delete m_Ui;
}

void QmitkFindSegmentationTaskDialog::SetTaskList(const mitk::SegmentationTaskList* taskList)
{
  if (m_TaskList != taskList)
  {
    m_TaskList = taskList;

    auto* table = m_Ui->tableWidget;
    table->setSortingEnabled(false);
    table->reset();

    if (m_TaskList.IsNotNull())
    {
      const int numberOfRows = static_cast<int>(m_TaskList->GetNumberOfTasks());
      const int numberOfColumns = table->columnCount();

      table->setRowCount(numberOfRows);

      for (int row = 0; row < numberOfRows; ++row)
      {
        for (int column = 0; column < numberOfColumns; ++column)
        {
          auto* item = new QTableWidgetItem();
          item->setData(Qt::UserRole, row);

          switch (column)
          {
          case Column::Number:
            item->setText(QString::number(row + 1));
            break;

          case Column::Name:
            if (m_TaskList->HasName(row))
              item->setText(QString::fromStdString(m_TaskList->GetName(row)));

            break;

          case Column::Status:
            item->setText(m_TaskList->IsDone(row) ? "Done" : "Not done");
            break;

          case Column::Image:
            if (m_TaskList->HasImage(row))
              item->setText(QString::fromStdString(m_TaskList->GetImage(row).string()));

            break;

          case Column::Segmentation:
            if (m_TaskList->HasSegmentation(row))
              item->setText(QString::fromStdString(m_TaskList->GetSegmentation(row).string()));

            break;

          case Column::Result:
            if (m_TaskList->HasResult(row))
              item->setText(QString::fromStdString(m_TaskList->GetResult(row).string()));

            break;

          case Column::Description:
            if (m_TaskList->HasDescription(row))
              item->setText(QString::fromStdString(m_TaskList->GetDescription(row)));

            break;
          }

          table->setItem(row, column, item);
        }
      }

      table->setSortingEnabled(true);
    }
  }
}

std::optional<size_t> QmitkFindSegmentationTaskDialog::GetSelectedTask() const
{
  return m_SelectedTask;
}

bool QmitkFindSegmentationTaskDialog::LoadSelectedTask() const
{
  return m_Ui->loadCheckBox->isChecked();
}

void QmitkFindSegmentationTaskDialog::OnItemSelectionChanged()
{
  auto selectedItems = m_Ui->tableWidget->selectedItems();

  if (!selectedItems.empty())
  {
    m_SelectedTask = static_cast<size_t>(selectedItems.front()->data(Qt::UserRole).toInt());
  }
  else
  {
    m_SelectedTask.reset();
  }
}

void QmitkFindSegmentationTaskDialog::OnItemDoubleClicked(QTableWidgetItem*)
{
  this->done(m_SelectedTask.has_value() ? QDialog::Accepted : QDialog::Rejected);
}

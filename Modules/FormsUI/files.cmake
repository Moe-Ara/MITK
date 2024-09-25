set(H_FILES
  include/mitkIQuestionWidgetFactory.h
  include/QmitkCheckboxesQuestionWidget.h
  include/QmitkDropdownQuestionWidget.h
  include/QmitkForm.h
  include/QmitkLinearScaleQuestionWidget.h
  include/QmitkMultipleChoiceQuestionWidget.h
  include/QmitkParagraphQuestionWidget.h
  include/QmitkQuestionWidget.h
  include/QmitkScreenshotQuestionWidget.h
  include/QmitkShortAnswerQuestionWidget.h
)

set(MOC_H_FILES
  include/QmitkCheckboxesQuestionWidget.h
  include/QmitkDropdownQuestionWidget.h
  include/QmitkForm.h
  include/QmitkLinearScaleQuestionWidget.h
  include/QmitkMultipleChoiceQuestionWidget.h
  include/QmitkParagraphQuestionWidget.h
  include/QmitkQuestionWidget.h
  include/QmitkScreenshotQuestionWidget.h
  include/QmitkShortAnswerQuestionWidget.h
  src/QmitkScreenshotWidget.h
)

set(UI_FILES
  src/QmitkForm.ui
  src/QmitkScreenshotWidget.ui
)

set(CPP_FILES
  mitkIQuestionWidgetFactory.cpp
  mitkModuleActivator.cpp
  mitkQuestionWidgetFactory.cpp
  QmitkCheckboxesQuestionWidget.cpp
  QmitkDropdownQuestionWidget.cpp
  QmitkForm.cpp
  QmitkLinearScaleQuestionWidget.cpp
  QmitkMultipleChoiceQuestionWidget.cpp
  QmitkParagraphQuestionWidget.cpp
  QmitkQuestionWidget.cpp
  QmitkScreenshotQuestionWidget.cpp
  QmitkScreenshotWidget.cpp
  QmitkShortAnswerQuestionWidget.cpp
)

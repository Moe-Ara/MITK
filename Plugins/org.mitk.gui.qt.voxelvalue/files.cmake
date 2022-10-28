set(INTERNAL_CPP_FILES
  mitkPluginActivator.cpp
  QmitkVoxelValueView.cpp
)

set(UI_FILES
)

set(MOC_H_FILES
  src/internal/mitkPluginActivator.h
  src/internal/QmitkVoxelValueView.h
)

set(CACHED_RESOURCE_FILES
  resources/icon.svg
  plugin.xml
)

set(QRC_FILES
)

set(CPP_FILES)

foreach(file ${SRC_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/${file})
endforeach(file ${SRC_CPP_FILES})

foreach(file ${INTERNAL_CPP_FILES})
  set(CPP_FILES ${CPP_FILES} src/internal/${file})
endforeach(file ${INTERNAL_CPP_FILES})

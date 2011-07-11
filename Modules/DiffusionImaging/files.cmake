SET(CPP_FILES 

  # Algorithms
  Algorithms/itkDiffusionQballGeneralizedFaImageFilter.h
  Algorithms/itkDiffusionQballPrepareVisualizationImageFilter.h
  Algorithms/itkTensorDerivedMeasurementsFilter.h
  Algorithms/itkBrainMaskExtractionImageFilter.h
  Algorithms/itkB0ImageExtractionImageFilter.h
  Algorithms/itkTensorImageToDiffusionImageFilter.h
  Algorithms/itkTensorToL2NormImageFilter.h
  Algorithms/itkTractsToProbabilityImageFilter.h
  
  # DicomImport
  DicomImport/mitkDicomDiffusionImageReader.cpp
  DicomImport/mitkGroupDiffusionHeadersFilter.cpp
  DicomImport/mitkDicomDiffusionImageHeaderReader.cpp
  DicomImport/mitkGEDicomDiffusionImageHeaderReader.cpp
  DicomImport/mitkPhilipsDicomDiffusionImageHeaderReader.cpp
  DicomImport/mitkSiemensDicomDiffusionImageHeaderReader.cpp
  DicomImport/mitkSiemensMosaicDicomDiffusionImageHeaderReader.cpp

  # DataStructures
  IODataStructures/mitkDiffusionImagingObjectFactory.cpp

  # DataStructures -> DWI
  IODataStructures/DiffusionWeightedImages/mitkDiffusionImageHeaderInformation.cpp
  IODataStructures/DiffusionWeightedImages/mitkDiffusionImageSource.cpp
  IODataStructures/DiffusionWeightedImages/mitkNrrdDiffusionImageReader.cpp
  IODataStructures/DiffusionWeightedImages/mitkNrrdDiffusionImageWriter.cpp
  IODataStructures/DiffusionWeightedImages/mitkNrrdDiffusionImageIOFactory.cpp
  IODataStructures/DiffusionWeightedImages/mitkNrrdDiffusionImageWriterFactory.cpp
  IODataStructures/DiffusionWeightedImages/mitkDiffusionImageSerializer.cpp

  # DataStructures -> QBall
  IODataStructures/QBallImages/mitkQBallImageSource.cpp
  IODataStructures/QBallImages/mitkNrrdQBallImageReader.cpp
  IODataStructures/QBallImages/mitkNrrdQBallImageWriter.cpp
  IODataStructures/QBallImages/mitkNrrdQBallImageIOFactory.cpp
  IODataStructures/QBallImages/mitkNrrdQBallImageWriterFactory.cpp
  IODataStructures/QBallImages/mitkQBallImage.cpp
  IODataStructures/QBallImages/mitkQBallImageSerializer.cpp

  # DataStructures -> Tensor
  IODataStructures/TensorImages/mitkTensorImageSource.cpp
  IODataStructures/TensorImages/mitkNrrdTensorImageReader.cpp
  IODataStructures/TensorImages/mitkNrrdTensorImageWriter.cpp
  IODataStructures/TensorImages/mitkNrrdTensorImageIOFactory.cpp
  IODataStructures/TensorImages/mitkNrrdTensorImageWriterFactory.cpp
  IODataStructures/TensorImages/mitkTensorImage.cpp
  IODataStructures/TensorImages/mitkTensorImageSerializer.cpp

  # DataStructures -> FiberBundle
  IODataStructures/FiberBundle/mitkFiberBundle.cpp
  IODataStructures/FiberBundle/mitkFiberBundleWriter.cpp
  IODataStructures/FiberBundle/mitkFiberBundleReader.cpp
  IODataStructures/FiberBundle/mitkFiberBundleIOFactory.cpp
  IODataStructures/FiberBundle/mitkFiberBundleWriterFactory.cpp
  IODataStructures/FiberBundle/mitkFiberBundleSerializer.cpp

  # DataStructures -> PlanarFigureComposite
  IODataStructures/PlanarFigureComposite/mitkPlanarFigureComposite.cpp

  # Rendering
  Rendering/vtkMaskedProgrammableGlyphFilter.cpp
  Rendering/mitkCompositeMapper.cpp
  Rendering/mitkVectorImageVtkGlyphMapper3D.cpp
  Rendering/vtkOdfSource.cxx
  Rendering/vtkThickPlane.cxx
  Rendering/mitkOdfNormalizationMethodProperty.cpp
  Rendering/mitkOdfScaleByProperty.cpp
  Rendering/mitkFiberBundleMapper3D.cpp
  
  # Interactions
  Interactions/mitkFiberBundleInteractor.cpp
)

SET(H_FILES 
  Rendering/mitkDiffusionImageMapper.h
  Reconstruction/itkDiffusionQballReconstructionImageFilter.h
  Reconstruction/mitkTeemDiffusionTensor3DReconstructionImageFilter.h
  Reconstruction/itkAnalyticalDiffusionQballReconstructionImageFilter.h
  Reconstruction/itkPointShell.h
  Reconstruction/itkOrientationDistributionFunction.h
  IODataStructures/DiffusionWeightedImages/mitkDiffusionImage.h
  IODataStructures/FiberBundle/itkSlowPolyLineParametricPath.h 
  Rendering/mitkOdfVtkMapper2D.h
)

SET( TOOL_FILES
)

IF(WIN32)
ENDIF(WIN32)

#MITK_MULTIPLEX_PICTYPE( Algorithms/mitkImageRegistrationMethod-TYPE.cpp ) 

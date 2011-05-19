/*=========================================================================
Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

//TODO remove these if possible
#include <mitkSurface.h>

//MITK
#include <mitkAbstractTransformGeometry.h>
#include <mitkDataNode.h>
#include <mitkDataNodeFactory.h>
#include <mitkImageSliceSelector.h>
#include <mitkLevelWindowProperty.h>
#include <mitkLookupTableProperty.h>
#include <mitkPlaneGeometry.h>
#include <mitkProperties.h>
#include <mitkResliceMethodProperty.h>
#include <mitkTimeSlicedGeometry.h>
#include <mitkVolumeCalculator.h>
#include <mitkVtkResliceInterpolationProperty.h>

//MITK Rendering
#include "mitkImageVtkMapper2D.h"
#include "mitkVtkPropRenderer.h"
#include "vtkMitkThickSlicesFilter.h"

//VTK
#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkGeneralTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkLookupTable.h>
#include <vtkImageData.h>
#include <vtkPoints.h>
#include <vtkFloatArray.h>
#include <vtkLinearTransform.h>
#include <vtkImageReslice.h>
#include <vtkImageChangeInformation.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkTexture.h>
#include <vtkCamera.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkImageMapToColors.h>

//ITK
#include <itkRGBAPixel.h>

int mitk::ImageVtkMapper2D::numRenderer = 0; //Number of renderers data is stored for.

mitk::ImageVtkMapper2D::ImageVtkMapper2D()
{
  m_VtkBased = true;
}

mitk::ImageVtkMapper2D::~ImageVtkMapper2D()
{
  this->Clear();
  this->InvokeEvent( itk::DeleteEvent() ); //TODO <- what is this doing exactly?
}

void mitk::ImageVtkMapper2D::AdjustCamera(mitk::BaseRenderer* renderer)
{
  LocalStorage *localStorage = m_LSH.GetLocalStorage(renderer);

  //activate parallel projection for 2D
  renderer->GetVtkRenderer()->GetActiveCamera()->SetParallelProjection(true);

  const mitk::DisplayGeometry* displayGeometry = renderer->GetDisplayGeometry();

  double imageHeightInMM = localStorage->m_ReslicedImage->GetDimensions()[1]; //the height of the current slice in mm
  double displayHeightInMM = displayGeometry->GetSizeInMM()[1]; //the display height in mm (gets smaller when you zoom in)
  //  double zoomFactor = displayHeightInMM/imageHeightInMM; //determine how much of the image can be displayed
  double zoomFactor = imageHeightInMM/displayHeightInMM; //determine how much of the image can be displayed

  Vector2D displayGeometryOriginInMM = displayGeometry->GetOriginInMM();  //top left of the render window
  Vector2D displayGeometryCenterInMM = displayGeometryOriginInMM + displayGeometry->GetSizeInMM()/2; //center of the render window

  //Scale the rendered object:
  //The image is scaled by a single factor, because in an orthographic projection sizes
  //are preserved (so you cannot scale X and Y axis with different parameters). The
  //parameter sets the size of the total display-volume. If you set this to the image
  //height, the image plus a border with the size of the image will be rendered.
  //Therefore, the size is imageHeightInMM / 2.
  renderer->GetVtkRenderer()->GetActiveCamera()->SetParallelScale(imageHeightInMM / 2);
  //zooming with the factor calculated by dividing displayHeight through imegeHeight. The factor is inverse, because the VTK zoom method is working inversely.
  renderer->GetVtkRenderer()->GetActiveCamera()->Zoom(zoomFactor);

  //the center of the view-plane
  double viewPlaneCenter[3];
  viewPlaneCenter[0] = displayGeometryCenterInMM[0];
  viewPlaneCenter[1] = displayGeometryCenterInMM[1];
  viewPlaneCenter[2] = 0.0; //the view-plane is located in the XY-plane with Z=0.0

  //define which direction is "up" for the ciamera (like default for vtk (0.0, 1.0, 0.0)
  double cameraUp[3];
  cameraUp[0] = 0.0;
  cameraUp[1] = 1.0;
  cameraUp[2] = 0.0;

  //the position of the camera (center[0], center[1], 1000)
  double cameraPosition[3];
  cameraPosition[0] = viewPlaneCenter[0];
  cameraPosition[1] = viewPlaneCenter[1];
  cameraPosition[2] = viewPlaneCenter[2] + 1000.0; //Reason for 1000 => VTK seems to calculate the clipping planes wrong for Z=1

  //set the camera corresponding to the textured plane
  vtkSmartPointer<vtkCamera> camera = renderer->GetVtkRenderer()->GetActiveCamera();
  if (camera)
  {
    camera->SetPosition( cameraPosition ); //set the camera position on the textured plane normal (in our case this is the view plane normal)
    camera->SetFocalPoint( viewPlaneCenter ); //set the focal point to the center of the textured plane
    camera->SetViewUp( cameraUp ); //set the view-up for the camera
  }
  //reset the clipping range TODO why? really needed if everything is correct?
  renderer->GetVtkRenderer()->ResetCameraClippingRange();
}

//set the two points defining the textured plane according to the dimension and spacing
void mitk::ImageVtkMapper2D::GeneratePlane(mitk::BaseRenderer* renderer, mitk::ScalarType spacing[2])
{
  LocalStorage *localStorage = m_LSH.GetLocalStorage(renderer);
  //set the origin to (0; 0; 0), because the default origin can be anywhere
  localStorage->m_Plane->SetOrigin(0.0, 0.0, 0.0);
  //These two points define the axes of the plane in combination with the origin (0/0/0).
  //Point 1 is the x-axis and point 2 the y-axis.
  //Each plane is transformed according to the view (transversal, coronal and saggital) afterwards.
  localStorage->m_Plane->SetPoint1((localStorage->m_ReslicedImage->GetDimensions()[0])*spacing[0], 0.0, 0.0);
  localStorage->m_Plane->SetPoint2(0.0, (localStorage->m_ReslicedImage->GetDimensions()[1])*spacing[1], 0.0);
}

const mitk::Image* mitk::ImageVtkMapper2D::GetInput( void )
{
  return static_cast< const mitk::Image * >( this->GetData() );
}

vtkProp* mitk::ImageVtkMapper2D::GetVtkProp(mitk::BaseRenderer* renderer)
{
  //return the actor corresponding to the renderer
  return m_LSH.GetLocalStorage(renderer)->m_Actor;
}

void mitk::ImageVtkMapper2D::MitkRenderOverlay(BaseRenderer* renderer)
{
  if ( this->IsVisible(renderer)==false )
    return;

  if ( this->GetVtkProp(renderer)->GetVisibility() )
  {
    this->GetVtkProp(renderer)->RenderOverlay(renderer->GetVtkRenderer());
  }
}

void mitk::ImageVtkMapper2D::MitkRenderOpaqueGeometry(BaseRenderer* renderer)
{
  if ( this->IsVisible( renderer )==false )
    return;

  if ( this->GetVtkProp(renderer)->GetVisibility() )
  {
    this->GetVtkProp(renderer)->RenderOpaqueGeometry( renderer->GetVtkRenderer() );
  }
}

void mitk::ImageVtkMapper2D::MitkRenderTranslucentGeometry(BaseRenderer* renderer)
{
  if ( this->IsVisible(renderer)==false )
    return;

  //TODO is it possible to have a visible BaseRenderer AND an invisible VtkRenderer???
  if ( this->GetVtkProp(renderer)->GetVisibility() ) {
    this->GetVtkProp(renderer)->RenderTranslucentPolygonalGeometry(renderer->GetVtkRenderer());
  }
}

void mitk::ImageVtkMapper2D::MitkRenderVolumetricGeometry(BaseRenderer* renderer)
{
  if(IsVisible(renderer)==false)
    return;

  //TODO is it possible to have a visible BaseRenderer AND an invisible VtkRenderer???
  if ( GetVtkProp(renderer)->GetVisibility() )
    this->GetVtkProp(renderer)->RenderVolumetricGeometry(renderer->GetVtkRenderer());
}

void mitk::ImageVtkMapper2D::GenerateData( mitk::BaseRenderer *renderer )
{
  LocalStorage *localStorage = m_LSH.GetLocalStorage(renderer);

  bool visible = IsVisible(renderer);

  if(visible==false)
  {
    localStorage->m_Actor->VisibilityOff();
    return;
  }

  mitk::Image *input = const_cast< mitk::Image * >( this->GetInput() );
  input->Update();

  if ( input == NULL )
  {
    return;
  }

  RendererInfo &rendererInfo = this->AccessRendererInfo( renderer );
  rendererInfo.Squeeze();

  //TODO ApplyProperties is called here? Control flow correct?
  //  this->ApplyProperties( renderer );

  const Geometry2D *worldGeometry = renderer->GetCurrentWorldGeometry2D();

  if( ( worldGeometry == NULL ) || ( !worldGeometry->IsValid() ) || ( !worldGeometry->GetReferenceGeometry() ))
  {
    return;
  }

  // check if there is something to display.
  if ( ! input->IsVolumeSet( this->GetTimestep() ) ) return;

  Image::RegionType requestedRegion = input->GetLargestPossibleRegion();
  requestedRegion.SetIndex( 3, this->GetTimestep() );
  requestedRegion.SetSize( 3, 1 );
  requestedRegion.SetSize( 4, 1 );
  input->SetRequestedRegion( &requestedRegion );
  input->Update();

  vtkImageData* inputData = input->GetVtkImageData( this->GetTimestep() );

  if ( inputData == NULL )
  {
    return;
  }

  vtkFloatingPointType spacing[3];
  inputData->GetSpacing( spacing );

  // how big the area is in physical coordinates: widthInMM x heightInMM pixels
  mitk::ScalarType widthInMM, heightInMM;

  // where we want to sample
  Point3D origin;
  Vector3D right, bottom, normal;
  Vector3D rightInIndex, bottomInIndex;

  // take transform of input image into account
  const TimeSlicedGeometry *inputTimeGeometry = input->GetTimeSlicedGeometry();
  const Geometry3D* inputGeometry = inputTimeGeometry->GetGeometry3D( this->GetTimestep() );

  //Spacing of the slice
  ScalarType mmPerPixel[2];

  // Bounds information for reslicing (only reuqired if reference geometry 
  // is present)
  vtkFloatingPointType bounds[6];
  bool boundsInitialized = false;

  for ( int i = 0; i < 6; ++i )
  {
    bounds[i] = 0.0;
  }

  // Do we have a simple PlaneGeometry?
  if ( dynamic_cast< const PlaneGeometry * >( worldGeometry ) != NULL )
  {
    const PlaneGeometry *planeGeometry =
        static_cast< const PlaneGeometry * >( worldGeometry );

    origin = planeGeometry->GetOrigin();
    right  = planeGeometry->GetAxisVector( 0 );
    bottom = planeGeometry->GetAxisVector( 1 );
    normal = planeGeometry->GetNormal();

    bool inPlaneResampleExtentByGeometry = false;
    GetDataNode()->GetBoolProperty(
        "in plane resample extent by geometry",
        inPlaneResampleExtentByGeometry, renderer
        );

    if ( inPlaneResampleExtentByGeometry )
    {
      // Resampling grid corresponds to the current world geometry. This
      // means that the spacing of the output 2D image depends on the
      // currently selected world geometry, and *not* on the image itself.
      rendererInfo.m_Extent[0] = worldGeometry->GetExtent( 0 );
      rendererInfo.m_Extent[1] = worldGeometry->GetExtent( 1 );
    }
    else
    {
      // Resampling grid corresponds to the input geometry. This means that
      // the spacing of the output 2D image is directly derived from the
      // associated input image, regardless of the currently selected world
      // geometry.
      inputGeometry->WorldToIndex( origin, right, rightInIndex );
      inputGeometry->WorldToIndex( origin, bottom, bottomInIndex );
      rendererInfo.m_Extent[0] = rightInIndex.GetNorm();
      rendererInfo.m_Extent[1] = bottomInIndex.GetNorm();
    }
    
    // Get the extent of the current world geometry and calculate resampling
    // spacing therefrom.
    widthInMM = worldGeometry->GetExtentInMM( 0 );
    heightInMM = worldGeometry->GetExtentInMM( 1 );

    mmPerPixel[0] = widthInMM / rendererInfo.m_Extent[0];
    mmPerPixel[1] = heightInMM / rendererInfo.m_Extent[1];

    right.Normalize();
    bottom.Normalize();
    normal.Normalize();

    origin += right * ( mmPerPixel[0] * 0.5 );
    origin += bottom * ( mmPerPixel[1] * 0.5 );

    widthInMM -= mmPerPixel[0];
    heightInMM -= mmPerPixel[1];

    // Use inverse transform of the input geometry for reslicing the 3D image
    rendererInfo.m_Reslicer->SetResliceTransform(
        inputGeometry->GetVtkTransform()->GetLinearInverse() );

    // Set background level to TRANSLUCENT (see Geometry2DDataVtkMapper3D)
    rendererInfo.m_Reslicer->SetBackgroundLevel( -32768 );

    // If a reference geometry does exist (as would usually be the case for
    // PlaneGeometry), store it in rendererInfo so that we have access to it
    // in Paint.
    //
    // Note: this is currently not strictly required, but could facilitate
    // correct plane clipping.
    if ( worldGeometry->GetReferenceGeometry() )
    {
      rendererInfo.m_ReferenceGeometry = worldGeometry->GetReferenceGeometry();

      // Calculate the actual bounds of the transformed plane clipped by the
      // dataset bounding box; this is required for drawing the texture at the
      // correct position during 3D mapping.
      boundsInitialized = this->CalculateClippedPlaneBounds(
          rendererInfo.m_ReferenceGeometry, planeGeometry, bounds );
      MITK_INFO << "boundsinitialized " << boundsInitialized;
    }
  }
  // Do we have an AbstractTransformGeometry?
  else if ( dynamic_cast< const AbstractTransformGeometry * >( worldGeometry ) )
  {
    const mitk::AbstractTransformGeometry* abstractGeometry =
        dynamic_cast< const AbstractTransformGeometry * >(worldGeometry);

    rendererInfo.m_Extent[0] = abstractGeometry->GetParametricExtent(0);
    rendererInfo.m_Extent[1] = abstractGeometry->GetParametricExtent(1);

    widthInMM = abstractGeometry->GetParametricExtentInMM(0);
    heightInMM = abstractGeometry->GetParametricExtentInMM(1);

    mmPerPixel[0] = widthInMM / rendererInfo.m_Extent[0];
    mmPerPixel[1] = heightInMM / rendererInfo.m_Extent[1];

    origin = abstractGeometry->GetPlane()->GetOrigin();

    right = abstractGeometry->GetPlane()->GetAxisVector(0);
    right.Normalize();

    bottom = abstractGeometry->GetPlane()->GetAxisVector(1);
    bottom.Normalize();

    normal = abstractGeometry->GetPlane()->GetNormal();
    normal.Normalize();

    // Use a combination of the InputGeometry *and* the possible non-rigid
    // AbstractTransformGeometry for reslicing the 3D Image
    vtkGeneralTransform *composedResliceTransform = vtkGeneralTransform::New();
    composedResliceTransform->Identity();
    composedResliceTransform->Concatenate(
        inputGeometry->GetVtkTransform()->GetLinearInverse() );
    composedResliceTransform->Concatenate(
        abstractGeometry->GetVtkAbstractTransform()
        );

    rendererInfo.m_Reslicer->SetResliceTransform( composedResliceTransform );
    composedResliceTransform->UnRegister( NULL ); // decrease RC

    // Set background level to BLACK instead of translucent, to avoid
    // boundary artifacts (see Geometry2DDataVtkMapper3D)
    rendererInfo.m_Reslicer->SetBackgroundLevel( -1023 );
  }
  else
  {
    return;
  }

  // Make sure that the image to display has a certain minimum size.
  if ( (rendererInfo.m_Extent[0] <= 2) && (rendererInfo.m_Extent[1] <= 2) )
  {
    return;
  }

  // Initialize the interpolation mode for resampling; switch to nearest
  // neighbor if the input image is too small.
  if ( (input->GetDimension() >= 3) && (input->GetDimension(2) > 1) )
  {
    VtkResliceInterpolationProperty *resliceInterpolationProperty;
    this->GetDataNode()->GetProperty(
        resliceInterpolationProperty, "reslice interpolation" );

    int interpolationMode = VTK_RESLICE_NEAREST;
    if ( resliceInterpolationProperty != NULL )
    {
      interpolationMode = resliceInterpolationProperty->GetInterpolation();
    }

    switch ( interpolationMode )
    {
    case VTK_RESLICE_NEAREST:
      rendererInfo.m_Reslicer->SetInterpolationModeToNearestNeighbor();
      break;

    case VTK_RESLICE_LINEAR:
      rendererInfo.m_Reslicer->SetInterpolationModeToLinear();
      break;

    case VTK_RESLICE_CUBIC:
      rendererInfo.m_Reslicer->SetInterpolationModeToCubic();
      break;
    }
  }
  else
  {
    rendererInfo.m_Reslicer->SetInterpolationModeToNearestNeighbor();
  }

  int thickSlicesMode = 0;
  
  int thickSlicesNum = 1;
  
  // Thick slices parameters
  if( inputData->GetNumberOfScalarComponents() == 1 ) // for now only single component are allowed
  {
    DataNode *dn=renderer->GetCurrentWorldGeometry2DNode();
    if(dn)
    {
      ResliceMethodProperty *resliceMethodEnumProperty=0;
      
      if( dn->GetProperty( resliceMethodEnumProperty, "reslice.thickslices" ) && resliceMethodEnumProperty )
        thickSlicesMode = resliceMethodEnumProperty->GetValueAsId(); 

      IntProperty *intProperty=0;
      if( dn->GetProperty( intProperty, "reslice.thickslices.num" ) && intProperty )
      {
        thickSlicesNum = intProperty->GetValue(); 
        if(thickSlicesNum < 1) thickSlicesNum=1;
        if(thickSlicesNum > 10) thickSlicesNum=10;
      }
    }
    else
    {
      MITK_WARN << "no associated widget plane data tree node found";
    }
  }
  
  rendererInfo.m_UnitSpacingImageFilter->SetInput( inputData );
  rendererInfo.m_Reslicer->SetInput( rendererInfo.m_UnitSpacingImageFilter->GetOutput() );
  
  rendererInfo.m_PixelsPerMM[0] = 1.0 / mmPerPixel[0];
  rendererInfo.m_PixelsPerMM[1] = 1.0 / mmPerPixel[1];

  //calulate the originArray and the orientations for the reslice-filter
  double originArray[3];
  itk2vtk( origin, originArray );

  rendererInfo.m_Reslicer->SetResliceAxesOrigin( originArray );

  double cosines[9];

  // direction of the X-axis of the sampled result
  vnl2vtk( right.Get_vnl_vector(), cosines );

  // direction of the Y-axis of the sampled result
  vnl2vtk( bottom.Get_vnl_vector(), cosines + 3 );

  // normal of the plane
  vnl2vtk( normal.Get_vnl_vector(), cosines + 6 );

  rendererInfo.m_Reslicer->SetResliceAxesDirectionCosines( cosines );



  int xMin, xMax, yMin, yMax;
  if ( boundsInitialized )
  {
    // Calculate output extent (integer values)
    xMin = static_cast< int >( bounds[0] / mmPerPixel[0] + 0.5 );
    xMax = static_cast< int >( bounds[1] / mmPerPixel[0] + 0.5 );
    yMin = static_cast< int >( bounds[2] / mmPerPixel[1] + 0.5 );
    yMax = static_cast< int >( bounds[3] / mmPerPixel[1] + 0.5 );

    // Calculate the extent by which the maximal plane (due to plane rotation)
    // overlaps the regular plane size.
    rendererInfo.m_Overlap[0] = -xMin;
    rendererInfo.m_Overlap[1] = -yMin;
  }
  else
  {
    // If no reference geometry is available, we also don't know about the
    // maximum plane size; so the overlap is just ignored
    rendererInfo.m_Overlap.Fill( 0.0 );

    xMin = yMin = 0;
    xMax = static_cast< int >( rendererInfo.m_Extent[0]
                               - rendererInfo.m_PixelsPerMM[0] + 0.5);
    yMax = static_cast< int >( rendererInfo.m_Extent[1] 
                               - rendererInfo.m_PixelsPerMM[1] + 0.5);
  }

  // Disallow huge dimensions
  if ( (xMax-xMin) * (yMax-yMin) > 4096*4096 )
  {
    return;
  }

  // Calculate dataset spacing in plane z direction (NOT spacing of current
  // world geometry)
  double dataZSpacing = 1.0;
  
  normal.Normalize();    
  Vector3D normInIndex;
  inputGeometry->WorldToIndex( origin, normal, normInIndex );
  
  if(thickSlicesMode > 0)
  {
    dataZSpacing = 1.0 / normInIndex.GetNorm();
    rendererInfo.m_Reslicer->SetOutputDimensionality( 3 );
    rendererInfo.m_Reslicer->SetOutputExtent( xMin, xMax-1, yMin, yMax-1, -thickSlicesNum, 0+thickSlicesNum );
  }  
  else
  {
    rendererInfo.m_Reslicer->SetOutputDimensionality( 2 );
    rendererInfo.m_Reslicer->SetOutputExtent( xMin, xMax-1, yMin, yMax-1, 0, 0 );
  }

  rendererInfo.m_Reslicer->SetOutputOrigin( 0.0, 0.0, 0.0 );
  rendererInfo.m_Reslicer->SetOutputSpacing( mmPerPixel[0], mmPerPixel[1], dataZSpacing );

  // xMax and yMax are meant exclusive until now, whereas
  // SetOutputExtent wants an inclusive bound. Thus, we need 
  // to subtract 1.

  // Do the reslicing. Modified() is called to make sure that the reslicer is
  // executed even though the input geometry information did not change; this
  // is necessary when the input /em data, but not the /em geometry changes.

  // The reslicing result is used both for 2D and for 3D mapping. 2D mapping
  // currently uses PIC data structures, while 3D mapping uses VTK data. Thus,
  // the reslicing result needs to be stored twice.

  // 1. Check the result
  vtkImageData* reslicedImage = 0;
  
  if(thickSlicesMode>0)
  {
    rendererInfo.m_TSFilter->SetThickSliceMode( thickSlicesMode-1 );
    rendererInfo.m_TSFilter->SetInput( rendererInfo.m_Reslicer->GetOutput() );
    rendererInfo.m_TSFilter->Modified();
    rendererInfo.m_TSFilter->Update();
    reslicedImage = rendererInfo.m_TSFilter->GetOutput();
  }
  else
  {
    rendererInfo.m_Reslicer->Modified();
    rendererInfo.m_Reslicer->Update();
    reslicedImage = rendererInfo.m_Reslicer->GetOutput();
  }  

  if((reslicedImage == NULL) || (reslicedImage->GetDataDimension() < 1))
  {
    MITK_WARN << "reslicer returned empty image";
    return;
  }

  bool imageIs2D = true;

  // 3. Store the result in a VTK image
  if ( imageIs2D )
  {
    if ( rendererInfo.m_Image == NULL )
    {
      rendererInfo.m_Image = vtkImageData::New();
    }
    rendererInfo.m_Image->DeepCopy( reslicedImage );
    rendererInfo.m_Image->Update();
  }
  else
  {
    if ( rendererInfo.m_Image != NULL )
    {
      rendererInfo.m_Image->Delete();
    }
    rendererInfo.m_Image = NULL;
  }

  //TODO how does the reslicer know for which render window it is reslicing for?
  //set the current slice for the localStorage
  localStorage->m_ReslicedImage = reslicedImage;

  //set the current slice as texture for the plane
  localStorage->m_Texture->SetInput(localStorage->m_ReslicedImage);

  //apply the properties after the slice was set
  this->ApplyProperties( renderer );
  //set the size textured plane
  this->GeneratePlane( renderer, mmPerPixel );

  //turn the light out in the scene in order to render correct grey values. TODO How to turn it on if you need it?
  renderer->GetVtkRenderer()->RemoveAllLights();

  //remove the VTK interaction
  renderer->GetVtkRenderer()->GetRenderWindow()->SetInteractor(NULL);

  //get the transformation matrix of the reslicer in order to render the slice as transversal, coronal or saggital
  vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
  vtkSmartPointer<vtkMatrix4x4> matrix = rendererInfo.m_Reslicer->GetResliceAxes();

  trans->SetMatrix(matrix);
  localStorage->m_TransformMatrix = matrix;

  //transform the plane to the corresponding view (transversal, coronal or saggital)
  localStorage->m_TransformFilter->SetTransform(trans);
  localStorage->m_TransformFilter->SetInputConnection(localStorage->m_Plane->GetOutputPort());
  localStorage->m_TransformFilter->Update();
  localStorage->m_Mapper->SetInputConnection(localStorage->m_TransformFilter->GetOutputPort());

  //set up the camera to view the transformed plane
  this->AdjustCamera( renderer );

  //TODO remove this later
  static int counter = 0;
  if(counter < 3)
  {
    mitk::Surface::Pointer surf = mitk::Surface::New();
    surf->SetVtkPolyData(localStorage->m_Mapper->GetInput());
    surf->Update();

    mitk::DataNode::Pointer node = mitk::DataNode::New();
    node->SetData(surf);
    renderer->GetDataStorage()->Add(node);
    counter++;
  }
  //TODO remove this later
  renderer->GetVtkRenderer()->SetBackground(1, 1, 1);

  //Transform the camera to the current position (transveral, coronal and saggital plane).
  //This is necessary, because the vtkTransformFilter does not manipulate the vtkCamera.
  //(Without not all three planes would be visible).
  renderer->GetVtkRenderer()->GetActiveCamera()->ApplyTransform(trans);

  // We have been modified
  rendererInfo.m_LastUpdateTime.Modified();
}

bool mitk::ImageVtkMapper2D::LineIntersectZero( vtkPoints *points, int p1, int p2,
                                                vtkFloatingPointType *bounds )
{
  vtkFloatingPointType point1[3];
  vtkFloatingPointType point2[3];
  points->GetPoint( p1, point1 );
  points->GetPoint( p2, point2 );

  if ( (point1[2] * point2[2] <= 0.0) && (point1[2] != point2[2]) )
  {
    double x, y;
    x = ( point1[0] * point2[2] - point1[2] * point2[0] ) / ( point2[2] - point1[2] );
    y = ( point1[1] * point2[2] - point1[2] * point2[1] ) / ( point2[2] - point1[2] );

    if ( x < bounds[0] ) { bounds[0] = x; }
    if ( x > bounds[1] ) { bounds[1] = x; }
    if ( y < bounds[2] ) { bounds[2] = y; }
    if ( y > bounds[3] ) { bounds[3] = y; }
    bounds[4] = bounds[5] = 0.0;
    return true;
  }
  return false;
}

bool mitk::ImageVtkMapper2D::CalculateClippedPlaneBounds( const Geometry3D *boundingGeometry,
                                                          const PlaneGeometry *planeGeometry, vtkFloatingPointType *bounds )
{
  // Clip the plane with the bounding geometry. To do so, the corner points 
  // of the bounding box are transformed by the inverse transformation 
  // matrix, and the transformed bounding box edges derived therefrom are 
  // clipped with the plane z=0. The resulting min/max values are taken as 
  // bounds for the image reslicer.
  const mitk::BoundingBox *boundingBox = boundingGeometry->GetBoundingBox();

  mitk::BoundingBox::PointType bbMin = boundingBox->GetMinimum();
  mitk::BoundingBox::PointType bbMax = boundingBox->GetMaximum();
  mitk::BoundingBox::PointType bbCenter = boundingBox->GetCenter();

  vtkPoints *points = vtkPoints::New();
  if(boundingGeometry->GetImageGeometry())
  {
    points->InsertPoint( 0, bbMin[0]-0.5, bbMin[1]-0.5, bbMin[2]-0.5 );
    points->InsertPoint( 1, bbMin[0]-0.5, bbMin[1]-0.5, bbMax[2]-0.5 );
    points->InsertPoint( 2, bbMin[0]-0.5, bbMax[1]-0.5, bbMax[2]-0.5 );
    points->InsertPoint( 3, bbMin[0]-0.5, bbMax[1]-0.5, bbMin[2]-0.5 );
    points->InsertPoint( 4, bbMax[0]-0.5, bbMin[1]-0.5, bbMin[2]-0.5 );
    points->InsertPoint( 5, bbMax[0]-0.5, bbMin[1]-0.5, bbMax[2]-0.5 );
    points->InsertPoint( 6, bbMax[0]-0.5, bbMax[1]-0.5, bbMax[2]-0.5 );
    points->InsertPoint( 7, bbMax[0]-0.5, bbMax[1]-0.5, bbMin[2]-0.5 );
  }
  else
  {
    points->InsertPoint( 0, bbMin[0], bbMin[1], bbMin[2] );
    points->InsertPoint( 1, bbMin[0], bbMin[1], bbMax[2] );
    points->InsertPoint( 2, bbMin[0], bbMax[1], bbMax[2] );
    points->InsertPoint( 3, bbMin[0], bbMax[1], bbMin[2] );
    points->InsertPoint( 4, bbMax[0], bbMin[1], bbMin[2] );
    points->InsertPoint( 5, bbMax[0], bbMin[1], bbMax[2] );
    points->InsertPoint( 6, bbMax[0], bbMax[1], bbMax[2] );
    points->InsertPoint( 7, bbMax[0], bbMax[1], bbMin[2] );
  }

  vtkPoints *newPoints = vtkPoints::New();

  vtkTransform *transform = vtkTransform::New();
  transform->Identity();
  transform->Concatenate(
      planeGeometry->GetVtkTransform()->GetLinearInverse()
      );

  transform->Concatenate( boundingGeometry->GetVtkTransform() );

  transform->TransformPoints( points, newPoints );

  bounds[0] = bounds[2] = 10000000.0;
  bounds[1] = bounds[3] = -10000000.0;
  bounds[4] = bounds[5] = 0.0;

  this->LineIntersectZero( newPoints, 0, 1, bounds );
  this->LineIntersectZero( newPoints, 1, 2, bounds );
  this->LineIntersectZero( newPoints, 2, 3, bounds );
  this->LineIntersectZero( newPoints, 3, 0, bounds );
  this->LineIntersectZero( newPoints, 0, 4, bounds );
  this->LineIntersectZero( newPoints, 1, 5, bounds );
  this->LineIntersectZero( newPoints, 2, 6, bounds );
  this->LineIntersectZero( newPoints, 3, 7, bounds );
  this->LineIntersectZero( newPoints, 4, 5, bounds );
  this->LineIntersectZero( newPoints, 5, 6, bounds );
  this->LineIntersectZero( newPoints, 6, 7, bounds );
  this->LineIntersectZero( newPoints, 7, 4, bounds );

  // clean up vtk data
  points->Delete();
  newPoints->Delete();
  transform->Delete();

  if ( (bounds[0] > 9999999.0) || (bounds[2] > 9999999.0)
    || (bounds[1] < -9999999.0) || (bounds[3] < -9999999.0) )
    {
    return false;
  }
  else
  {
    // The resulting bounds must be adjusted by the plane spacing, since we
    // we have so far dealt with index coordinates
    const float *planeSpacing = planeGeometry->GetFloatSpacing();
    bounds[0] *= planeSpacing[0];
    bounds[1] *= planeSpacing[0];
    bounds[2] *= planeSpacing[1];
    bounds[3] *= planeSpacing[1];
    bounds[4] *= planeSpacing[2];
    bounds[5] *= planeSpacing[2];
    return true;
  }
}

void mitk::ImageVtkMapper2D::GenerateAllData()
{
  RendererInfoMap::iterator it, end = m_RendererInfo.end();

  for ( it = m_RendererInfo.begin(); it != end; ++it)
  {
    this->Update( it->first );
  }
}

void mitk::ImageVtkMapper2D::Clear()
{
  RendererInfoMap::iterator it, end = m_RendererInfo.end();
  for ( it = m_RendererInfo.begin(); it != end; ++it )
  {
    it->second.RemoveObserver();
    it->second.Squeeze();
  }
  m_RendererInfo.clear();
}

void mitk::ImageVtkMapper2D::ApplyProperties(mitk::BaseRenderer* renderer)
{
  //get the current localStorage for the corresponding renderer
  LocalStorage *localStorage = m_LSH.GetLocalStorage(renderer);

  RendererInfo &rendererInfo = this->AccessRendererInfo( renderer );

  // check for interpolation properties
  bool textureInterpolation = false;
  GetDataNode()->GetBoolProperty( "texture interpolation", textureInterpolation, renderer );
  rendererInfo.m_TextureInterpolation = textureInterpolation;
  if(textureInterpolation)
  { //texture interpolation on
    localStorage->m_Texture->InterpolateOn();
  }
  else
  { //texture interpolation off
    localStorage->m_Texture->InterpolateOff();
  }
  //do not repeat the texture (the image)
  localStorage->m_Texture->RepeatOff();

//  //get the opacity and set it for the actor
//  float opacity = 0;
//  GetOpacity(opacity, renderer);
//  localStorage->m_Actor->GetProperty()->SetOpacity(opacity);

//  //get the color and set it for the actor
//  float rgb[3] = { 1.0f, 1.0f, 1.0f };
//  GetColor( rgb, renderer );
//  double rgbConv[3] = {(double)rgb[0], (double)rgb[1], (double)rgb[2]};
//  localStorage->m_Actor->GetProperty()->SetColor(rgbConv);

  //    bool binary = false;
  //    this->GetDataNode()->GetBoolProperty( "binary", binary, renderer );
  //  if( binary )
  //  {


  float rgba[4]= { 1.0f, 1.0f, 1.0f, 1.0f };
  float opacity = 1.0f;

  // check for color prop and use it for rendering if it exists
  // binary image hovering & binary image selection
  bool hover    = false;
  bool selected = false;
  GetDataNode()->GetBoolProperty("binaryimage.ishovering", hover, renderer);
  GetDataNode()->GetBoolProperty("selected", selected, renderer);
  if(hover && !selected)
  {
    mitk::ColorProperty::Pointer colorprop = dynamic_cast<mitk::ColorProperty*>(GetDataNode()->GetProperty
                                                                                ("binaryimage.hoveringcolor", renderer));
    if(colorprop.IsNotNull())
      memcpy(rgba, colorprop->GetColor().GetDataPointer(), 3*sizeof(float));
    else
      GetColor( rgba, renderer );

  }
  if(selected)
  {
    mitk::ColorProperty::Pointer colorprop = dynamic_cast<mitk::ColorProperty*>(GetDataNode()->GetProperty
                                                                                ("binaryimage.selectedcolor", renderer));
    if(colorprop.IsNotNull())
      memcpy(rgba, colorprop->GetColor().GetDataPointer(), 3*sizeof(float));
    else
      GetColor( rgba, renderer );

  }
  if(!hover && !selected)
  {
    GetColor( rgba, renderer );
  }

  // check for opacity prop and use it for rendering if it exists
  GetOpacity( opacity, renderer );
  rgba[3] = opacity;

//  mitk::LevelWindow levelWindow;
  mitk::LevelWindow opacLevelWindow;

  bool binary = false;
  this->GetDataNode()->GetBoolProperty( "binary", binary, renderer );

  if ( binary )
  {    
    localStorage->m_Texture->MapColorScalarsThroughLookupTableOn();

//    image->setExtrema(0, 1);
//    image->setOpacityExtrema( 0.0, 255.0 );
//    image->setBinary(true);

    bool binaryOutline = false;
    if ( this->GetInput()->GetPixelType().GetBpe() <= 8 )
    {
      if (this->GetDataNode()->GetBoolProperty( "outline binary", binaryOutline, renderer ))
      {
//        image->setOutline(binaryOutline);
        float binaryOutlineWidth(1.0);
        if (this->GetDataNode()->GetFloatProperty( "outline width", binaryOutlineWidth, renderer ))
        {
//          image->setOutlineWidth(binaryOutlineWidth);
        }
      }
    }
    else
    {
      MITK_WARN << "Type of all binary images should be (un)signed char. Outline does not work on other pixel types!";
    }
  }
  else
  {
    localStorage->m_Texture->MapColorScalarsThroughLookupTableOff();
    LevelWindow levelWindow;
    if( !this->GetLevelWindow( levelWindow, renderer, "levelWindow" ) )
    {
      this->GetLevelWindow( levelWindow, renderer );
    }
//    image->setExtrema( levelWindow.GetLowerWindowBound(), levelWindow.GetUpperWindowBound() );


      ScalarType windowMin = 0.0;
      ScalarType windowMax = 255.0;

      //get the level window
    //  GetLevelWindow(levelWindow, renderer);
      windowMin = levelWindow.GetLowerWindowBound();
      windowMax = levelWindow.GetUpperWindowBound();

      //set up the lookuptable with the level window range
      localStorage->m_LookupTable->SetSaturationRange( 0.0, 0.0 );
      localStorage->m_LookupTable->SetHueRange( 0.0, 0.0 );
      localStorage->m_LookupTable->SetValueRange( 0.0, 1.0 );
      localStorage->m_LookupTable->SetRange( windowMin, windowMax );
      localStorage->m_LookupTable->Build();


      //set the lookuptable for the texture
      localStorage->m_Texture->SetLookupTable( localStorage->m_LookupTable );

    // obtain opacity level window
    if( this->GetLevelWindow( opacLevelWindow, renderer, "opaclevelwindow" ) )
    {
      MITK_INFO << "opace " << opacLevelWindow.GetLowerWindowBound() << " " << opacLevelWindow.GetUpperWindowBound();
//      image->setOpacityExtrema( opacLevelWindow.GetLowerWindowBound(), opacLevelWindow.GetUpperWindowBound() );
//      localStorage->m_LookupTable->SetRange(opacLevelWindow.GetLowerWindowBound(), opacLevelWindow.GetLowerWindowBound());
    }
    else
    {
//      image->setOpacityExtrema( 0.0, 255.0 );
//      localStorage->m_LookupTable->SetRange(0.0, 255.0);
    }
  }

  bool useColor = false;
  GetDataNode()->GetBoolProperty( "use color", useColor, renderer );
  mitk::LookupTableProperty::Pointer LookupTableProp;

  if ( !useColor )
  {
    LookupTableProp = dynamic_cast<mitk::LookupTableProperty*>(
        this->GetDataNode()->GetProperty("LookupTable"));

    if ( LookupTableProp.IsNull() )
    {
      useColor = true;
    }
  }

  if ( useColor || binary )
  {
    // If lookup table use is NOT requested (or we have a binary image...):
//    image->setColor( rgba[0], rgba[1], rgba[2], rgba[3] );
      double rgbConv[3] = {(double)rgba[0], (double)rgba[1], (double)rgba[2]};
      localStorage->m_Actor->GetProperty()->SetColor(rgbConv);
      localStorage->m_Actor->GetProperty()->SetOpacity(rgba[3]);
//      localStorage->m_Texture->SetLookupTable( localStorage->m_LookupTable );
//      localStorage->m_LookupTable->Build();
  }
  else
  {
    // If lookup table use is requested:
    // only update the lut, when the properties have changed...
    if ( LookupTableProp->GetLookupTable()->GetMTime()
      <= this->GetDataNode()->GetPropertyList()->GetMTime() )
      {
      LookupTableProp->GetLookupTable()->ChangeOpacityForAll( opacity );
      LookupTableProp->GetLookupTable()->ChangeOpacity(0, 0.0);
    }
//    image->setColors(LookupTableProp->GetLookupTable()->GetRawLookupTable());
    localStorage->m_Texture->SetLookupTable(LookupTableProp->GetLookupTable()->GetVtkLookupTable());
  }
}

void mitk::ImageVtkMapper2D::Update(mitk::BaseRenderer* renderer)
{
  if ( !this->IsVisible( renderer ) )
  {
    return;
  }

  mitk::Image* data  = const_cast<mitk::Image *>( this->GetInput() );
  if ( data == NULL )
  {
    return;
  }

  // Calculate time step of the input data for the specified renderer (integer value)
  this->CalculateTimeStep( renderer );

  // Check if time step is valid
  const TimeSlicedGeometry *dataTimeGeometry = data->GetTimeSlicedGeometry();
  if ( ( dataTimeGeometry == NULL )
    || ( dataTimeGeometry->GetTimeSteps() == 0 )
    || ( !dataTimeGeometry->IsValidTime( this->GetTimestep() ) ) )
    {
    return;
  }

  const DataNode *node = this->GetDataNode();
  RendererInfo& rendererInfo = AccessRendererInfo( renderer );
  data->UpdateOutputInformation();

  //check if something important has changed and we need to rerender
  if ( (rendererInfo.m_LastUpdateTime < node->GetMTime()) //was the node modified?
    || (rendererInfo.m_LastUpdateTime < data->GetPipelineMTime()) //Was the data modified?
    || (rendererInfo.m_LastUpdateTime < renderer->GetCurrentWorldGeometry2DUpdateTime()) //was the geometry modified?
    || (rendererInfo.m_LastUpdateTime < renderer->GetDisplayGeometryUpdateTime()) // TODO this does not work
    || (rendererInfo.m_LastUpdateTime < renderer->GetDisplayGeometry()->GetMTime()) //was the display geometry modified? e.g. zooming, panning
    || (rendererInfo.m_LastUpdateTime < renderer->GetCurrentWorldGeometry2D()->GetMTime())
    || (rendererInfo.m_LastUpdateTime < node->GetPropertyList()->GetMTime()) //was a property modified?
    || (rendererInfo.m_LastUpdateTime < node->GetPropertyList(renderer)->GetMTime()) )
    {
    this->GenerateData( renderer );
  }
  // since we have checked that nothing important has changed, we can set
  // m_LastUpdateTime to the current time
  rendererInfo.m_LastUpdateTime.Modified();
}

void mitk::ImageVtkMapper2D::DeleteRendererCallback( itk::Object *object, const itk::EventObject & )
{
  mitk::BaseRenderer *renderer = dynamic_cast< mitk::BaseRenderer* >( object );
  if ( renderer )
  {
    m_RendererInfo.erase( renderer );
  }
}

mitk::ImageVtkMapper2D::RendererInfo::RendererInfo()
  : m_RendererID(-1),
  m_Renderer(NULL),
  m_UnitSpacingImageFilter( NULL ),
  m_Reslicer( NULL ),
  m_TSFilter( NULL ),
  m_Image(NULL),
  m_ReferenceGeometry(NULL),
  m_TextureInterpolation(true),
  m_ObserverID( 0 )
{
  m_PixelsPerMM.Fill(0);
};

mitk::ImageVtkMapper2D::RendererInfo::~RendererInfo()
{
  this->Squeeze();

  if ( m_UnitSpacingImageFilter != NULL )
  {
    m_UnitSpacingImageFilter->Delete();
  }
  if ( m_Reslicer != NULL )
  {
    m_Reslicer->Delete();
  }
  if ( m_TSFilter != NULL )
  {
    m_TSFilter->Delete();
  }
  if ( m_Image != NULL )
  {
    m_Image->Delete();
  }
}

void mitk::ImageVtkMapper2D::RendererInfo::Squeeze()
{
  if ( m_Image != NULL )
  {
    m_Image->Delete();
    m_Image = NULL;
  }
}

void mitk::ImageVtkMapper2D::RendererInfo::RemoveObserver()
{
  if ( m_ObserverID != 0 )
  {
    // m_ObserverID has to be decreased by one. Was incremented by one after creation to make the test m_ObserverID != 0 possible.
    m_Renderer->RemoveObserver( m_ObserverID-1 );
  }
}

void mitk::ImageVtkMapper2D::RendererInfo::Initialize( int rendererID, mitk::BaseRenderer *renderer,
                                                       unsigned long observerID )
{
  // increase ID by one to avoid 0 ID, has to be decreased before remove of the observer
  m_ObserverID = observerID+1;

  assert(rendererID>=0);
  assert(m_RendererID<0);

  m_RendererID = rendererID;
  m_Renderer = renderer;

  m_Image = vtkImageData::New();

  m_Reslicer = vtkImageReslice::New();
  m_TSFilter = vtkMitkThickSlicesFilter::New();

  m_Reslicer->ReleaseDataFlagOn();
  m_TSFilter->ReleaseDataFlagOn();

  m_UnitSpacingImageFilter = vtkImageChangeInformation::New();
  m_UnitSpacingImageFilter->SetOutputSpacing( 1.0, 1.0, 1.0 );
}

void mitk::ImageVtkMapper2D::SetDefaultProperties(mitk::DataNode* node, mitk::BaseRenderer* renderer, bool overwrite)
{
  mitk::Image::Pointer image = dynamic_cast<mitk::Image*>(node->GetData());

  // Properties common for both images and segmentations
  node->AddProperty( "use color", mitk::BoolProperty::New( true ), renderer, overwrite );
  node->AddProperty( "outline binary", mitk::BoolProperty::New( false ), renderer, overwrite );
  node->AddProperty( "outline width", mitk::FloatProperty::New( 1.0 ), renderer, overwrite );
  if(image->IsRotated()) node->AddProperty( "reslice interpolation", mitk::VtkResliceInterpolationProperty::New(VTK_RESLICE_CUBIC) );
  else node->AddProperty( "reslice interpolation", mitk::VtkResliceInterpolationProperty::New() );
  node->AddProperty( "texture interpolation", mitk::BoolProperty::New( mitk::DataNodeFactory::m_TextureInterpolationActive ) );  // set to user configurable default value (see global options)
  node->AddProperty( "in plane resample extent by geometry", mitk::BoolProperty::New( false ) );
  node->AddProperty( "bounding box", mitk::BoolProperty::New( false ) );
  
  bool isBinaryImage(false);
  if ( ! node->GetBoolProperty("binary", isBinaryImage) )
  {

    // ok, property is not set, use heuristic to determine if this
    // is a binary image
    mitk::Image::Pointer centralSliceImage;
    ScalarType minValue = 0.0;
    ScalarType maxValue = 0.0;
    ScalarType min2ndValue = 0.0;
    ScalarType max2ndValue = 0.0;
    mitk::ImageSliceSelector::Pointer sliceSelector = mitk::ImageSliceSelector::New();

    sliceSelector->SetInput(image);
    sliceSelector->SetSliceNr(image->GetDimension(2)/2);
    sliceSelector->SetTimeNr(image->GetDimension(3)/2);
    sliceSelector->SetChannelNr(image->GetDimension(4)/2);
    sliceSelector->Update();
    centralSliceImage = sliceSelector->GetOutput();
    if ( centralSliceImage.IsNotNull() && centralSliceImage->IsInitialized() )
    {
      minValue    = centralSliceImage->GetScalarValueMin();
      maxValue    = centralSliceImage->GetScalarValueMax();
      min2ndValue = centralSliceImage->GetScalarValue2ndMin();
      max2ndValue = centralSliceImage->GetScalarValue2ndMax();
    }
    if ( minValue == maxValue )
    {
      // centralSlice is strange, lets look at all data
      minValue    = image->GetScalarValueMin();
      maxValue    = image->GetScalarValueMaxNoRecompute();
      min2ndValue = image->GetScalarValue2ndMinNoRecompute();
      max2ndValue = image->GetScalarValue2ndMaxNoRecompute();
    }
    isBinaryImage = ( maxValue == min2ndValue && minValue == max2ndValue );
  }

  // some more properties specific for a binary...
  if (isBinaryImage)
  {
    node->AddProperty( "opacity", mitk::FloatProperty::New(0.3f), renderer, overwrite );
    node->AddProperty( "color", ColorProperty::New(1.0,0.0,0.0), renderer, overwrite );
    node->AddProperty( "binaryimage.selectedcolor", ColorProperty::New(1.0,0.0,0.0), renderer, overwrite );
    node->AddProperty( "binaryimage.selectedannotationcolor", ColorProperty::New(1.0,0.0,0.0), renderer, overwrite );
    node->AddProperty( "binaryimage.hoveringcolor", ColorProperty::New(1.0,0.0,0.0), renderer, overwrite );
    node->AddProperty( "binaryimage.hoveringannotationcolor", ColorProperty::New(1.0,0.0,0.0), renderer, overwrite );
    node->AddProperty( "binary", mitk::BoolProperty::New( true ), renderer, overwrite );
    node->AddProperty("layer", mitk::IntProperty::New(10), renderer, overwrite);
  }
  else          //...or image type object
  {
    node->AddProperty( "opacity", mitk::FloatProperty::New(1.0f), renderer, overwrite );
    node->AddProperty( "color", ColorProperty::New(1.0,1.0,1.0), renderer, overwrite );
    node->AddProperty( "binary", mitk::BoolProperty::New( false ), renderer, overwrite );
    node->AddProperty("layer", mitk::IntProperty::New(0), renderer, overwrite);
  }

  if(image.IsNotNull() && image->IsInitialized())
  {
    if((overwrite) || (node->GetProperty("levelwindow", renderer)==NULL))
    {
      mitk::LevelWindowProperty::Pointer levWinProp = mitk::LevelWindowProperty::New();
      mitk::LevelWindow levelwindow;
      levelwindow.SetAuto( image, true, true );
      levWinProp->SetLevelWindow( levelwindow );
      node->SetProperty( "levelwindow", levWinProp, renderer );
    }
    if(((overwrite) || (node->GetProperty("opaclevelwindow", renderer)==NULL))
      && (image->GetPixelType().GetItkTypeId() && *(image->GetPixelType().GetItkTypeId()) == typeid(itk::RGBAPixel<unsigned char>)))
      {
      mitk::LevelWindow opaclevwin;
      opaclevwin.SetRangeMinMax(0,255);
      opaclevwin.SetWindowBounds(0,255);
      mitk::LevelWindowProperty::Pointer prop = mitk::LevelWindowProperty::New(opaclevwin);
      node->SetProperty( "opaclevelwindow", prop, renderer );
    }
    if((overwrite) || (node->GetProperty("LookupTable", renderer)==NULL))
    {
      // add a default rainbow lookup table for color mapping
      mitk::LookupTable::Pointer mitkLut = mitk::LookupTable::New();
      vtkLookupTable* vtkLut = mitkLut->GetVtkLookupTable();
      vtkLut->SetHueRange(0.6667, 0.0);
      vtkLut->SetTableRange(0.0, 20.0);
      vtkLut->Build();
      mitk::LookupTableProperty::Pointer mitkLutProp = mitk::LookupTableProperty::New();
      mitkLutProp->SetLookupTable(mitkLut);
      node->SetProperty( "LookupTable", mitkLutProp );
    }
  }  
  Superclass::SetDefaultProperties(node, renderer, overwrite);
}

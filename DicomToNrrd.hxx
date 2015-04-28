/*=============================================================================
//  --- DICOM to NRRD coverter ---+
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
//  Authors: Schuyler Kylstra
=============================================================================*/
#ifndef DicomToNrrd_hxx_included
#define DicomToNrrd_hxx_included

#include <cfloat>

/* ITK includes */
#include <itkImage.h>
#include <itkImageFileWriter.h>
#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImageSeriesReader.h>

#include <itkIdentityTransform.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkResampleImageFilter.h>
#include <itkSmartPointer.h>
#include <itkSpatialOrientationAdapter.h>

#include "ProgramArguments.h"


namespace DicomToNrrd {

  /*******************************************************************/
  /** Run the algorithm on an input image and write it to the output
      image. */
  /*******************************************************************/

  template< class TInput >
  int Execute( TInput * originalImage, itk::SmartPointer< TInput > & resampledInput, DicomToNrrd::ProgramArguments args)
  {
    /* Typedefs */
    typedef float                      TFloatType;
    typedef typename TInput::PixelType T;
    typedef T                          TPixelType;
    typedef T                          TLabelPixelType;

    const unsigned char DIMENSION = 3;

    typedef itk::Image<TPixelType, DIMENSION>      InputImageType;
    typedef itk::Image<TLabelPixelType, DIMENSION> LabelImageType;
    typedef itk::ImageFileWriter<LabelImageType>  WriterLabelType;

    typedef typename LabelImageType::SizeType    TSize;
    typedef typename LabelImageType::SpacingType TSpacing;
    typedef typename LabelImageType::PointType   TOrigin;
    typedef typename LabelImageType::IndexType   TIndex;

    /*  Automatic Resampling to RAI */
    typename InputImageType::DirectionType originalImageDirection = originalImage->GetDirection();

    itk::SpatialOrientationAdapter adapter;
    typename InputImageType::DirectionType RAIDirection = adapter.ToDirectionCosines(itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAI);

    bool shouldConvert = false;

    for ( int i = 0; i < 3; ++i ) {
      for ( int j = 0; j < 3; ++j ) {
        if (abs(originalImageDirection[i][j] - RAIDirection[i][j]) > 1e-6) {
          shouldConvert = true;
          break;
        }
      }
    }

    typedef itk::ResampleImageFilter< InputImageType, InputImageType > ResampleImageFilterType;
    typename ResampleImageFilterType::Pointer resampleFilter = ResampleImageFilterType::New();

    if ( shouldConvert ) {
      typedef itk::IdentityTransform< double, DIMENSION > IdentityTransformType;

      // Figure out bounding box of rotated image
      double boundingBox[6] = { DBL_MAX, -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX, -DBL_MAX };
      typedef typename InputImageType::IndexType  IndexType;
      typedef typename InputImageType::RegionType RegionType;
      typedef typename InputImageType::PointType  PointType;

      RegionType region = originalImage->GetLargestPossibleRegion();
      IndexType lowerUpper[2];
      lowerUpper[0] = region.GetIndex();
      lowerUpper[1] = region.GetUpperIndex();

      for ( unsigned int i = 0; i < 8; ++i ) {
        IndexType cornerIndex;
        cornerIndex[0] = lowerUpper[ (i & 1u) >> 0 ][0];
        cornerIndex[1] = lowerUpper[ (i & 2u) >> 1 ][1];
        cornerIndex[2] = lowerUpper[ (i & 4u) >> 2 ][2];
        std::cout << "cornerIndex: " << cornerIndex << std::endl;

        PointType point;
        originalImage->TransformIndexToPhysicalPoint( cornerIndex, point );
        boundingBox[0] = std::min( point[0], boundingBox[0] );
        boundingBox[1] = std::max( point[0], boundingBox[1] );
        boundingBox[2] = std::min( point[1], boundingBox[2] );
        boundingBox[3] = std::max( point[1], boundingBox[3] );
        boundingBox[4] = std::min( point[2], boundingBox[4] );
        boundingBox[5] = std::max( point[2], boundingBox[5] );
      }

      // Now transform the bounding box from physical space to index space
      PointType lowerPoint;
      lowerPoint[0] = boundingBox[0];
      lowerPoint[1] = boundingBox[2];
      lowerPoint[2] = boundingBox[4];

      PointType upperPoint;
      upperPoint[0] = boundingBox[1];
      upperPoint[1] = boundingBox[3];
      upperPoint[2] = boundingBox[5];

      typename InputImageType::Pointer dummyImage = InputImageType::New();
      dummyImage->SetOrigin( lowerPoint );
      dummyImage->SetSpacing( originalImage->GetSpacing() );
      dummyImage->SetLargestPossibleRegion( RegionType() );

      IndexType newLower, newUpper;
      dummyImage->TransformPhysicalPointToIndex( lowerPoint, newLower );
      dummyImage->TransformPhysicalPointToIndex( upperPoint, newUpper );

      RegionType outputRegion;
      outputRegion.SetIndex( newLower );
      outputRegion.SetUpperIndex( newUpper );

      // Find the minimum pixel value in the image. This will be used as the default value
      // in the resample filter.
      typedef itk::MinimumMaximumImageCalculator< InputImageType > MinMaxType;
      typename MinMaxType::Pointer minMaxCalculator = MinMaxType::New();
      minMaxCalculator->SetImage( originalImage );
      minMaxCalculator->Compute();

      resampleFilter->SetTransform( IdentityTransformType::New() );
      resampleFilter->SetInput( originalImage );
      resampleFilter->SetSize( outputRegion.GetSize() );
      resampleFilter->SetOutputOrigin( lowerPoint );
      resampleFilter->SetOutputSpacing( originalImage->GetSpacing() );
      resampleFilter->SetDefaultPixelValue( minMaxCalculator->GetMinimum() );
      resampleFilter->Update();

      originalImage = resampleFilter->GetOutput();
    }

    resampledInput = originalImage;

    return EXIT_SUCCESS;

  }



    template <class T>
  int ExecuteFromFile( const ProgramArguments & args, T)
  {
    /* Typedefs */
    typedef float TFloatType;
    typedef T TPixelType;
    typedef T TLabelPixelType;

    const unsigned char DIMENSION = 3;

    typedef itk::Image<TPixelType, DIMENSION> InputImageType;
    typedef itk::Image<TLabelPixelType, DIMENSION> LabelImageType;
    typedef itk::GDCMImageIO ImageIOType;

    typedef itk::ImageSeriesReader< InputImageType > ReaderType;
    typedef itk::GDCMSeriesFileNames NamesGeneratorType;
    typedef std::vector< std::string >   SeriesIdContainer;
    typedef std::vector< std::string >   FileNamesContainer;
    typedef itk::ImageFileWriter<LabelImageType> WriterLabelType;


    
    typename ReaderType::Pointer reader = ReaderType::New();

    ImageIOType::Pointer dicomIO = ImageIOType::New();

    reader->SetImageIO( dicomIO );

    
    NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();

    nameGenerator->SetDirectory( args.dicomDir );


    try
    {
      
      const SeriesIdContainer & seriesUID = nameGenerator->GetSeriesUIDs();
      SeriesIdContainer::const_iterator seriesItr = seriesUID.begin();

      std::string seriesIdentifier;
      seriesIdentifier = seriesUID.begin()->c_str();


      
      FileNamesContainer fileNames;
      fileNames = nameGenerator->GetFileNames( seriesIdentifier );
      reader->SetFileNames( fileNames );

      // Read the input file
      reader->Update();
    }
    catch (itk::ExceptionObject &ex)
    {
      std::cerr << "Exception caught: " << ex << std::endl;
      return EXIT_FAILURE;
    }
    

    // Run the algorithm
    typename InputImageType::Pointer resampledInput;
    int result = Execute( reader->GetOutput(), resampledInput, args );
    if ( result != EXIT_SUCCESS ) {
      return result;
    }

    // Write the result.
    typename WriterLabelType::Pointer writer = WriterLabelType::New();
    writer->SetInput( resampledInput );
    writer->SetFileName( args.outputImage );
    writer->Update();

    return EXIT_SUCCESS;
  }

}

#endif

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

/* STL includes */
#include <string>

/* Local includes */
#include "DicomToNrrdCLP.h"
#include "DicomToNrrdConfig.h"
#include "DicomToNrrd.hxx"
#include "ProgramArguments.h"

#include <itkImage.h>
#include <itkImageFileReader.h>

namespace DicomToNrrd {

  /*******************************************************************/
  /** Query the image type. */
  /*******************************************************************/
  void GetImageType ( std::string fileName,
                      itk::ImageIOBase::IOPixelType & pixelType,
                      itk::ImageIOBase::IOComponentType & componentType)
  {
    typedef itk::Image<unsigned char, 3> ImageType;
    itk::ImageFileReader<ImageType>::Pointer imageReader =
      itk::ImageFileReader<ImageType>::New();

    imageReader->SetFileName(fileName.c_str());
    imageReader->UpdateOutputInformation();

    pixelType = imageReader->GetImageIO()->GetPixelType();
    componentType = imageReader->GetImageIO()->GetComponentType();
  }
  
} // end namespace DicomToNrrd

/*******************************************************************/
int main( int argc, char * argv[] )
{

  PARSE_ARGS;

  DicomToNrrd::ProgramArguments args;
  args.dicomDir    = dicomDir;
  args.outputImage = outputImage;
  

  itk::ImageIOBase::IOPixelType     inputPixelType;
  itk::ImageIOBase::IOComponentType inputComponentType;

  int ret = EXIT_FAILURE;

  // GetImageType(inputImage, inputPixelType, inputComponentType);

  try {
    ret = DicomToNrrd::ExecuteFromFile( args, static_cast<short>(0) );
  } catch( itk::ExceptionObject & excep ) {
    std::cerr << argv[0] << ": exception caught !" << std::endl;
    std::cerr << excep << std::endl;

    return EXIT_FAILURE;
  }

  return ret;
}


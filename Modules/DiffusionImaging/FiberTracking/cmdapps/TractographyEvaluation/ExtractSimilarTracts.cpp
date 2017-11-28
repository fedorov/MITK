﻿/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include <mitkFiberBundle.h>
#include <mitkCommandLineParser.h>
#include <boost/lexical_cast.hpp>
#include <mitkIOUtil.h>
#include <itkTractClusteringFilter.h>

typedef itksys::SystemTools ist;
typedef itk::Image<unsigned char, 3>    ItkUcharImgType;

mitk::FiberBundle::Pointer LoadFib(std::string filename)
{
  std::vector<mitk::BaseData::Pointer> fibInfile = mitk::IOUtil::Load(filename);
  if( fibInfile.empty() )
    std::cout << "File " << filename << " could not be read!";
  mitk::BaseData::Pointer baseData = fibInfile.at(0);
  return dynamic_cast<mitk::FiberBundle*>(baseData.GetPointer());
}

ItkUcharImgType::Pointer LoadItkMaskImage(const std::string& filename)
{
  mitk::Image::Pointer img = dynamic_cast<mitk::Image*>(mitk::IOUtil::Load(filename)[0].GetPointer());
  ItkUcharImgType::Pointer itkMask = ItkUcharImgType::New();
  mitk::CastToItkImage(img, itkMask);
  return itkMask;
}

/*!
\brief Spatially cluster fibers
*/
int main(int argc, char* argv[])
{
  mitkCommandLineParser parser;

  parser.setTitle("Extract Similar Tracts");
  parser.setCategory("Fiber Tracking Evaluation");
  parser.setContributor("MIC");

  parser.setArgumentPrefix("--", "-");
  parser.addArgument("", "i", mitkCommandLineParser::InputFile, "Input:", "input fiber bundle (.fib, .trk, .tck)", us::Any(), false);
  parser.addArgument("ref_tracts", "", mitkCommandLineParser::StringList, "Ref. Tracts:", "reference tracts (.fib, .trk, .tck)", us::Any(), false);
  parser.addArgument("ref_masks", "", mitkCommandLineParser::StringList, "Ref. Masks:", "reference bundle masks", us::Any());
  parser.addArgument("", "o", mitkCommandLineParser::OutputDirectory, "Output:", "output root", us::Any(), false);
  parser.addArgument("distance", "", mitkCommandLineParser::Int, "Distance:", "", 10);
  parser.addArgument("metric", "", mitkCommandLineParser::String, "Metric:", "");

  std::map<std::string, us::Any> parsedArgs = parser.parseArguments(argc, argv);
  if (parsedArgs.size()==0)
    return EXIT_FAILURE;

  std::string in_fib = us::any_cast<std::string>(parsedArgs["i"]);
  std::string out_root = us::any_cast<std::string>(parsedArgs["o"]);
  mitkCommandLineParser::StringContainerType ref_bundle_files = us::any_cast<mitkCommandLineParser::StringContainerType>(parsedArgs["ref_tracts"]);
  mitkCommandLineParser::StringContainerType ref_mask_files;
  if (parsedArgs.count("ref_masks"))
    ref_mask_files = us::any_cast<mitkCommandLineParser::StringContainerType>(parsedArgs["ref_masks"]);

  if (ref_mask_files.size()>0 && ref_mask_files.size()!=ref_bundle_files.size())
  {
    MITK_INFO << "If reference masks are used, there has to be one mask per reference tract.";
    return EXIT_FAILURE;
  }

  int distance = 10;
  if (parsedArgs.count("distance"))
    distance = us::any_cast<int>(parsedArgs["distance"]);

  std::string metric = "MDF_STD";
  if (parsedArgs.count("metric"))
    metric = us::any_cast<std::string>(parsedArgs["metric"]);

  try
  {
    mitk::FiberBundle::Pointer fib = LoadFib(in_fib);
    fib->ResampleToNumPoints(12);

    std::vector< mitk::FiberBundle::Pointer > ref_fibs;
    std::vector< ItkUcharImgType::Pointer > ref_masks;
    for (std::size_t i=0; i<ref_bundle_files.size(); ++i)
    {
      MITK_INFO << "Loading " << ist::GetFilenameName(ref_bundle_files.at(i));
      if (i<ref_mask_files.size())
        MITK_INFO << "Loading " << ist::GetFilenameName(ref_mask_files.at(i));

      std::streambuf *old = cout.rdbuf(); // <-- save
      std::stringstream ss;
      std::cout.rdbuf (ss.rdbuf());       // <-- redirect
      try
      {
        ref_fibs.push_back(LoadFib(ref_bundle_files.at(i)));
        if (i<ref_mask_files.size())
          ref_masks.push_back(LoadItkMaskImage(ref_mask_files.at(i)));
        else
          ref_masks.push_back(nullptr);
        std::cout.rdbuf (old);              // <-- restore
      }
      catch(...){
        std::cout.rdbuf (old);              // <-- restore
        std::cout << "could not load: " << ref_bundle_files.at(i);
        return EXIT_FAILURE;
      }
    }

    std::vector< float > distances;
    distances.push_back(distance);

    mitk::FiberBundle::Pointer anchor_tractogram = mitk::FiberBundle::New(nullptr);
    unsigned int c = 0;
    for (auto ref_fib : ref_fibs)
    {
      MITK_INFO << "Extracting " << ist::GetFilenameName(ref_bundle_files.at(c));

      std::streambuf *old = cout.rdbuf(); // <-- save
      std::stringstream ss;
      std::cout.rdbuf (ss.rdbuf());       // <-- redirect
      try
      {
        itk::TractClusteringFilter::Pointer segmenter = itk::TractClusteringFilter::New();

        // calculate centroids from reference bundle
        {
          itk::TractClusteringFilter::Pointer clusterer = itk::TractClusteringFilter::New();
          clusterer->SetDistances({10,20,30});
          clusterer->SetTractogram(ref_fib);
          clusterer->SetMetric(itk::TractClusteringFilter::Metric::MDF_STD);
          clusterer->Update();
          std::vector<mitk::FiberBundle::Pointer> tracts = clusterer->GetOutCentroids();
          ref_fib = mitk::FiberBundle::New(nullptr);
          ref_fib = ref_fib->AddBundles(tracts);
          mitk::IOUtil::Save(ref_fib, out_root + "centroids_" + ist::GetFilenameName(ref_bundle_files.at(c)));
          segmenter->SetInCentroids(ref_fib);
        }

        // segment tract
        segmenter->SetFilterMask(ref_masks.at(c));
        segmenter->SetOverlapThreshold(0.8);
        segmenter->SetDistances(distances);
        segmenter->SetTractogram(fib);
        segmenter->SetDoResampling(false);
        if (metric=="MDF")
          segmenter->SetMetric(itk::TractClusteringFilter::Metric::MDF);
        else if (metric=="MDF_STD")
          segmenter->SetMetric(itk::TractClusteringFilter::Metric::MDF_STD);
        else if (metric=="MAX_MDF")
          segmenter->SetMetric(itk::TractClusteringFilter::Metric::MAX_MDF);
        segmenter->Update();

        std::vector<mitk::FiberBundle::Pointer> clusters = segmenter->GetOutTractograms();
        if (clusters.size()>0)
        {
          fib = clusters.back();
          clusters.pop_back();
          mitk::FiberBundle::Pointer result = mitk::FiberBundle::New(nullptr);
          result = result->AddBundles(clusters);
          anchor_tractogram = anchor_tractogram->AddBundle(result);
          mitk::IOUtil::Save(result, out_root + "anchor_" + ist::GetFilenameName(ref_bundle_files.at(c)));
        }
      }
      catch(itk::ExceptionObject& excpt)
      {
        MITK_INFO << "Exception while processing " << ist::GetFilenameName(ref_bundle_files.at(c));
        MITK_INFO << excpt.GetDescription();
      }
      catch(std::exception& excpt)
      {
        MITK_INFO << "Exception while processing " << ist::GetFilenameName(ref_bundle_files.at(c));
        MITK_INFO << excpt.what();
      }
      std::cout.rdbuf (old);              // <-- restore
      if (fib->GetNumFibers()==0)
        break;
      ++c;
    }
    MITK_INFO << "Streamlines in anchor tractogram: " << anchor_tractogram->GetNumFibers();
    mitk::IOUtil::Save(anchor_tractogram, out_root + "anchor_tractogram.trk");

    MITK_INFO << "Streamlines remaining in candidate tractogram: " << fib->GetNumFibers();
    mitk::IOUtil::Save(fib, out_root + "candidate_tractogram.trk");
  }
  catch (itk::ExceptionObject e)
  {
    std::cout << e;
    return EXIT_FAILURE;
  }
  catch (std::exception e)
  {
    std::cout << e.what();
    return EXIT_FAILURE;
  }
  catch (...)
  {
    std::cout << "ERROR!?!";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

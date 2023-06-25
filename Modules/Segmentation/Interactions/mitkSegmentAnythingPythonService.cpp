/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkSegmentAnythingPythonService.h"

#include "mitkIOUtil.h"
#include <itksys/SystemTools.hxx>
#include <chrono>
#include <thread>
#include <filesystem>

using namespace std::chrono_literals;

namespace mitk
{
  const std::string SIGNALCONSTANTS::READY = "READY";
  const std::string SIGNALCONSTANTS::KILL = "KILL";
  const std::string SIGNALCONSTANTS::OFF = "OFF";
  const std::string SIGNALCONSTANTS::CUDA_OUT_OF_MEMORY_ERROR = "CudaOutOfMemoryError";
  SegmentAnythingPythonService::Status SegmentAnythingPythonService::CurrentStatus =
    SegmentAnythingPythonService::Status::OFF;
}

mitk::SegmentAnythingPythonService::SegmentAnythingPythonService(
  std::string workingDir, std::string modelType, std::string checkPointPath, unsigned int gpuId)
  : m_PythonPath(workingDir),
    m_ModelType(modelType),
    m_CheckpointPath(checkPointPath),
    m_GpuId(gpuId)
{
  this->CreateTempDirs(m_PARENT_TEMP_DIR_PATTERN);
}

mitk::SegmentAnythingPythonService::~SegmentAnythingPythonService()
{
  if (CurrentStatus == Status::READY)
  {
    this->StopAsyncProcess();
  }
  CurrentStatus = Status::OFF;
  std::filesystem::remove_all(this->GetMitkTempDir());
}

void mitk::SegmentAnythingPythonService::onPythonProcessEvent(itk::Object*, const itk::EventObject &e, void*)
{
  std::string testCOUT,testCERR;
  const auto *pEvent = dynamic_cast<const mitk::ExternalProcessStdOutEvent *>(&e);
  if (pEvent)
  {
    testCOUT = testCOUT + pEvent->GetOutput();
    testCOUT.erase(std::find_if(testCOUT.rbegin(), testCOUT.rend(), [](unsigned char ch) {
        return !std::isspace(ch);}).base(), testCOUT.end()); // remove trailing whitespaces, if any
    if (SIGNALCONSTANTS::READY == testCOUT)
    {
      CurrentStatus = Status::READY;
    }
    if (SIGNALCONSTANTS::KILL == testCOUT)
    {
      CurrentStatus = Status::KILLED;
    }
    if (SIGNALCONSTANTS::CUDA_OUT_OF_MEMORY_ERROR == testCOUT)
    {
      CurrentStatus = Status::CUDAError;
    }
    MITK_INFO << testCOUT;
  }
  const auto *pErrEvent = dynamic_cast<const mitk::ExternalProcessStdErrEvent *>(&e);
  if (pErrEvent)
  {
    testCERR = testCERR + pErrEvent->GetOutput();
    MITK_ERROR << testCERR;
  }
}

void mitk::SegmentAnythingPythonService::StopAsyncProcess()
{
  std::stringstream controlStream;
  controlStream << SIGNALCONSTANTS::KILL;
  this->WriteControlFile(controlStream);
  m_Future.get();
}

void mitk::SegmentAnythingPythonService::StartAsyncProcess()
{
  if (nullptr != m_DaemonExec)
  {
    this->StopAsyncProcess();
  }
  if (this->GetMitkTempDir().empty())
  {
    this->CreateTempDirs(m_PARENT_TEMP_DIR_PATTERN);
  }
  std::stringstream controlStream;
  controlStream << SIGNALCONSTANTS::READY;
  this->WriteControlFile(controlStream);

  m_DaemonExec = ProcessExecutor::New();
  itk::CStyleCommand::Pointer spCommand = itk::CStyleCommand::New();
  spCommand->SetCallback(&mitk::SegmentAnythingPythonService::onPythonProcessEvent);
  m_DaemonExec->AddObserver(ExternalProcessOutputEvent(), spCommand);
  m_Future = std::async(std::launch::async, &mitk::SegmentAnythingPythonService::start_python_daemon, this);
}

void mitk::SegmentAnythingPythonService::TransferPointsToProcess(std::stringstream &triggerCSV)
{
  CheckStatus();
  std::string triggerFilePath = m_InDir + IOUtil::GetDirectorySeparator() + m_TRIGGER_FILENAME;
  std::ofstream csvfile;
  csvfile.open(triggerFilePath, std::ofstream::out | std::ofstream::trunc);
  csvfile << triggerCSV.rdbuf();
  csvfile.close();
}

void mitk::SegmentAnythingPythonService::WriteControlFile(std::stringstream &statusStream)
{
  std::string controlFilePath = m_InDir + IOUtil::GetDirectorySeparator() + "control.txt";
  std::ofstream controlFile;
  controlFile.open(controlFilePath, std::ofstream::out | std::ofstream::trunc);
  controlFile << statusStream.rdbuf();
  controlFile.close();
}

void mitk::SegmentAnythingPythonService::start_python_daemon()
{
  ProcessExecutor::ArgumentListType args;
  std::string command = "python";
  args.push_back("-u");

  args.push_back("run_inference_daemon.py");
  
  args.push_back("--input-folder");
  args.push_back(m_InDir);

  args.push_back("--output-folder");
  args.push_back(m_OutDir);

  args.push_back("--trigger-file");
  args.push_back(m_TRIGGER_FILENAME);

  args.push_back("--model-type");
  args.push_back(m_ModelType);

  args.push_back("--checkpoint");
  args.push_back(m_CheckpointPath);

  args.push_back("--device");
  if (m_GpuId == -1)
  {
    args.push_back("cpu");
  }
  else
  {
    args.push_back("cuda");
    std::string cudaEnv = "CUDA_VISIBLE_DEVICES=" + std::to_string(m_GpuId);
    itksys::SystemTools::PutEnv(cudaEnv.c_str());
  }

  try
  {
    std::stringstream logStream;
    for (const auto &arg : args)
      logStream << arg << " ";
    logStream << m_PythonPath;
    MITK_INFO << logStream.str();
    m_DaemonExec->Execute(m_PythonPath, command, args);
  }
  catch (const mitk::Exception &e)
  {
    MITK_ERROR << e.GetDescription();
    return;
  }
  MITK_INFO << "Python process ended.";
}

bool mitk::SegmentAnythingPythonService::CheckStatus()
{
  switch (CurrentStatus)
  {
    case mitk::SegmentAnythingPythonService::Status::READY:
      return true;
    case mitk::SegmentAnythingPythonService::Status::CUDAError:
      mitkThrow() << "Error: Cuda Out of Memory. Change your model type in Preferences and Activate Segment Anything tool again.";
    case mitk::SegmentAnythingPythonService::Status::KILLED:
      mitkThrow() << "Error: Python process is already terminated. Cannot load requested segmentation. Activate Segment Anything tool again.";
    default:
      return false;
  }
}

void mitk::SegmentAnythingPythonService::CreateTempDirs(const std::string &dirPattern)
{
  this->SetMitkTempDir(IOUtil::CreateTemporaryDirectory(dirPattern));
  m_InDir = IOUtil::CreateTemporaryDirectory("sam-in-XXXXXX", m_MitkTempDir);
  m_OutDir = IOUtil::CreateTemporaryDirectory("sam-out-XXXXXX", m_MitkTempDir);
}

mitk::LabelSetImage::Pointer mitk::SegmentAnythingPythonService::RetrieveImageFromProcess()
{
  std::string outputImagePath = m_OutDir + IOUtil::GetDirectorySeparator() + m_CurrentUId + ".nrrd";
  while (!std::filesystem::exists(outputImagePath))
  {
    this->CheckStatus();
    std::this_thread::sleep_for(100ms);
  }
  LabelSetImage::Pointer outputBuffer = mitk::IOUtil::Load<LabelSetImage>(outputImagePath);
  return outputBuffer;
}

void mitk::SegmentAnythingPythonService::TransferImageToProcess(const Image *inputAtTimeStep, std::string &UId)
{
  std::string inputImagePath = m_InDir + IOUtil::GetDirectorySeparator() + UId + ".nrrd";
  IOUtil::Save(inputAtTimeStep, inputImagePath);
  m_CurrentUId = UId;
}

/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.s

============================================================================*/

#include "QmitkSetupVirtualEnvUtil.h"

#include "mitkLog.h"
#include <QStandardPaths>
#include <itkCommand.h>
#include <QDirIterator>

QmitkSetupVirtualEnvUtil::QmitkSetupVirtualEnvUtil()
{
  m_BaseDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QDir::separator() +
              qApp->organizationName() + QDir::separator();
}

QmitkSetupVirtualEnvUtil::QmitkSetupVirtualEnvUtil(const QString &baseDir)
{
  m_BaseDir = baseDir;
}

QString& QmitkSetupVirtualEnvUtil::GetBaseDir()
{
  return m_BaseDir;
}

QString QmitkSetupVirtualEnvUtil::GetVirtualEnvPath()
{
  return m_venvPath;
}

QString& QmitkSetupVirtualEnvUtil::GetSystemPythonPath()
{
  return m_SysPythonPath;
}

QString& QmitkSetupVirtualEnvUtil::GetPythonPath()
{
  return m_PythonPath;
}

QString& QmitkSetupVirtualEnvUtil::GetPipPath()
{
  return m_PipPath;
}

void QmitkSetupVirtualEnvUtil::SetVirtualEnvPath(const QString &path)
{
  m_venvPath = path;
}

void QmitkSetupVirtualEnvUtil::SetPipPath(const QString &path)
{
  m_PipPath = path;
}

void QmitkSetupVirtualEnvUtil::SetPythonPath(const QString &path)
{
  if (this->IsPythonPath(path))
  {
    m_PythonPath = path;
  }
  else
  {
    MITK_INFO << "Python was not detected in " + path.toStdString();
  }
}

void QmitkSetupVirtualEnvUtil::SetSystemPythonPath(const QString &path)
{
  if (this->IsPythonPath(path))
  {
    m_SysPythonPath = path;
  }
  else
  {
    MITK_INFO << "Python was not detected in " + path.toStdString();
  }
}

void QmitkSetupVirtualEnvUtil::PrintProcessEvent(itk::Object * /*pCaller*/, const itk::EventObject &e, void *)
{
  std::string testCOUT;
  std::string testCERR;
  const auto *pEvent = dynamic_cast<const mitk::ExternalProcessStdOutEvent *>(&e);
  if (pEvent)
  {
    testCOUT = testCOUT + pEvent->GetOutput();
    MITK_INFO << testCOUT;
  }
  const auto *pErrEvent = dynamic_cast<const mitk::ExternalProcessStdErrEvent *>(&e);
  if (pErrEvent)
  {
    testCERR = testCERR + pErrEvent->GetOutput();
    MITK_ERROR << testCERR;
  }
}

void QmitkSetupVirtualEnvUtil::InstallPytorch(const std::string &workingDir,
                                              void (*callback)(itk::Object *, const itk::EventObject &, void *))
{
  mitk::ProcessExecutor::ArgumentListType args;
  auto spExec = mitk::ProcessExecutor::New();
  auto spCommand = itk::CStyleCommand::New();
  spCommand->SetCallback(callback);
  spExec->AddObserver(mitk::ExternalProcessOutputEvent(), spCommand);
  args.push_back("-m");
  args.push_back("pip");
  args.push_back("install");
  args.push_back("light-the-torch==0.7.5");
  spExec->Execute(workingDir, "python", args);
  PipInstall("torch==2.0.0", workingDir, callback, "ltt");
  PipInstall("torchvision==0.15.0", workingDir, callback, "ltt");
}

void QmitkSetupVirtualEnvUtil::InstallPytorch()
{
  this->InstallPytorch(GetPythonPath().toStdString(), &PrintProcessEvent);
}

void QmitkSetupVirtualEnvUtil::PipInstall(const std::string &library,
                                          const std::string &workingDir,
                                          void (*callback)(itk::Object *, const itk::EventObject &, void *),
                                          const std::string &command)
{
  mitk::ProcessExecutor::ArgumentListType args;
  auto spExec = mitk::ProcessExecutor::New();
  auto spCommand = itk::CStyleCommand::New();
  spCommand->SetCallback(callback);
  spExec->AddObserver(mitk::ExternalProcessOutputEvent(), spCommand);
  args.push_back("install");
  args.push_back(library);
  spExec->Execute(workingDir, command, args);
}

void QmitkSetupVirtualEnvUtil::PipInstall(const std::string &library,
  void (*callback)(itk::Object*, const itk::EventObject&, void*),
  const std::string& command)
{
  this->PipInstall(library, this->GetPipPath().toStdString(), callback, command);
}

void QmitkSetupVirtualEnvUtil::ExecutePython(const std::string &pythonCode,
                                             const std::string &workingDir,
                                             void (*callback)(itk::Object *, const itk::EventObject &, void *),
                                             const std::string &command)
{
  mitk::ProcessExecutor::ArgumentListType args;
  auto spExec = mitk::ProcessExecutor::New();
  auto spCommand = itk::CStyleCommand::New();
  spCommand->SetCallback(callback);
  spExec->AddObserver(mitk::ExternalProcessOutputEvent(), spCommand);
  args.push_back("-c");
  args.push_back(pythonCode);
  spExec->Execute(workingDir, command, args);
}

void QmitkSetupVirtualEnvUtil::ExecutePython(const std::string &args,
                                             void (*callback)(itk::Object *, const itk::EventObject &, void *),
                                             const std::string &command)
{
  this->ExecutePython(args, this->GetPythonPath().toStdString(), callback, command);
}

bool QmitkSetupVirtualEnvUtil::IsPythonPath(const QString &pythonPath)
{
  QString fullPath = pythonPath;
  bool isExists =
#ifdef _WIN32
    QFile::exists(fullPath + QDir::separator() + QString("python.exe"));
#else
    QFile::exists(fullPath + QDir::separator() + QString("python3"));
#endif
  return isExists;
}

namespace
{
  bool CheckForFiles(QString path, QStringList supportedPipVersions)
  {
    bool isFileFound = false;
    for (QDirIterator subIt(path, QDir::AllEntries, QDirIterator::NoIteratorFlags); subIt.hasNext();)
    {
      subIt.next();
      QString fileName = subIt.fileName();
      if (supportedPipVersions.contains(fileName))
      {
        isFileFound = true;
        break;
      }
    }
    return isFileFound;
  }
} // namespace

QString QmitkSetupVirtualEnvUtil::GetExactPythonPath(const QString &pyEnv)
{
  QString fullPath = pyEnv;
  bool isPythonExists = false;
  bool isSupportedVersion = false;
  const QStringList supportedPythonVersions
#ifdef _WIN32
    = {"pip3.9.exe", "pip3.10.exe", "pip3.11.exe", "python39.dll", "python310.dll", "python311.dll"};
  // check if python exist in given folder.
  isPythonExists = QFile::exists(fullPath + QDir::separator() + QString("python.exe"));
  // check for python dll file for version
  isSupportedVersion = ::CheckForFiles(fullPath, supportedPythonVersions);
  if (!isPythonExists && // check if in Scripts already, if not go there
      !(fullPath.endsWith("Scripts", Qt::CaseInsensitive) || fullPath.endsWith("Scripts/", Qt::CaseInsensitive)))
  {
    fullPath += QDir::separator() + QString("Scripts");
    isPythonExists = QFile::exists(fullPath + QDir::separator() + QString("python.exe"));
    if (!isSupportedVersion) // check for version again, if not successful earlier
    {
      isSupportedVersion = ::CheckForFiles(fullPath, supportedPythonVersions);
    }
  }
#else
    = {"python3.9", "python3.10", "python3.11"};
  isPythonExists = QFile::exists(fullPath + QDir::separator() + QString("python3"));
  isSupportedVersion = ::CheckForFiles(fullPath, supportedPythonVersions);
  if (!isSupportedVersion)
  {
    QString libPath = fullPath + QDir::separator() + QString("lib");
    isSupportedVersion = ::CheckForFiles(libPath, supportedPythonVersions);
  }
  if (!isPythonExists &&
      !(fullPath.endsWith("bin", Qt::CaseInsensitive) || fullPath.endsWith("bin/", Qt::CaseInsensitive)))
  {
    fullPath += QDir::separator() + QString("bin");
    isPythonExists = QFile::exists(fullPath + QDir::separator() + QString("python3"));
  }
#endif
  if (!isPythonExists || !isSupportedVersion)
  {
    fullPath.clear();
  }
  return fullPath;
}

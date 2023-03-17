/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkLabelSet.h"
#include "mitkDICOMSegmentationPropertyHelper.h"

#include <itkCommand.h>

mitk::LabelSet::LabelSet() : m_ActiveLabelValue(0), m_Layer(0)
{
  m_LookupTable = mitk::LookupTable::New();
  m_LookupTable->SetType(mitk::LookupTable::MULTILABEL);
  m_ReservedLabelValuesFunctor = [this]() {
    std::vector<LabelValueType> result = { 0 };

    for (auto [value, label] : this->m_LabelContainer)
    {
      result.emplace_back(value);
    }
    return result;
  };
}

mitk::LabelSet::~LabelSet()
{
  m_LabelContainer.clear();
}

mitk::LabelSet::LabelSet(const LabelSet &other)
  : itk::Object(),
    m_LookupTable(other.GetLookupTable()->Clone()),
    m_ActiveLabelValue(other.GetActiveLabel()->GetValue()),
    m_Layer(other.GetLayer())
{
  // clone Labels
  auto otherIt = other.IteratorConstBegin();
  for (; otherIt != other.IteratorConstEnd(); ++otherIt)
  {
    m_LabelContainer[otherIt->first] = otherIt->second->Clone();

    itk::MemberCommand<LabelSet>::Pointer command = itk::MemberCommand<LabelSet>::New();
    command->SetCallbackFunction(this, &LabelSet::OnLabelModified);
    m_LabelContainer[otherIt->first]->AddObserver(itk::ModifiedEvent(), command);
  }
  m_ReservedLabelValuesFunctor = other.m_ReservedLabelValuesFunctor;
}

void mitk::LabelSet::OnLabelModified(const Object* sender, const itk::EventObject&)
{
  auto label = dynamic_cast<const Label*>(sender);
  if (nullptr == label) mitkThrow() << "LabelSet is in wrong state. LabelModified event is not send by a label instance.";

  ModifyLabelEvent.Send(label->GetValue());
  Superclass::Modified();
}

mitk::LabelSet::LabelContainerConstIteratorType mitk::LabelSet::IteratorConstEnd() const
{
  return m_LabelContainer.end();
}

mitk::LabelSet::LabelContainerConstIteratorType mitk::LabelSet::IteratorConstBegin() const
{
  return m_LabelContainer.begin();
}

mitk::LabelSet::LabelContainerIteratorType mitk::LabelSet::IteratorEnd()
{
  return m_LabelContainer.end();
}

mitk::LabelSet::LabelContainerIteratorType mitk::LabelSet::IteratorBegin()
{
  return m_LabelContainer.begin();
}

unsigned int mitk::LabelSet::GetNumberOfLabels() const
{
  return m_LabelContainer.size();
}

void mitk::LabelSet::SetLayer(unsigned int layer)
{
  m_Layer = layer;
  Modified();
}

void mitk::LabelSet::SetActiveLabel(PixelType pixelValue)
{
  m_ActiveLabelValue = pixelValue;
  ActiveLabelEvent.Send(pixelValue);
  Modified();
}

bool mitk::LabelSet::ExistLabel(PixelType pixelValue)
{
  return m_LabelContainer.count(pixelValue) > 0 ? true : false;
}

void mitk::LabelSet::AddLabel(mitk::Label *label, bool addAsClone)
{
  unsigned int max_size = mitk::Label::MAX_LABEL_VALUE + 1;
  if (m_LabelContainer.size() >= max_size)
    return;

  mitk::Label::Pointer newLabel = addAsClone ? label->Clone() : label;

  // TODO use layer of label parameter
  newLabel->SetLayer(m_Layer);

  PixelType pixelValue = newLabel->GetValue();
  if (!m_LabelContainer.empty())
  {
    auto usedValues = m_ReservedLabelValuesFunctor();
    auto finding = std::find(usedValues.begin(), usedValues.end(), pixelValue);

    if (!usedValues.empty() && usedValues.end() != finding)
    {
      pixelValue = usedValues.back()+1;
      MITK_DEBUG << "LabelSet label collision. Tried to add a label with a value already in use. Value will be adapted. Old value: " << newLabel->GetValue() << "; new value: " << pixelValue;
      newLabel->SetValue(pixelValue);
    }
  }

  // new map entry
  m_LabelContainer[pixelValue] = newLabel;
  UpdateLookupTable(pixelValue);

  // add DICOM information of the label
  DICOMSegmentationPropertyHelper::SetDICOMSegmentProperties(newLabel);

  itk::MemberCommand<LabelSet>::Pointer command = itk::MemberCommand<LabelSet>::New();
  command->SetCallbackFunction(this, &LabelSet::OnLabelModified);
  newLabel->AddObserver(itk::ModifiedEvent(), command);

  SetActiveLabel(newLabel->GetValue());
  AddLabelEvent.Send(newLabel->GetValue());
  Modified();
}

void mitk::LabelSet::AddLabel(const std::string &name, const mitk::Color &color)
{
  mitk::Label::Pointer newLabel = mitk::Label::New();
  newLabel->SetName(name);
  newLabel->SetColor(color);
  AddLabel(newLabel);
}

void mitk::LabelSet::RenameLabel(PixelType pixelValue, const std::string &name, const mitk::Color &color)
{
  mitk::Label *label = GetLabel(pixelValue);
  label->SetName(name);
  label->SetColor(color);

  // change DICOM information of the label
  DICOMSegmentationPropertyHelper::SetDICOMSegmentProperties(label);
}

void mitk::LabelSet::SetLookupTable(mitk::LookupTable *lut)
{
  m_LookupTable = lut;
  Modified();
}

void mitk::LabelSet::PrintSelf(std::ostream & /*os*/, itk::Indent /*indent*/) const
{
}

void mitk::LabelSet::RemoveLabel(PixelType pixelValue)
{
  auto it = m_LabelContainer.rbegin();
  PixelType nextActivePixelValue = it->first;

  for (; it != m_LabelContainer.rend(); ++it)
  {
    if (it->first == pixelValue)
    {
      it->second->RemoveAllObservers();
      m_LabelContainer.erase(pixelValue);
      break;
    }
    nextActivePixelValue = it->first;
  }

  if (m_ActiveLabelValue == pixelValue)
  {
    if (ExistLabel(nextActivePixelValue))
      SetActiveLabel(nextActivePixelValue);
    else
      SetActiveLabel(m_LabelContainer.rbegin()->first);
  }

  RemoveLabelEvent.Send(pixelValue);

  Modified();
}

void mitk::LabelSet::RemoveAllLabels()
{
  auto _it = IteratorBegin();
  for (; _it != IteratorConstEnd();)
  {
    auto labelValue = _it->first;
    m_LabelContainer.erase(_it++);
    RemoveLabelEvent.Send(labelValue);
  }
  AllLabelsModifiedEvent.Send();
}

void mitk::LabelSet::SetNextActiveLabel()
{
  auto it = m_LabelContainer.find(m_ActiveLabelValue);

  if (it != m_LabelContainer.end())
    ++it;

  if (it == m_LabelContainer.end())
  {
    it = m_LabelContainer.begin();
    if (m_LabelContainer.size() > 1)
      ++it; // ...skip background label!
  }

  SetActiveLabel(it->first);
}

void mitk::LabelSet::SetAllLabelsLocked(bool value)
{
  auto _end = m_LabelContainer.end();
  auto _it = m_LabelContainer.begin();
  for (; _it != _end; ++_it)
    _it->second->SetLocked(value);
  AllLabelsModifiedEvent.Send();
  Modified();
}

void mitk::LabelSet::SetAllLabelsVisible(bool value)
{
  auto _end = m_LabelContainer.end();
  auto _it = m_LabelContainer.begin();
  for (; _it != _end; ++_it)
  {
    _it->second->SetVisible(value);
    UpdateLookupTable(_it->first);
  }
  AllLabelsModifiedEvent.Send();
  Modified();
}

void mitk::LabelSet::UpdateLookupTable(PixelType pixelValue)
{
  const mitk::Color &color = GetLabel(pixelValue)->GetColor();

  double rgba[4];
  m_LookupTable->GetTableValue(static_cast<int>(pixelValue), rgba);
  rgba[0] = color.GetRed();
  rgba[1] = color.GetGreen();
  rgba[2] = color.GetBlue();
  if (GetLabel(pixelValue)->GetVisible())
    rgba[3] = GetLabel(pixelValue)->GetOpacity();
  else
    rgba[3] = 0.0;
  m_LookupTable->SetTableValue(static_cast<int>(pixelValue), rgba);
}

mitk::Label *mitk::LabelSet::GetLabel(PixelType pixelValue)
{
  if (m_LabelContainer.find(pixelValue) == m_LabelContainer.end())
    return nullptr;
  return m_LabelContainer[pixelValue];
}

const mitk::Label *mitk::LabelSet::GetLabel(PixelType pixelValue) const
{
  auto it = m_LabelContainer.find(pixelValue);
  if (it == m_LabelContainer.end())
    return nullptr;
  return it->second.GetPointer();
}

bool mitk::Equal(const mitk::LabelSet &leftHandSide, const mitk::LabelSet &rightHandSide, ScalarType eps, bool verbose)
{
  bool returnValue = true;
  // LabelSetmembers

  MITK_INFO(verbose) << "--- LabelSet Equal ---";

  // m_LookupTable;
  const mitk::LookupTable *lhsLUT = leftHandSide.GetLookupTable();
  const mitk::LookupTable *rhsLUT = rightHandSide.GetLookupTable();

  returnValue = *lhsLUT == *rhsLUT;
  if (!returnValue)
  {
    MITK_INFO(verbose) << "Lookup tabels not equal.";
    return returnValue;
    ;
  }

  // m_ActiveLabel;
  returnValue = mitk::Equal(*leftHandSide.GetActiveLabel(), *rightHandSide.GetActiveLabel(), eps, verbose);
  if (!returnValue)
  {
    MITK_INFO(verbose) << "Active label not equal.";
    return returnValue;
    ;
  }

  // m_Layer;
  returnValue = leftHandSide.GetLayer() == rightHandSide.GetLayer();
  if (!returnValue)
  {
    MITK_INFO(verbose) << "Layer index not equal.";
    return returnValue;
    ;
  }

  // container size;
  returnValue = leftHandSide.GetNumberOfLabels() == rightHandSide.GetNumberOfLabels();
  if (!returnValue)
  {
    MITK_INFO(verbose) << "Number of labels not equal.";
    return returnValue;
    ;
  }

  // Label container (map)

  // m_LabelContainer;
  auto lhsit = leftHandSide.IteratorConstBegin();
  auto rhsit = rightHandSide.IteratorConstBegin();
  for (; lhsit != leftHandSide.IteratorConstEnd(); ++lhsit, ++rhsit)
  {
    returnValue = rhsit->first == lhsit->first;
    if (!returnValue)
    {
      MITK_INFO(verbose) << "Label in label container not equal.";
      return returnValue;
      ;
    }

    returnValue = mitk::Equal(*(rhsit->second), *(lhsit->second), eps, verbose);
    if (!returnValue)
    {
      MITK_INFO(verbose) << "Label in label container not equal.";
      return returnValue;
      ;
    }
  }

  return returnValue;
}

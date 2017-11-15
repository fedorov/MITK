/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include "QmitkInteractiveTransformationWidget.h"

// mitk includes
#include "mitkRenderingManager.h"
#include "mitkBaseRenderer.h"
#include "mitkNavigationData.h"

// vtk includes
#include "vtkMatrix4x4.h"
#include "vtkLinearTransform.h"

const std::string QmitkInteractiveTransformationWidget::VIEW_ID = "org.mitk.views.interactivetransformationwidget";

QmitkInteractiveTransformationWidget::QmitkInteractiveTransformationWidget(QWidget* parent, Qt::WindowFlags f)
  : QWidget(parent, f), m_Controls(nullptr), m_Geometry(nullptr)
{
  CreateQtPartControl(this);
  CreateConnections();

  m_ResetGeometry = mitk::Geometry3D::New();
}

QmitkInteractiveTransformationWidget::~QmitkInteractiveTransformationWidget()
{
}

void QmitkInteractiveTransformationWidget::CreateQtPartControl(QWidget *parent)
{
  if (!m_Controls)
  {
    // create GUI widgets
    m_Controls = new Ui::QmitkInteractiveTransformationWidgetControls;
    m_Controls->setupUi(parent);
  }
}

void QmitkInteractiveTransformationWidget::CreateConnections()
{
  if (m_Controls)
  {
    // translations
    connect(m_Controls->m_XTransSpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &QmitkInteractiveTransformationWidget::OnXTranslationValueChanged);
    connect(m_Controls->m_XTransSlider, &QSlider::valueChanged, this, &QmitkInteractiveTransformationWidget::OnXTranslationValueChanged);

    connect(m_Controls->m_YTransSpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &QmitkInteractiveTransformationWidget::OnYTranslationValueChanged);
    connect(m_Controls->m_YTransSlider, &QSlider::valueChanged, this, &QmitkInteractiveTransformationWidget::OnYTranslationValueChanged);

    connect(m_Controls->m_ZTransSpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &QmitkInteractiveTransformationWidget::OnZTranslationValueChanged);
    connect(m_Controls->m_ZTransSlider, &QSlider::valueChanged, this, &QmitkInteractiveTransformationWidget::OnZTranslationValueChanged);

    // rotations
    connect(m_Controls->m_XRotSpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &QmitkInteractiveTransformationWidget::OnXRotationValueChanged);
    connect(m_Controls->m_XRotSlider, &QSlider::valueChanged, this, &QmitkInteractiveTransformationWidget::OnXRotationValueChanged);

    connect(m_Controls->m_YRotSpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &QmitkInteractiveTransformationWidget::OnYRotationValueChanged);
    connect(m_Controls->m_YRotSlider, &QSlider::valueChanged, this, &QmitkInteractiveTransformationWidget::OnYRotationValueChanged);

    connect(m_Controls->m_ZRotSpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &QmitkInteractiveTransformationWidget::OnZRotationValueChanged);
    connect(m_Controls->m_ZRotSlider, &QSlider::valueChanged, this, &QmitkInteractiveTransformationWidget::OnZRotationValueChanged);

    connect((QObject*)(m_Controls->m_ResetPB), SIGNAL(clicked()), this, SLOT(OnResetGeometryToIdentity()));
    connect((QObject*)(m_Controls->m_RevertChanges), SIGNAL(clicked()), this, SLOT(OnRevertChanges()));
    connect((QObject*)(m_Controls->m_UseManipulatedToolTipPB), SIGNAL(clicked()), this, SLOT(OnApplyManipulatedToolTip()));
    connect((QObject*)(m_Controls->m_Cancel), SIGNAL(clicked()), this, SLOT(OnCancel()));
  }
}

void QmitkInteractiveTransformationWidget::SetToolToEdit(const mitk::NavigationTool::Pointer _tool)
{
  m_ToolToEdit = _tool->Clone();
  mitk::BaseRenderer::GetInstance(mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget4"))->GetDataStorage()
    ->Add(m_ToolToEdit->GetDataNode());
  m_ToolToEdit->GetDataNode()->SetName("Tool Tip to be edited");

  //change color to red
  m_ToolToEdit->GetDataNode()->SetProperty("color", mitk::ColorProperty::New(1, 0, 0));

  //use the set-fuction via vtk matrix, 'cause this garantees a deep copy and not just sharing a pointer.
  m_Geometry = m_ToolToEdit->GetDataNode()->GetData()->GetGeometry();
  m_ResetGeometry->SetIndexToWorldTransformByVtkMatrix(m_Geometry->GetVtkMatrix()); //Remember the original values to be able to reset and abort everything
}

void QmitkInteractiveTransformationWidget::SetDefaultOffset(const mitk::Point3D _defaultValues)
{
  m_Geometry->SetOrigin(_defaultValues);
  m_ResetGeometry->SetOrigin(_defaultValues); //Remember the original values to be able to reset and abort everything
  SetValuesToGUI(m_Geometry->GetIndexToWorldTransform());
}

void QmitkInteractiveTransformationWidget::SetDefaultRotation(const mitk::Quaternion _defaultValues)
{
  // Conversion to navigation data / transform
  mitk::NavigationData::Pointer rotationTransform = mitk::NavigationData::New(m_Geometry->GetIndexToWorldTransform());
  rotationTransform->SetOrientation(_defaultValues);
  m_Geometry->SetIndexToWorldTransform(rotationTransform->GetAffineTransform3D());

  //For ResetGeometry, use the set-fuction via vtk matrix, 'cause this garantees a deep copy and not just sharing a pointer.
  m_ResetGeometry->SetIndexToWorldTransformByVtkMatrix(m_Geometry->GetVtkMatrix()); //Remember the original values to be able to reset and abort everything
  SetValuesToGUI(m_Geometry->GetIndexToWorldTransform());
}

void QmitkInteractiveTransformationWidget::SetValuesToGUI(const mitk::AffineTransform3D::Pointer _defaultValues)
{

  //Set toolTip values in gui
  m_Controls->m_XTransSlider->setValue(_defaultValues->GetOffset()[0]);
  m_Controls->m_YTransSlider->setValue(_defaultValues->GetOffset()[1]);
  m_Controls->m_ZTransSlider->setValue(_defaultValues->GetOffset()[2]);

  //first: some conversion
  mitk::NavigationData::Pointer transformConversionHelper = mitk::NavigationData::New(_defaultValues);
  double eulerAlphaDegrees = transformConversionHelper->GetOrientation().rotation_euler_angles()[0] / vnl_math::pi * 180;
  double eulerBetaDegrees = transformConversionHelper->GetOrientation().rotation_euler_angles()[1] / vnl_math::pi * 180;
  double eulerGammaDegrees = transformConversionHelper->GetOrientation().rotation_euler_angles()[2] / vnl_math::pi * 180;

  m_Controls->m_XRotSpinBox->setValue(eulerAlphaDegrees);
  m_Controls->m_YRotSpinBox->setValue(eulerBetaDegrees);
  m_Controls->m_ZRotSpinBox->setValue(eulerGammaDegrees);

  //Update view
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Section to allow interactive positioning of the moving surface
/////////////////////////////////////////////////////////////////////////////////////////////

void QmitkInteractiveTransformationWidget::OnXTranslationValueChanged(double v)
{
  this->blockSignals(true);//block signals to avoid loop between slider and spinbox. Unblock at the end of the function!

  //Set values to member variable
  mitk::Point3D translationParams = m_Geometry->GetOrigin();
  translationParams[0] = v;
  m_Geometry->SetOrigin(translationParams);

  //Update Gui (change may come either from spin box or from slider)
  m_Controls->m_XTransSlider->setValue(v);
  m_Controls->m_XTransSpinBox->setValue(v);

  //Update view
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
  this->blockSignals(false);//unblock signals. See above, don't remove this line. Unblock at the end of the function!
}

void QmitkInteractiveTransformationWidget::OnYTranslationValueChanged(double v)
{
  this->blockSignals(true);//block signals to avoid loop between slider and spinbox. Unblock at the end of the function!

  //Set values to member variable
  mitk::Point3D translationParams = m_Geometry->GetOrigin();
  translationParams[1] = v;
  m_Geometry->SetOrigin(translationParams);

  //Update Gui (change may come either from spin box or from slider)
  m_Controls->m_YTransSlider->setValue(v);
  m_Controls->m_YTransSpinBox->setValue(v);

  //Update view
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
  this->blockSignals(false);//unblock signals. See above, don't remove this line. Unblock at the end of the function!
}

void QmitkInteractiveTransformationWidget::OnZTranslationValueChanged(double v)
{
  this->blockSignals(true);//block signals to avoid loop between slider and spinbox. Unblock at the end of the function!

  //Set values to member variable
  mitk::Point3D translationParams = m_Geometry->GetOrigin();
  translationParams[2] = v;
  m_Geometry->SetOrigin(translationParams);

  //Update Gui (change may come either from spin box or from slider)
  m_Controls->m_ZTransSlider->setValue(v);
  m_Controls->m_ZTransSpinBox->setValue(v);

  //Update view
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
  this->blockSignals(false);//unblock signals. See above, don't remove this line. Unblock at the end of the function!
}

void QmitkInteractiveTransformationWidget::OnXRotationValueChanged(double v)
{
  this->blockSignals(true);//block signals to avoid loop between slider and spinbox. Unblock at the end of the function!
  mitk::Vector3D rotationParams;
  rotationParams[0] = v;
  rotationParams[1] = m_Controls->m_YRotSlider->value();
  rotationParams[2] = m_Controls->m_ZRotSlider->value();

  //Update Gui (change may come either from spin box or from slider)
  m_Controls->m_XRotSlider->setValue(v);
  m_Controls->m_XRotSpinBox->setValue(v);

  this->Rotate(rotationParams);
  this->blockSignals(false);//unblock signals. See above, don't remove this line. Unblock at the end of the function!
}

void QmitkInteractiveTransformationWidget::OnYRotationValueChanged(double v)
{
  this->blockSignals(true);//block signals to avoid loop between slider and spinbox. Unblock at the end of the function!
  mitk::Vector3D rotationParams;
  rotationParams[0] = m_Controls->m_XRotSlider->value();
  rotationParams[1] = v;
  rotationParams[2] = m_Controls->m_ZRotSlider->value();

  //Update Gui (change may come either from spin box or from slider)
  m_Controls->m_YRotSlider->setValue(v);
  m_Controls->m_YRotSpinBox->setValue(v);

  this->Rotate(rotationParams);
  this->blockSignals(false);//unblock signals. See above, don't remove this line. Unblock at the end of the function!
}

void QmitkInteractiveTransformationWidget::OnZRotationValueChanged(double v)
{
  this->blockSignals(true);//block signals to avoid loop between slider and spinbox. Unblock at the end of the function!
  mitk::Vector3D rotationParams;
  rotationParams[0] = m_Controls->m_XRotSlider->value();
  rotationParams[1] = m_Controls->m_YRotSlider->value();
  rotationParams[2] = v;

  //Update Gui (change may come either from spin box or from slider)
  m_Controls->m_ZRotSlider->setValue(v);
  m_Controls->m_ZRotSpinBox->setValue(v);

  this->Rotate(rotationParams);
  this->blockSignals(false);//unblock signals. See above, don't remove this line. Unblock at the end of the function!
}

void QmitkInteractiveTransformationWidget::Rotate(mitk::Vector3D rotateVector)
{
  //0: from degrees to radians
  double radianX = rotateVector[0] * vnl_math::pi / 180;
  double radianY = rotateVector[1] * vnl_math::pi / 180;
  double radianZ = rotateVector[2] * vnl_math::pi / 180;

  //1: from euler angles to quaternion
  mitk::Quaternion rotation(radianX, radianY, radianZ);

  //2: Conversion to navigation data / transform
  mitk::NavigationData::Pointer rotationTransform = mitk::NavigationData::New(m_Geometry->GetIndexToWorldTransform());
  rotationTransform->SetOrientation(rotation);

  m_Geometry->SetIndexToWorldTransform(rotationTransform->GetAffineTransform3D());

  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void QmitkInteractiveTransformationWidget::OnResetGeometryToIdentity()
{
  // reset the input to its initial state.
  m_Geometry->SetIdentity();

  //Update Sliders
  this->SetValuesToGUI(m_Geometry->GetIndexToWorldTransform());
  //Refresh view
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void QmitkInteractiveTransformationWidget::OnRevertChanges()
{
  // reset the input to its initial state.
  m_Geometry->SetIndexToWorldTransformByVtkMatrix(m_ResetGeometry->GetVtkMatrix());

  //Update Sliders
  this->SetValuesToGUI(m_Geometry->GetIndexToWorldTransform());
  //Refresh view
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void QmitkInteractiveTransformationWidget::OnApplyManipulatedToolTip()
{
  mitk::BaseRenderer::GetInstance(mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget4"))->GetDataStorage()
    ->Remove(m_ToolToEdit->GetDataNode());

  mitk::AffineTransform3D::Pointer toolTip = m_Geometry->GetIndexToWorldTransform();
  emit EditToolTipFinished(toolTip);
}

void QmitkInteractiveTransformationWidget::OnCancel()
{
  mitk::BaseRenderer::GetInstance(mitk::BaseRenderer::GetRenderWindowByName("stdmulti.widget4"))->GetDataStorage()
    ->Remove(m_ToolToEdit->GetDataNode());

  emit EditToolTipFinished(nullptr);
}
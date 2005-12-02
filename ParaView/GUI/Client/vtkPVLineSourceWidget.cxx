/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLineSourceWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLineSourceWidget.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDataInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSM3DWidgetProxy.h"
#include "vtkPVWindow.h"
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLineSourceWidget);
vtkCxxRevisionMacro(vtkPVLineSourceWidget, "1.37");

vtkCxxSetObjectMacro(vtkPVLineSourceWidget, InputMenu, vtkPVInputMenu);

//----------------------------------------------------------------------------
vtkPVLineSourceWidget::vtkPVLineSourceWidget()
{
  this->SourceProxy = 0;
  this->InputMenu = NULL;
}

//----------------------------------------------------------------------------
vtkPVLineSourceWidget::~vtkPVLineSourceWidget()
{
  if(this->SourceProxy)
    {
    vtkSMProxyManager* proxyM = vtkSMObject::GetProxyManager();
    const char* proxyName = proxyM->GetProxyName("sources", this->SourceProxy);
    if (proxyName)
      {
      proxyM->UnRegisterProxy("sources", proxyName);
      }
    this->DisableAnimation();
    this->SourceProxy->Delete();
    this->SourceProxy = 0;
    }
  this->SetInputMenu(NULL);
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::EnableAnimation()
{
  vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
  if (this->PVSource && this->SourceProxy)
    {
    vtkSMSourceProxy* sproxy = this->PVSource->GetProxy();
    if (sproxy)
      {
      const char* root = pm->GetProxyName("animateable", sproxy);
      if (root)
        {
        ostrstream animName;
        animName << root << ".LineSource" << ends;
        pm->RegisterProxy("animateable", animName.str(), this->SourceProxy);
        delete[] animName.str();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::DisableAnimation()
{
  if (this->SourceProxy)
    {
    vtkSMProxyManager* proxyM = vtkSMObject::GetProxyManager();
    const char* proxyName = proxyM->GetProxyName("animateable", this->SourceProxy);
    if (proxyName)
      {
      proxyM->UnRegisterProxy("animateable", proxyName);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Create()
{
  static int proxyNum = 0;
  vtkSMProxyManager *pm = vtkSMObject::GetProxyManager();
  this->SourceProxy = vtkSMSourceProxy::SafeDownCast(
    pm->NewProxy("sources", "LineSource"));
  ostrstream str;
  str << "LineSource" << proxyNum << ends;
  pm->RegisterProxy("sources", str.str(), this->SourceProxy);
  proxyNum++;
  delete[] str.str();

  // Call the superclass to create the widget and set the appropriate flags
  this->Superclass::Create();

  // Set up controller properties. Controller properties are set so 
  // that in the SM State, we can have a mapping from the widget to the 
  // controlled implicit function.
  vtkSMProperty* p = this->SourceProxy->GetProperty("Point1");
  p->SetControllerProxy(this->WidgetProxy);
  p->SetControllerProperty(this->WidgetProxy->GetProperty("Point1"));

  p = this->SourceProxy->GetProperty("Point2");
  p->SetControllerProxy(this->WidgetProxy);
  p->SetControllerProperty(this->WidgetProxy->GetProperty("Point2"));
  
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Initialize()
{
  
  this->PlaceWidget();

  this->Accept();
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::ResetInternal()
{
  if ( !this->ModifiedFlag)
    {
    return;
    }
  vtkSMDoubleVectorProperty *pt1p = vtkSMDoubleVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("Point1"));
  vtkSMDoubleVectorProperty *pt2p = vtkSMDoubleVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("Point2"));
  vtkSMIntVectorProperty *resp = vtkSMIntVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("Resolution"));
  if (pt1p)
    {
    this->SetPoint1Internal(pt1p->GetElement(0), pt1p->GetElement(1),
      pt1p->GetElement(2));
    }
  if (pt2p)
    {
    this->SetPoint2Internal(pt2p->GetElement(0), pt2p->GetElement(1),
      pt2p->GetElement(2));
    }
  if (resp)
    {
    this->SetResolution(resp->GetElement(0));
    }
  this->ModifiedFlag = 0;

  this->Render();
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Accept()
{
  int modFlag = this->GetModifiedFlag();

  if (modFlag)
    {
    vtkSMDoubleVectorProperty *pt1p = vtkSMDoubleVectorProperty::SafeDownCast(
      this->SourceProxy->GetProperty("Point1"));
    vtkSMDoubleVectorProperty *pt2p = vtkSMDoubleVectorProperty::SafeDownCast(
      this->SourceProxy->GetProperty("Point2"));
    vtkSMIntVectorProperty *resp = vtkSMIntVectorProperty::SafeDownCast(
      this->SourceProxy->GetProperty("Resolution"));
    double pt[3];
    this->WidgetProxy->UpdatePropertyInformation();
    if (pt1p)
      {
      this->GetPoint1Internal(pt);
      pt1p->SetElement(0, pt[0]);
      pt1p->SetElement(1, pt[1]);
      pt1p->SetElement(2, pt[2]);
      }
    if (pt2p)
      {
      this->GetPoint2Internal(pt);
      pt2p->SetElement(0, pt[0]);
      pt2p->SetElement(1, pt[1]);
      pt2p->SetElement(2, pt[2]);
      }
    if (resp)
      {
      resp->SetElement(0, this->GetResolutionInternal());
      }
    this->SourceProxy->UpdateVTKObjects();
    this->SourceProxy->UpdatePipeline();
    }
  // 3DWidgets need to explictly call UpdateAnimationInterface on accept
  // since the animatable proxies might have been registered/unregistered
  // which needs to be updated in the Animation interface.
  this->GetPVApplication()->GetMainWindow()->UpdateAnimationInterface();
  // I actually want to call vtkPVWidget::Accept, not the Accept method of
  // the superclass (vtkPVLineWidget).
  this->vtkPVWidget::Accept();
}

//-----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Update()
{
  if (this->InputMenu)
    {
    vtkPVSource *input = this->InputMenu->GetCurrentValue();
    if (input)
      {
      double bds[6];
      double x, y, z;
      input->GetDataInformation()->GetBounds(bds);
      x = (bds[0]+bds[1])/2; 
      y = bds[2]; 
      z = (bds[4]+bds[5])/2;
      this->SetPoint1(x, y, z);
      x = (bds[0]+bds[1])/2; 
      y = bds[3]; 
      z = (bds[4]+bds[5])/2;
      this->SetPoint2(x, y, z);
      this->PlaceWidget(bds);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::SaveInBatchScript(ofstream *file)
{
  if (!this->SourceProxy)
  {
  vtkErrorMacro("Source proxy must be set to save to a batch script.");
  return;
  }
  
  vtkClientServerID sourceID = this->SourceProxy->GetID(0);
  if (sourceID.ID == 0)
  {
  vtkErrorMacro("Sanity check failed. " << this->GetClassName());
  return;
  } 

  this->WidgetProxy->SaveInBatchScript(file);
  *file << endl;
  *file << "set pvTemp" << sourceID
    << " [$proxyManager NewProxy sources LineSource]" << endl;
  *file << "  $proxyManager RegisterProxy sources pvTemp"
    << sourceID << " $pvTemp" << sourceID << endl;
  *file << "  $pvTemp" << sourceID << " UnRegister {}" << endl;

  
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("Point1"));
  if(dvp)
    {
    *file << "  [$pvTemp" << sourceID << " GetProperty Point1] "
      << "SetElements3 " 
      << dvp->GetElement(0) << " " 
      << dvp->GetElement(1) << " " 
      << dvp->GetElement(2) << endl;
    *file << "  [$pvTemp" << sourceID << " GetProperty Point1]"
      << " SetControllerProxy $pvTemp" << this->WidgetProxy->GetID(0)
      << endl;
    *file << " [$pvTemp" << sourceID << " GetProperty Point1]"
      << " SetControllerProperty [$pvTemp" << this->WidgetProxy->GetID(0)
      << " GetProperty Point1]" << endl;
    }
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("Point2"));
  if (dvp)
    {
    *file << "  [$pvTemp" << sourceID << " GetProperty Point2] "
      << "SetElements3 " 
      << dvp->GetElement(0) << " " 
      << dvp->GetElement(1) << " " 
      << dvp->GetElement(2) << endl;
    *file << "  [$pvTemp" << sourceID << " GetProperty Point2]"
      << " SetControllerProxy $pvTemp" << this->WidgetProxy->GetID(0)
      << endl;
    *file << " [$pvTemp" << sourceID << " GetProperty Point2]"
      << " SetControllerProperty [$pvTemp" << this->WidgetProxy->GetID(0)
      << " GetProperty Point2]" << endl;
    }
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("Resolution"));
  if(ivp)
    {
    *file << "  [$pvTemp" << sourceID << " GetProperty Resolution] "
      << "SetElements1 " << ivp->GetElement(0) << endl;
    *file << "  [$pvTemp" << sourceID << " GetProperty Resolution]"
      << " SetControllerProxy $pvTemp" << this->WidgetProxy->GetID(0)
      << endl;
    *file << " [$pvTemp" << sourceID << " GetProperty Resolution]"
      << " SetControllerProperty [$pvTemp" << this->WidgetProxy->GetID(0)
      << " GetProperty Resolution]" << endl;
    }
  *file << "  $pvTemp" << sourceID << " UpdateVTKObjects" << endl;
  *file << endl;
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SourceProxy: " << this->SourceProxy << endl;
}

//-----------------------------------------------------------------------------
void vtkPVLineSourceWidget::CopyProperties(
  vtkPVWidget *clone, vtkPVSource *pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVLineSourceWidget *psw = vtkPVLineSourceWidget::SafeDownCast(clone);
  if (psw)
    {
    if (this->InputMenu)
      {
      vtkPVInputMenu *im = this->InputMenu->ClonePrototype(pvSource, map);
      psw->SetInputMenu(im);
      im->Delete();
      }
    }
}

//-----------------------------------------------------------------------------
int vtkPVLineSourceWidget::ReadXMLAttributes(vtkPVXMLElement *element,
  vtkPVXMLPackageParser *parser)
{
  if (!this->Superclass::ReadXMLAttributes(element, parser))
    {
    return 0;
    }

  const char *input_menu = element->GetAttribute("input_menu");
  if (input_menu)
    {
    vtkPVXMLElement *ime = element->LookupElement(input_menu);
    if (!ime)
      {
      vtkErrorMacro("Couldn't find InputMenu element " << input_menu);
      return 0;
      }

    vtkPVWidget *w = this->GetPVWidgetFromParser(ime, parser);
    vtkPVInputMenu *imw = vtkPVInputMenu::SafeDownCast(w);
    if (!imw)
      {
      if (w)
        {
        w->Delete();
        }
      vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
      return 0;
      }
    imw->AddDependent(this);
    this->SetInputMenu(imw);
    imw->Delete();
    }

  return 1;
}

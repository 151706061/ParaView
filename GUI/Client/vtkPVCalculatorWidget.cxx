/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCalculatorWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCalculatorWidget.h"

#include "vtkArrayCalculator.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVStringAndScalarListWidgetProperty.h"
#include "vtkPVWidgetCollection.h"
#include "vtkPVWindow.h"
#include "vtkSource.h"
#include "vtkStringList.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCalculatorWidget);
vtkCxxRevisionMacro(vtkPVCalculatorWidget, "1.23");

int vtkPVCalculatorWidgetCommand(ClientData cd, Tcl_Interp *interp,
                                int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVCalculatorWidget::vtkPVCalculatorWidget()
{
  this->CommandFunction = vtkPVCalculatorWidgetCommand;
  
  this->AttributeModeFrame = vtkKWWidget::New();
  this->AttributeModeLabel = vtkKWLabel::New();
  this->AttributeModeMenu = vtkKWOptionMenu::New();
  
  this->CalculatorFrame = vtkKWLabeledFrame::New();
  this->FunctionLabel = vtkKWEntry::New();

  this->ButtonClear = vtkKWPushButton::New();
  this->ButtonZero = vtkKWPushButton::New();
  this->ButtonOne = vtkKWPushButton::New();
  this->ButtonTwo = vtkKWPushButton::New();
  this->ButtonThree = vtkKWPushButton::New();
  this->ButtonFour = vtkKWPushButton::New();
  this->ButtonFive = vtkKWPushButton::New();
  this->ButtonSix = vtkKWPushButton::New();
  this->ButtonSeven = vtkKWPushButton::New();
  this->ButtonEight = vtkKWPushButton::New();
  this->ButtonNine = vtkKWPushButton::New();
  this->ButtonDivide = vtkKWPushButton::New();
  this->ButtonMultiply = vtkKWPushButton::New();
  this->ButtonSubtract = vtkKWPushButton::New();
  this->ButtonAdd = vtkKWPushButton::New();
  this->ButtonDecimal = vtkKWPushButton::New();
  this->ButtonDot = vtkKWPushButton::New();
  this->ButtonSin = vtkKWPushButton::New();
  this->ButtonCos = vtkKWPushButton::New();
  this->ButtonTan = vtkKWPushButton::New();
  this->ButtonASin = vtkKWPushButton::New();
  this->ButtonACos = vtkKWPushButton::New();
  this->ButtonATan = vtkKWPushButton::New();
  this->ButtonSinh = vtkKWPushButton::New();
  this->ButtonCosh = vtkKWPushButton::New();
  this->ButtonTanh = vtkKWPushButton::New();
  this->ButtonPow = vtkKWPushButton::New();
  this->ButtonSqrt = vtkKWPushButton::New();
  this->ButtonExp = vtkKWPushButton::New();
  this->ButtonCeiling = vtkKWPushButton::New();
  this->ButtonFloor = vtkKWPushButton::New();
  this->ButtonLog = vtkKWPushButton::New();
  this->ButtonAbs = vtkKWPushButton::New();
  this->ButtonMag = vtkKWPushButton::New();
  this->ButtonNorm = vtkKWPushButton::New();
  this->ButtonIHAT = vtkKWPushButton::New();
  this->ButtonJHAT = vtkKWPushButton::New();
  this->ButtonKHAT = vtkKWPushButton::New();
  this->ButtonLeftParenthesis = vtkKWPushButton::New();
  this->ButtonRightParenthesis = vtkKWPushButton::New();
  this->ScalarsMenu = vtkKWMenuButton::New();
  this->VectorsMenu = vtkKWMenuButton::New();
  
  this->ScalarArrayNames = 0;
  this->ScalarVariableNames = 0;
  this->ScalarComponents = 0;
  this->NumberOfScalarVariables = 0;
  this->VectorArrayNames = 0;
  this->VectorVariableNames = 0;
  this->NumberOfVectorVariables = 0;
  
  this->Property = NULL;
}

//----------------------------------------------------------------------------
vtkPVCalculatorWidget::~vtkPVCalculatorWidget()
{
  this->AttributeModeLabel->Delete();
  this->AttributeModeLabel = NULL;
  this->AttributeModeMenu->Delete();
  this->AttributeModeMenu = NULL;
  this->AttributeModeFrame->Delete();
  this->AttributeModeFrame = NULL;
  
  this->FunctionLabel->Delete();
  this->FunctionLabel = NULL;
  
  this->ButtonClear->Delete();
  this->ButtonClear = NULL;
  this->ButtonZero->Delete();
  this->ButtonZero = NULL;
  this->ButtonOne->Delete();
  this->ButtonOne = NULL;
  this->ButtonTwo->Delete();
  this->ButtonTwo = NULL;
  this->ButtonThree->Delete();
  this->ButtonThree = NULL;
  this->ButtonFour->Delete();
  this->ButtonFour = NULL;
  this->ButtonFive->Delete();
  this->ButtonFive = NULL;
  this->ButtonSix->Delete();
  this->ButtonSix = NULL;
  this->ButtonSeven->Delete();
  this->ButtonSeven = NULL;
  this->ButtonEight->Delete();
  this->ButtonEight = NULL;
  this->ButtonNine->Delete();
  this->ButtonNine = NULL;
  this->ButtonDivide->Delete();
  this->ButtonDivide = NULL;
  this->ButtonMultiply->Delete();
  this->ButtonMultiply = NULL;
  this->ButtonSubtract->Delete();
  this->ButtonSubtract = NULL;
  this->ButtonAdd->Delete();
  this->ButtonAdd = NULL;
  this->ButtonDecimal->Delete();
  this->ButtonDecimal = NULL;
  this->ButtonDot->Delete();
  this->ButtonDot = NULL;
  this->ButtonSin->Delete();
  this->ButtonSin = NULL;
  this->ButtonCos->Delete();
  this->ButtonCos = NULL;
  this->ButtonTan->Delete();
  this->ButtonTan = NULL;
  this->ButtonASin->Delete();
  this->ButtonASin = NULL;
  this->ButtonACos->Delete();
  this->ButtonACos = NULL;
  this->ButtonATan->Delete();
  this->ButtonATan = NULL;
  this->ButtonSinh->Delete();
  this->ButtonSinh = NULL;
  this->ButtonCosh->Delete();
  this->ButtonCosh = NULL;
  this->ButtonTanh->Delete();
  this->ButtonTanh = NULL;
  this->ButtonPow->Delete();
  this->ButtonPow = NULL;
  this->ButtonSqrt->Delete();
  this->ButtonSqrt = NULL;
  this->ButtonExp->Delete();
  this->ButtonExp = NULL;
  this->ButtonCeiling->Delete();
  this->ButtonCeiling = NULL;
  this->ButtonFloor->Delete();
  this->ButtonFloor = NULL;
  this->ButtonLog->Delete();
  this->ButtonLog = NULL;
  this->ButtonAbs->Delete();
  this->ButtonAbs = NULL;
  this->ButtonMag->Delete();
  this->ButtonMag = NULL;
  this->ButtonNorm->Delete();
  this->ButtonNorm = NULL;
  this->ButtonIHAT->Delete();
  this->ButtonIHAT = NULL;
  this->ButtonJHAT->Delete();
  this->ButtonJHAT = NULL;
  this->ButtonKHAT->Delete();
  this->ButtonKHAT = NULL;
  this->ButtonLeftParenthesis->Delete();
  this->ButtonLeftParenthesis = NULL;
  this->ButtonRightParenthesis->Delete();
  this->ButtonRightParenthesis = NULL;
  this->ScalarsMenu->Delete();
  this->ScalarsMenu = NULL;
  this->VectorsMenu->Delete();
  this->VectorsMenu = NULL;
  this->CalculatorFrame->Delete();
  this->CalculatorFrame = NULL;
  
  this->ClearAllVariables();
  
  this->SetProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);

  this->AttributeModeFrame->SetParent(this);
  this->AttributeModeFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->AttributeModeFrame->GetWidgetName());

  this->AttributeModeLabel->SetParent(this->AttributeModeFrame);
  this->AttributeModeLabel->Create(pvApp, "-width 18 -justify right");
  this->AttributeModeLabel->SetLabel("Attribute Mode");
  this->AttributeModeLabel->SetBalloonHelpString(
    "Select whether to operate on point or cell data");
  this->AttributeModeMenu->SetParent(this->AttributeModeFrame);
  this->AttributeModeMenu->Create(pvApp, "");
  this->AttributeModeMenu->AddEntryWithCommand("Point Data", this,
                                               "ChangeAttributeMode point");
  this->AttributeModeMenu->AddEntryWithCommand("Cell Data", this,
                                               "ChangeAttributeMode cell");
  this->AttributeModeMenu->SetCurrentEntry("Point Data");
  this->AttributeModeMenu->SetBalloonHelpString(
    "Select whether to operate on point or cell data");
  this->Script("pack %s %s -side left",
               this->AttributeModeLabel->GetWidgetName(),
               this->AttributeModeMenu->GetWidgetName());
  
  this->CalculatorFrame->SetParent(this);
  this->CalculatorFrame->ShowHideFrameOn();
  this->CalculatorFrame->Create(pvApp, 0);
  this->CalculatorFrame->SetLabel("Calculator");
  this->Script("pack %s -fill x -expand t -side top",
               this->CalculatorFrame->GetWidgetName());

  this->FunctionLabel->SetParent(this->CalculatorFrame->GetFrame());
  this->FunctionLabel->Create(pvApp, "");
  this->FunctionLabel->SetValue("");
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->FunctionLabel->GetWidgetName(), this->GetTclName());
  this->Script("grid %s -columnspan 8 -sticky ew", 
               this->FunctionLabel->GetWidgetName());
  
  this->ButtonClear->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonClear->Create(pvApp, "");
  this->ButtonClear->SetLabel("Clear");
  this->ButtonClear->SetCommand(this, "ClearFunction");
  this->ButtonLeftParenthesis->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonLeftParenthesis->Create(pvApp, "");
  this->ButtonLeftParenthesis->SetLabel("(");
  this->ButtonLeftParenthesis->SetCommand(this, "UpdateFunction (");
  this->ButtonRightParenthesis->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonRightParenthesis->Create(pvApp, "");
  this->ButtonRightParenthesis->SetLabel(")");
  this->ButtonRightParenthesis->SetCommand(this, "UpdateFunction )");
  this->ButtonIHAT->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonIHAT->Create(pvApp, "");
  this->ButtonIHAT->SetLabel("iHat");
  this->ButtonIHAT->SetCommand(this, "UpdateFunction iHat");
  this->ButtonJHAT->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonJHAT->Create(pvApp, "");
  this->ButtonJHAT->SetLabel("jHat");
  this->ButtonJHAT->SetCommand(this, "UpdateFunction jHat");
  this->ButtonKHAT->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonKHAT->Create(pvApp, "");
  this->ButtonKHAT->SetLabel("kHat");
  this->ButtonKHAT->SetCommand(this, "UpdateFunction kHat");
  this->Script("grid %s %s %s %s %s %s -sticky ew",
               this->ButtonClear->GetWidgetName(),
               this->ButtonLeftParenthesis->GetWidgetName(),
               this->ButtonRightParenthesis->GetWidgetName(),
               this->ButtonIHAT->GetWidgetName(),
               this->ButtonJHAT->GetWidgetName(),
               this->ButtonKHAT->GetWidgetName());
  
  this->ButtonSin->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonSin->Create(pvApp, "");
  this->ButtonSin->SetLabel("sin");
  this->ButtonSin->SetCommand(this, "UpdateFunction sin");
  this->ButtonCos->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonCos->Create(pvApp, "");
  this->ButtonCos->SetLabel("cos");
  this->ButtonCos->SetCommand(this, "UpdateFunction cos");
  this->ButtonTan->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonTan->Create(pvApp, "");
  this->ButtonTan->SetLabel("tan");
  this->ButtonTan->SetCommand(this, "UpdateFunction tan");
  this->ButtonSeven->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonSeven->Create(pvApp, "");
  this->ButtonSeven->SetLabel("7");
  this->ButtonSeven->SetCommand(this, "UpdateFunction 7");
  this->ButtonEight->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonEight->Create(pvApp, "");
  this->ButtonEight->SetLabel("8");
  this->ButtonEight->SetCommand(this, "UpdateFunction 8");
  this->ButtonNine->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonNine->Create(pvApp, "");
  this->ButtonNine->SetLabel("9");
  this->ButtonNine->SetCommand(this, "UpdateFunction 9");
  this->ButtonDivide->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonDivide->Create(pvApp, "");
  this->ButtonDivide->SetLabel("/");
  this->ButtonDivide->SetCommand(this, "UpdateFunction /");
  this->Script("grid %s %s %s %s %s %s %s -sticky ew",
               this->ButtonSin->GetWidgetName(),
               this->ButtonCos->GetWidgetName(),
               this->ButtonTan->GetWidgetName(),
               this->ButtonSeven->GetWidgetName(),
               this->ButtonEight->GetWidgetName(),
               this->ButtonNine->GetWidgetName(),
               this->ButtonDivide->GetWidgetName());
  
  this->ButtonASin->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonASin->Create(pvApp, "");
  this->ButtonASin->SetLabel("asin");
  this->ButtonASin->SetCommand(this, "UpdateFunction asin");
  this->ButtonACos->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonACos->Create(pvApp, "");
  this->ButtonACos->SetLabel("acos");
  this->ButtonACos->SetCommand(this, "UpdateFunction acos");
  this->ButtonATan->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonATan->Create(pvApp, "");
  this->ButtonATan->SetLabel("atan");
  this->ButtonATan->SetCommand(this, "UpdateFunction atan");
  this->ButtonFour->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonFour->Create(pvApp, "");
  this->ButtonFour->SetLabel("4");
  this->ButtonFour->SetCommand(this, "UpdateFunction 4");
  this->ButtonFive->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonFive->Create(pvApp, "");
  this->ButtonFive->SetLabel("5");
  this->ButtonFive->SetCommand(this, "UpdateFunction 5");
  this->ButtonSix->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonSix->Create(pvApp, "");
  this->ButtonSix->SetLabel("6");
  this->ButtonSix->SetCommand(this, "UpdateFunction 6");
  this->ButtonMultiply->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonMultiply->Create(pvApp, "");
  this->ButtonMultiply->SetLabel("*");
  this->ButtonMultiply->SetCommand(this, "UpdateFunction *");
  this->Script("grid %s %s %s %s %s %s %s -sticky ew",
               this->ButtonASin->GetWidgetName(),
               this->ButtonACos->GetWidgetName(),
               this->ButtonATan->GetWidgetName(),
               this->ButtonFour->GetWidgetName(),
               this->ButtonFive->GetWidgetName(),
               this->ButtonSix->GetWidgetName(),
               this->ButtonMultiply->GetWidgetName());
  
  this->ButtonSinh->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonSinh->Create(pvApp, "");
  this->ButtonSinh->SetLabel("sinh");
  this->ButtonSinh->SetCommand(this, "UpdateFunction sinh");
  this->ButtonCosh->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonCosh->Create(pvApp, "");
  this->ButtonCosh->SetLabel("cosh");
  this->ButtonCosh->SetCommand(this, "UpdateFunction cosh");
  this->ButtonTanh->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonTanh->Create(pvApp, "");
  this->ButtonTanh->SetLabel("tanh");
  this->ButtonTanh->SetCommand(this, "UpdateFunction tanh");
  this->ButtonOne->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonOne->Create(pvApp, "");
  this->ButtonOne->SetLabel("1");
  this->ButtonOne->SetCommand(this, "UpdateFunction 1");
  this->ButtonTwo->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonTwo->Create(pvApp, "");
  this->ButtonTwo->SetLabel("2");
  this->ButtonTwo->SetCommand(this, "UpdateFunction 2");
  this->ButtonThree->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonThree->Create(pvApp, "");
  this->ButtonThree->SetLabel("3");
  this->ButtonThree->SetCommand(this, "UpdateFunction 3");
  this->ButtonSubtract->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonSubtract->Create(pvApp, "");
  this->ButtonSubtract->SetLabel("-");
  this->ButtonSubtract->SetCommand(this, "UpdateFunction -");
  this->Script("grid %s %s %s %s %s %s %s -sticky ew",
               this->ButtonSinh->GetWidgetName(),
               this->ButtonCosh->GetWidgetName(),
               this->ButtonTanh->GetWidgetName(),
               this->ButtonOne->GetWidgetName(),
               this->ButtonTwo->GetWidgetName(),
               this->ButtonThree->GetWidgetName(),
               this->ButtonSubtract->GetWidgetName());

  this->ButtonPow->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonPow->Create(pvApp, "");
  this->ButtonPow->SetLabel("x^y");
  this->ButtonPow->SetCommand(this, "UpdateFunction ^");
  this->ButtonSqrt->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonSqrt->Create(pvApp, "");
  this->ButtonSqrt->SetLabel("sqrt");
  this->ButtonSqrt->SetCommand(this, "UpdateFunction sqrt");
  this->ButtonExp->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonExp->Create(pvApp, "");
  this->ButtonExp->SetLabel("e^x");
  this->ButtonExp->SetCommand(this, "UpdateFunction exp");
  this->ButtonLog->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonLog->Create(pvApp, "");
  this->ButtonLog->SetLabel("log");
  this->ButtonLog->SetCommand(this, "UpdateFunction log");
  this->ButtonZero->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonZero->Create(pvApp, "");
  this->ButtonZero->SetLabel("0");
  this->ButtonZero->SetCommand(this, "UpdateFunction 0");
  this->ButtonDecimal->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonDecimal->Create(pvApp, "");
  this->ButtonDecimal->SetLabel(".");
  this->ButtonDecimal->SetCommand(this, "UpdateFunction .");
  this->ButtonAdd->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonAdd->Create(pvApp, "");
  this->ButtonAdd->SetLabel("+");
  this->ButtonAdd->SetCommand(this, "UpdateFunction +");
  this->Script("grid %s %s %s %s %s %s %s -sticky ew",
               this->ButtonPow->GetWidgetName(),
               this->ButtonSqrt->GetWidgetName(),
               this->ButtonExp->GetWidgetName(),
               this->ButtonLog->GetWidgetName(),
               this->ButtonZero->GetWidgetName(),
               this->ButtonDecimal->GetWidgetName(),
               this->ButtonAdd->GetWidgetName()); 
  
  this->ButtonCeiling->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonCeiling->Create(pvApp, "");
  this->ButtonCeiling->SetLabel("ceil");
  this->ButtonCeiling->SetCommand(this, "UpdateFunction ceil");
  this->ButtonFloor->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonFloor->Create(pvApp, "");
  this->ButtonFloor->SetLabel("floor");
  this->ButtonFloor->SetCommand(this, "UpdateFunction floor");
  this->ButtonAbs->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonAbs->Create(pvApp, "");
  this->ButtonAbs->SetLabel("abs");
  this->ButtonAbs->SetCommand(this, "UpdateFunction abs");
  this->Script("grid %s %s %s -sticky ew",
               this->ButtonCeiling->GetWidgetName(),
               this->ButtonFloor->GetWidgetName(),
               this->ButtonAbs->GetWidgetName());
  
  this->ButtonDot->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonDot->Create(pvApp, "");
  this->ButtonDot->SetLabel("v1.v2");
  this->ButtonDot->SetCommand(this, "UpdateFunction .");
  this->ButtonMag->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonMag->Create(pvApp, "");
  this->ButtonMag->SetLabel("mag");
  this->ButtonMag->SetCommand(this, "UpdateFunction mag");
  this->ButtonNorm->SetParent(this->CalculatorFrame->GetFrame());
  this->ButtonNorm->Create(pvApp, "");
  this->ButtonNorm->SetLabel("norm");
  this->ButtonNorm->SetCommand(this, "UpdateFunction norm");
  this->Script("grid %s %s %s -sticky ew", 
               this->ButtonDot->GetWidgetName(),
               this->ButtonMag->GetWidgetName(), 
               this->ButtonNorm->GetWidgetName());
  
  this->ScalarsMenu->SetParent(this->CalculatorFrame->GetFrame());
  this->ScalarsMenu->Create(pvApp, "");
  this->ScalarsMenu->SetButtonText("scalars");
  this->ScalarsMenu->SetBalloonHelpString("Select a scalar array to operate on");
  this->VectorsMenu->SetParent(this->CalculatorFrame->GetFrame());
  this->VectorsMenu->Create(pvApp, "");
  this->VectorsMenu->SetButtonText("vectors");
  this->VectorsMenu->SetBalloonHelpString("Select a vector array to operate on");
  this->ChangeAttributeMode("point");
  this->Script("grid %s -row 6 -column 3 -columnspan 4 -sticky news",
               this->ScalarsMenu->GetWidgetName());
  this->Script("grid %s -row 7 -column 3 -columnspan 4 -sticky news",
               this->VectorsMenu->GetWidgetName());
  
  this->Script("grid columnconfigure %s 3 -minsize 40",
               this->CalculatorFrame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 4 -minsize 40",
               this->CalculatorFrame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 5 -minsize 40",
               this->CalculatorFrame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 6 -minsize 40",
               this->CalculatorFrame->GetFrame()->GetWidgetName());
}

void vtkPVCalculatorWidget::UpdateFunction(const char* newSymbol)
{
  char* newFunction;
  const char* currentFunction = this->FunctionLabel->GetValue();
  newFunction = new char[strlen(currentFunction)+strlen(newSymbol)+1];
  sprintf(newFunction, "%s%s", currentFunction, newSymbol);
  this->FunctionLabel->SetValue(newFunction);
  delete [] newFunction;
  this->ModifiedCallback();
}

void vtkPVCalculatorWidget::ClearFunction()
{
  this->FunctionLabel->SetValue("");

  this->ClearAllVariables();
  this->AddAllVariables(0);
  
  this->ModifiedCallback();
}

void vtkPVCalculatorWidget::ChangeAttributeMode(const char* newMode)
{
  if (!strcmp(newMode, "point"))
    {
    this->AttributeModeMenu->SetValue("Point Data");
    this->AddTraceEntry("$kw(%s) ChangeAttributeMode {%s}",
                        this->GetTclName(), newMode);
    }
  if (!strcmp(newMode, "cell"))
    {
    this->AttributeModeMenu->SetValue("Cell Data");
    this->AddTraceEntry("$kw(%s) ChangeAttributeMode {%s}",
                        this->GetTclName(), newMode);
    }
  
  this->ScalarsMenu->GetMenu()->DeleteAllMenuItems();
  this->VectorsMenu->GetMenu()->DeleteAllMenuItems();
  this->FunctionLabel->SetValue("");

  this->AddAllVariables(1);

  this->ModifiedCallback();
}

void vtkPVCalculatorWidget::AddScalarVariable(const char* variableName,
                                              const char* arrayName,
                                              int component)
{
  if (this->ScalarVariableExists(variableName, arrayName, component))
    {
    return;
    }
  
  char** arrayNames = new char *[this->NumberOfScalarVariables];
  char** varNames = new char *[this->NumberOfScalarVariables];
  int* tempComponents = new int[this->NumberOfScalarVariables];
  int i;
  
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    arrayNames[i] = new char[strlen(this->ScalarArrayNames[i]) + 1];
    strcpy(arrayNames[i], this->ScalarArrayNames[i]);
    delete [] this->ScalarArrayNames[i];
    this->ScalarArrayNames[i] = NULL;
    varNames[i] = new char[strlen(this->ScalarVariableNames[i]) + 1];
    strcpy(varNames[i], this->ScalarVariableNames[i]);
    delete [] this->ScalarVariableNames[i];
    this->ScalarVariableNames[i] = NULL;
    tempComponents[i] = this->ScalarComponents[i];
    }
  if (this->ScalarArrayNames)
    {
    delete [] this->ScalarArrayNames;
    this->ScalarArrayNames = NULL;
    }
  if (this->ScalarVariableNames)
    {
    delete [] this->ScalarVariableNames;
    this->ScalarVariableNames = NULL;
    }
  if (this->ScalarComponents)
    {
    delete [] this->ScalarComponents;
    this->ScalarComponents = NULL;
    }
  
  this->ScalarArrayNames = new char *[this->NumberOfScalarVariables + 1];
  this->ScalarVariableNames = new char *[this->NumberOfScalarVariables + 1];
  this->ScalarComponents = new int[this->NumberOfScalarVariables + 1];
  
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    this->ScalarArrayNames[i] = new char[strlen(arrayNames[i]) + 1];
    strcpy(this->ScalarArrayNames[i], arrayNames[i]);
    delete [] arrayNames[i];
    this->ScalarVariableNames[i] = new char[strlen(varNames[i]) + 1];
    strcpy(this->ScalarVariableNames[i], varNames[i]);
    delete [] varNames[i];
    this->ScalarComponents[i] = tempComponents[i];
    }
  delete [] arrayNames;
  delete [] varNames;
  delete [] tempComponents;
  
  this->ScalarArrayNames[i] = new char[strlen(arrayName) + 1];
  strcpy(this->ScalarArrayNames[i], arrayName);
  this->ScalarVariableNames[i] = new char[strlen(variableName) + 1];
  strcpy(this->ScalarVariableNames[i], variableName);
  this->ScalarComponents[i] = component;
  
  this->NumberOfScalarVariables++;
}

int vtkPVCalculatorWidget::ScalarVariableExists(const char *variableName,
                                                const char *arrayName,
                                                int component)
{
  int i;
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    if (!strcmp(this->ScalarVariableNames[i], variableName) &&
        !strcmp(this->ScalarArrayNames[i], arrayName) &&
        this->ScalarComponents[i] == component)
      {
      return 1;
      }
    }
  
  return 0;
}

void vtkPVCalculatorWidget::AddVectorVariable(const char* variableName,
                                             const char* arrayName)
{
  if (this->VectorVariableExists(variableName, arrayName))
    {
    return;
    }
  
  char** arrayNames = new char *[this->NumberOfVectorVariables];
  char** varNames = new char *[this->NumberOfVectorVariables];
  int i;
  
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    arrayNames[i] = new char[strlen(this->VectorArrayNames[i]) + 1];
    strcpy(arrayNames[i], this->VectorArrayNames[i]);
    delete [] this->VectorArrayNames[i];
    this->VectorArrayNames[i] = NULL;
    varNames[i] = new char[strlen(this->VectorVariableNames[i]) + 1];
    strcpy(varNames[i], this->VectorVariableNames[i]);
    delete [] this->VectorVariableNames[i];
    this->VectorVariableNames[i] = NULL;
    }
  if (this->VectorArrayNames)
    {
    delete [] this->VectorArrayNames;
    this->VectorArrayNames = NULL;
    }
  if (this->VectorVariableNames)
    {
    delete [] this->VectorVariableNames;
    this->VectorVariableNames = NULL;
    }
  
  this->VectorArrayNames = new char *[this->NumberOfVectorVariables + 1];
  this->VectorVariableNames = new char *[this->NumberOfVectorVariables + 1];
  
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    this->VectorArrayNames[i] = new char[strlen(arrayNames[i]) + 1];
    strcpy(this->VectorArrayNames[i], arrayNames[i]);
    delete [] arrayNames[i];
    this->VectorVariableNames[i] = new char[strlen(varNames[i]) + 1];
    strcpy(this->VectorVariableNames[i], varNames[i]);
    delete [] varNames[i];
    }
  delete [] arrayNames;
  delete [] varNames;
  
  this->VectorArrayNames[i] = new char[strlen(arrayName) + 1];
  strcpy(this->VectorArrayNames[i], arrayName);
  this->VectorVariableNames[i] = new char[strlen(variableName) + 1];
  strcpy(this->VectorVariableNames[i], variableName);
  
  this->NumberOfVectorVariables++;  
}

int vtkPVCalculatorWidget::VectorVariableExists(const char *variableName,
                                                const char *arrayName)
{
  int i;
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    if (!strcmp(this->VectorVariableNames[i], variableName) &&
        !strcmp(this->VectorArrayNames[i], arrayName))
      {
      return 1;
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkPVCalculatorWidget::Trace(ofstream *file)
{
  int idx;

  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  if (!strcmp(this->AttributeModeMenu->GetValue(), "Point Data"))
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeAttributeMode {point}"
          << endl;
    }
  if (!strcmp(this->AttributeModeMenu->GetValue(), "Cell Data"))
    {
    *file << "$kw(" << this->GetTclName() << ") ChangeAttributeMode {cell}"
          << endl;
    }
  
  for (idx = 0; idx < this->NumberOfScalarVariables; ++ idx)
    {
    *file << "$kw(" << this->GetTclName() << ") AddScalarVariable {"
          << this->ScalarVariableNames[idx] << "} {"
          << this->ScalarArrayNames[idx] << "} " << this->ScalarComponents[idx]
          << endl;
    }

  for (idx = 0; idx < this->NumberOfVectorVariables; ++ idx)
    {
    *file << "$kw(" << this->GetTclName() << ") AddVectorVariable {"
          << this->VectorVariableNames[idx] << "} {"
          << this->VectorArrayNames[idx] << "}" << endl;
    }

  *file << "$kw(" << this->GetTclName() << ") SetFunctionLabel {"
        << this->FunctionLabel->GetValue() << "}" << endl;
}


//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::AcceptInternal(vtkClientServerID vtkSourceID)
{
  // Format a command to move value from widget to vtkObjects (on all
  // processes).  The VTK objects do not yet have to have the same Tcl
  // name!
  int i;
  
  char **cmds = new char *[this->NumberOfScalarVariables +
                          this->NumberOfVectorVariables + 3];
  char **strings = new char *[2*(this->NumberOfScalarVariables +
                                 this->NumberOfVectorVariables)+1];
  float *scalars = new float[this->NumberOfScalarVariables +
                            3*this->NumberOfVectorVariables];
  int *numStrings = new int[this->NumberOfScalarVariables +
                           this->NumberOfVectorVariables + 3];
  int *numScalars = new int[this->NumberOfScalarVariables +
                           this->NumberOfVectorVariables + 3];
  int stringCount = 0, scalarCount = 0, cmdCount = 0;
  
  cmds[cmdCount] = new char[19];
  strcpy(cmds[cmdCount], "RemoveAllVariables");
  numStrings[cmdCount] = 0;
  numScalars[cmdCount] = 0;
  cmdCount++;
  
  const char *mode = this->AttributeModeMenu->GetValue();
  if (strcmp(mode, "Point Data") == 0)
    {
    cmds[cmdCount] = new char[31];
    strcpy(cmds[cmdCount], "SetAttributeModeToUsePointData");
    numStrings[cmdCount] = 0;
    numScalars[cmdCount] = 0;
    cmdCount++;
    }
  else
    {
    cmds[cmdCount] = new char[30];
    strcpy(cmds[cmdCount], "SetAttributeModeToUseCellData");
    numStrings[cmdCount] = 0;
    numScalars[cmdCount] = 0;
    cmdCount++;
    }
  
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    cmds[cmdCount] = new char[18];
    strcpy(cmds[cmdCount], "AddScalarVariable");
    strings[stringCount] = new char[strlen(this->ScalarVariableNames[i])+1];
    strcpy(strings[stringCount], this->ScalarVariableNames[i]);
    stringCount++;
    strings[stringCount] = new char[strlen(this->ScalarArrayNames[i])+1];
    strcpy(strings[stringCount], this->ScalarArrayNames[i]);
    stringCount++;
    scalars[scalarCount] = this->ScalarComponents[i];
    scalarCount++;
    numStrings[cmdCount] = 2;
    numScalars[cmdCount] = 1;
    cmdCount++;
    }
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    cmds[cmdCount] = new char[18];
    strcpy(cmds[cmdCount], "AddVectorVariable");
    strings[stringCount] = new char[strlen(this->VectorVariableNames[i])+1];
    strcpy(strings[stringCount], this->VectorVariableNames[i]);
    stringCount++;
    strings[stringCount] = new char[strlen(this->VectorArrayNames[i])+1];
    strcpy(strings[stringCount], this->VectorArrayNames[i]);
    stringCount++;
    scalars[scalarCount] = 0;
    scalarCount++;
    scalars[scalarCount] = 1;
    scalarCount++;
    scalars[scalarCount] = 2;
    scalarCount++;
    numStrings[cmdCount] = 2;
    numScalars[cmdCount] = 3;
    cmdCount++;
    }
  
  cmds[cmdCount] = new char[12];
  strcpy(cmds[cmdCount], "SetFunction");
  strings[stringCount] = new char[strlen(this->FunctionLabel->GetValue())+1];
  strcpy(strings[stringCount], this->FunctionLabel->GetValue());
  stringCount++;
  numStrings[cmdCount] = 1;
  numScalars[cmdCount] = 0;
  cmdCount++;
  
  this->Property->SetVTKCommands(cmdCount, cmds, numStrings, numScalars);
  this->Property->SetStrings(stringCount, strings);
  this->Property->SetScalars(scalarCount, scalars);
  this->Property->SetVTKSourceID(vtkSourceID);
  this->Property->AcceptInternal();
  
  for (i = 0; i < cmdCount; i++)
    {
    delete [] cmds[i];
    }
  for (i = 0; i < stringCount; i++)
    {
    delete [] strings[i];
    }
  delete [] cmds;
  delete [] strings;
  delete [] scalars;
  delete [] numStrings;
  delete [] numScalars;
  
  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::ResetInternal()
{
  if ( this->FunctionLabel->IsCreated() )
    {
    int numStrings = this->Property->GetNumberOfStrings();
    if (numStrings > 0)
      {
      this->FunctionLabel->SetValue(this->Property->GetString(numStrings-1));
      }
    }
  
  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
}


//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::SetFunctionLabel(char *function)
{
  this->ModifiedCallback();
  this->FunctionLabel->SetValue(function);
}


//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::SaveInBatchScript(ofstream *file)
{
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("SaveInBatchScript requires a PVSource.")
    return;
    }

  int i;

  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);

  *file << "  [$pvTemp" << sourceID.ID 
        << " GetProperty AttributeMode] SetElement 0 ";
  if (strcmp(this->AttributeModeMenu->GetValue(), "Point Data") == 0)
    {
    *file << 1;
    }
  else
    {
    *file << 2;
    }
  *file << endl;

  *file << "  [$pvTemp" << sourceID.ID 
        << " GetProperty AddScalarVariable] SetNumberOfElements " 
        << this->NumberOfScalarVariables*3 << endl;
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    *file << "  [$pvTemp" << sourceID.ID 
          << " GetProperty AddScalarVariable] SetElement " << i*3
          << " {" <<  this->ScalarVariableNames[i] << "}" << endl;
    *file << "  [$pvTemp" << sourceID.ID 
          << " GetProperty AddScalarVariable] SetElement " << i*3+1
          << " {" <<  this->ScalarArrayNames[i] << "}" << endl;
    *file << "  [$pvTemp" << sourceID.ID 
          << " GetProperty AddScalarVariable] SetElement " << i*3+2
          << " " << this->ScalarComponents[i] << endl;
    }

  *file << "  [$pvTemp" << sourceID.ID 
        << " GetProperty AddVectorVariable] SetNumberOfElements " 
        << this->NumberOfVectorVariables*5 << endl;
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    *file << "  [$pvTemp" << sourceID.ID 
          << " GetProperty AddVectorVariable] SetElement " << i*5
          << " {" <<  this->VectorVariableNames[i] << "}" << endl;
    *file << "  [$pvTemp" << sourceID.ID 
          << " GetProperty AddVectorVariable] SetElement " << i*5+1
          << " {" <<  this->VectorArrayNames[i] << "}" << endl;
    *file << "  [$pvTemp" << sourceID.ID 
          << " GetProperty AddVectorVariable] SetElement " << i*5+2
          << " 0" << endl;
    *file << "  [$pvTemp" << sourceID.ID 
          << " GetProperty AddVectorVariable] SetElement " << i*5+3
          << " 1" << endl;
    *file << "  [$pvTemp" << sourceID.ID 
          << " GetProperty AddVectorVariable] SetElement " << i*5+4
          << " 2" << endl;
    }


  if ( this->FunctionLabel->IsCreated() )
    {
    *file << "  [$pvTemp" << sourceID.ID 
          << " GetProperty Function] SetElement 0 "
          <<  "{" << this->FunctionLabel->GetValue() << "}"
          << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::ClearAllVariables()
{
  int i;
  
  for (i = 0; i < this->NumberOfScalarVariables; i++)
    {
    delete [] this->ScalarVariableNames[i];
    this->ScalarVariableNames[i] = NULL;
    delete [] this->ScalarArrayNames[i];
    this->ScalarArrayNames[i] = NULL;
    }
  if (this->ScalarVariableNames)
    {
    delete [] this->ScalarVariableNames;
    this->ScalarVariableNames = NULL;
    }
  if (this->ScalarArrayNames)
    {
    delete [] this->ScalarArrayNames;
    this->ScalarArrayNames = NULL;
    }
  if (this->ScalarComponents)
    {
    delete [] this->ScalarComponents;
    this->ScalarComponents = NULL;
    }
  this->NumberOfScalarVariables = 0;
  
  for (i = 0; i < this->NumberOfVectorVariables; i++)
    {
    delete [] this->VectorVariableNames[i];
    this->VectorVariableNames[i] = NULL;
    delete [] this->VectorArrayNames[i];
    this->VectorArrayNames[i] = NULL;
    }
  if (this->VectorVariableNames)
    {
    delete [] this->VectorVariableNames;
    this->VectorVariableNames = NULL;
    }
  if (this->VectorArrayNames)
    {
    delete [] this->VectorArrayNames;
    this->VectorArrayNames = NULL;
    }
  this->NumberOfVectorVariables = 0;  
}

//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::AddAllVariables(int populateMenus)
{
  vtkPVDataSetAttributesInformation* fdi = NULL;
  int i, j;
  int numComponents;
  char menuCommand[256];
  char menuEntry[256];
  char* name;
  const char* mode = this->AttributeModeMenu->GetValue();

  // Populate the scalar and array menu using collected data information.
  if (strcmp(mode, "Point Data") == 0)
    {
    fdi = this->PVSource->GetPVInput(0)->GetDataInformation()->GetPointDataInformation();
    }
  else if (strcmp(mode, "Cell Data") == 0)
    {
    fdi = this->PVSource->GetPVInput(0)->GetDataInformation()->GetCellDataInformation();
    }
  
  if (fdi)
    {
    for (i = 0; i < fdi->GetNumberOfArrays(); i++)
      {
      numComponents = fdi->GetArrayInformation(i)->GetNumberOfComponents();
      name = fdi->GetArrayInformation(i)->GetName();
      for (j = 0; j < numComponents; j++)
        {
        if (numComponents == 1)
          {
          this->AddScalarVariable(name, name, 0);
          if (populateMenus)
            {
            sprintf(menuCommand, "UpdateFunction {%s}", name);
            this->ScalarsMenu->GetMenu()->AddCommand(name, this,
                                                     menuCommand);
            }
          }
        else
          {
          sprintf(menuEntry, "%s_%d", name, j);
          this->AddScalarVariable(menuEntry, name, j);
          if (populateMenus)
            {
            sprintf(menuCommand, "UpdateFunction {%s}", menuEntry);
            this->ScalarsMenu->GetMenu()->AddCommand(menuEntry, this, menuCommand);
            }
          }
        }
      if (numComponents == 3)
        {
        this->AddVectorVariable(name, name);
        if (populateMenus)
          {
          sprintf(menuCommand, "UpdateFunction {%s}", name);
          this->VectorsMenu->GetMenu()->AddCommand(name, this,
                                                   menuCommand);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVStringAndScalarListWidgetProperty::SafeDownCast(prop);
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVCalculatorWidget::GetProperty()
{
  return this->Property;
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVCalculatorWidget::CreateAppropriateProperty()
{
  return vtkPVStringAndScalarListWidgetProperty::New();
}

//-----------------------------------------------------------------------------
void vtkPVCalculatorWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->AttributeModeFrame);
  this->PropagateEnableState(this->AttributeModeLabel);
  this->PropagateEnableState(this->AttributeModeMenu);
  this->PropagateEnableState(this->CalculatorFrame);
  this->PropagateEnableState(this->FunctionLabel);
  this->PropagateEnableState(this->ButtonClear);
  this->PropagateEnableState(this->ButtonZero);
  this->PropagateEnableState(this->ButtonOne);
  this->PropagateEnableState(this->ButtonTwo);
  this->PropagateEnableState(this->ButtonThree);
  this->PropagateEnableState(this->ButtonFour);
  this->PropagateEnableState(this->ButtonFive);
  this->PropagateEnableState(this->ButtonSix);
  this->PropagateEnableState(this->ButtonSeven);
  this->PropagateEnableState(this->ButtonEight);
  this->PropagateEnableState(this->ButtonNine);
  this->PropagateEnableState(this->ButtonDivide);
  this->PropagateEnableState(this->ButtonMultiply);
  this->PropagateEnableState(this->ButtonSubtract);
  this->PropagateEnableState(this->ButtonAdd);
  this->PropagateEnableState(this->ButtonDecimal);
  this->PropagateEnableState(this->ButtonDot);
  this->PropagateEnableState(this->ButtonSin);
  this->PropagateEnableState(this->ButtonCos);
  this->PropagateEnableState(this->ButtonTan);
  this->PropagateEnableState(this->ButtonASin);
  this->PropagateEnableState(this->ButtonACos);
  this->PropagateEnableState(this->ButtonATan);
  this->PropagateEnableState(this->ButtonSinh);
  this->PropagateEnableState(this->ButtonCosh);
  this->PropagateEnableState(this->ButtonTanh);
  this->PropagateEnableState(this->ButtonPow);
  this->PropagateEnableState(this->ButtonSqrt);
  this->PropagateEnableState(this->ButtonExp);
  this->PropagateEnableState(this->ButtonCeiling);
  this->PropagateEnableState(this->ButtonFloor);
  this->PropagateEnableState(this->ButtonLog);
  this->PropagateEnableState(this->ButtonAbs);
  this->PropagateEnableState(this->ButtonMag);
  this->PropagateEnableState(this->ButtonNorm);
  this->PropagateEnableState(this->ButtonIHAT);
  this->PropagateEnableState(this->ButtonJHAT);
  this->PropagateEnableState(this->ButtonKHAT);
  this->PropagateEnableState(this->ButtonLeftParenthesis);
  this->PropagateEnableState(this->ButtonRightParenthesis);
  this->PropagateEnableState(this->ScalarsMenu);
  this->PropagateEnableState(this->VectorsMenu);
}

//----------------------------------------------------------------------------
void vtkPVCalculatorWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

/*=========================================================================

  Program:   ParaView
  Module:    vtkPVConnectDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVConnectDialog.h"

#include "vtkPVApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledCheckButton.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWScale.h"
#include "vtkObjectFactory.h"
#include "vtkPVWindow.h"
#include "vtkStringList.h"

#include "vtkstd/string"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVConnectDialog);
vtkCxxRevisionMacro(vtkPVConnectDialog, "1.16");

//----------------------------------------------------------------------------
void vtkPVConnectDialog::Create(vtkKWApplication* app, const char* vtkNotUsed(opts))
{
  char buffer[1024];
  sprintf(buffer, "Cannot connect to the server %s:%d.\nPlease specify server to connect:",
    this->HostnameString, this->PortInt);
  this->SetOptions(
    vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault |
    vtkKWMessageDialog::WarningIcon );
  this->SetStyleToOkCancel();
  this->Superclass::Create(app, 0);
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);
  this->SetMasterWindow(pvApp->GetMainWindow());
  this->SetText(buffer);
  this->SetTitle("ParaView Connection Warning");
  this->Label->SetParent(this->BottomFrame);
  this->Label->Create(app, "-text {Hostname}");
  vtkKWFrame* frame = vtkKWFrame::New();
  frame->SetParent(this->BottomFrame);
  frame->Create(app, 0);
  this->Username->SetParent(frame);
  this->Username->Create(app, 0);
  this->Username->SetValue(this->SSHUser);
  this->Hostname->SetParent(frame);
  this->Hostname->Create(app, 0);
  this->Hostname->SetLabel("@");
  this->Port->SetParent(frame);
  this->Port->Create(app,0);
  this->Port->SetLabel(":");
  this->Port->GetEntry()->SetWidth(4);
  this->Username->SetWidth(7);
  this->Script("pack %s -side left -expand 0", 
    this->Username->GetWidgetName());
  this->Script("pack %s -side left -expand 1 -fill x", 
    this->Hostname->GetWidgetName());
  this->Script("pack %s -side left -expand 0", 
    this->Port->GetWidgetName());
  this->Script("pack %s -side top -expand 1 -fill both", 
    frame->GetWidgetName());
  frame->Delete();
  frame = vtkKWFrame::New();
  frame->SetParent(this->BottomFrame);
  frame->Create(app, 0);

  this->MPIMode->SetParent(frame);
  this->MPIMode->Create(app, 0);
  this->MPIMode->SetLabel("Use MPI");
  if ( this->MultiProcessMode == 1 )
    {
    this->MPIMode->GetCheckButton()->SetState(1);
    }
  else
    {
    this->MPIMode->GetCheckButton()->SetState(0);
    }
  this->MPIMode->GetCheckButton()->SetCommand(this, "MPICheckBoxCallback");
  this->MPINumberOfServers->SetParent(frame);
  this->MPINumberOfServers->PopupScaleOn();
  this->MPINumberOfServers->Create(app, 0);
  this->MPINumberOfServers->DisplayEntry();
  this->MPINumberOfServers->DisplayLabel("Number of processes");
  this->MPINumberOfServers->SetRange(2, 10);
  this->MPINumberOfServers->SetValue(this->NumberOfProcesses);
  this->Script("pack %s -side left -expand 1 -fill x", 
    this->MPIMode->GetWidgetName());
  this->Script("pack %s -side left -expand 1 -fill x", 
    this->MPINumberOfServers->GetWidgetName());

  this->Script("pack %s -side top -expand 1 -fill both", 
    frame->GetWidgetName());
  frame->Delete();
  this->SetHostname(this->HostnameString);
  this->SetPort(this->PortInt);
  this->MPINumberOfServers->EnabledOff();

  char servers[1024];
  if ( app->GetRegisteryValue(2, "RunTime", "ConnectionServers", servers) )
    {
    const char* server = servers;
    size_t cc;
    size_t len = strlen(servers);
    for ( cc = 0; cc < len; cc ++ )
      {
      if ( servers[cc] == ',' )
        {
        servers[cc] = 0;
        this->Hostname->GetEntry()->AddValue(server);
        server = servers + cc + 1;
        }
      }
    if ( strlen(server) )
      {
      this->Hostname->GetEntry()->AddValue(server);
      }
    }
  this->GrabDialog = 0;
}

//----------------------------------------------------------------------------
void vtkPVConnectDialog::OK()
{
  this->SetHostnameString(this->Hostname->GetEntry()->GetValue());
  this->PortInt = this->Port->GetEntry()->GetValueAsInt();
  this->NumberOfProcesses = static_cast<int>(this->MPINumberOfServers->GetValue());
  if ( this->MPIMode->GetCheckButton()->GetState() )
    {
    this->MultiProcessMode = 1;
    }
  else
    {
    this->MultiProcessMode = 0;
    }
  this->SetSSHUser(this->Username->GetValue());

  int cc;
  vtkstd::string servers;
  servers = this->Hostname->GetEntry()->GetValue();
  for ( cc = 0; cc < this->Hostname->GetEntry()->GetNumberOfValues(); cc ++ )
    {
    servers += ",";
    servers += this->Hostname->GetEntry()->GetValueFromIndex(cc);
    }
  this->GetApplication()->SetRegisteryValue(2, "RunTime", "ConnectionServers", servers.c_str());

  this->Superclass::OK();
}

//----------------------------------------------------------------------------
void vtkPVConnectDialog::SetHostname(const char* hn)
{
  if ( this->Hostname->IsCreated() )
    {
    this->Hostname->GetEntry()->SetValue(hn);
    }
  this->SetHostnameString(hn);
}

//----------------------------------------------------------------------------
const char* vtkPVConnectDialog::GetHostName()
{
  return this->HostnameString;
}

//----------------------------------------------------------------------------
void vtkPVConnectDialog::SetPort(int pt)
{
  if ( this->Port->IsCreated() )
    {
    char buffer[100];
    sprintf(buffer, "%d", pt);
    this->Port->GetEntry()->SetValue(buffer);
    }
  this->PortInt = pt;
}

//----------------------------------------------------------------------------
int vtkPVConnectDialog::GetPort()
{
  return this->PortInt;
}

//----------------------------------------------------------------------------
void vtkPVConnectDialog::MPICheckBoxCallback()
{
  if ( this->MPIMode->GetCheckButton()->GetState() )
    {
    this->MPINumberOfServers->EnabledOn();
    }
  else
    {
    this->MPINumberOfServers->EnabledOff();
    }
}

//----------------------------------------------------------------------------
vtkPVConnectDialog::vtkPVConnectDialog()
{
  this->Username = vtkKWEntry::New();
  this->Hostname = vtkKWLabeledEntry::New();
  this->Hostname->GetEntry()->PullDownOn();
  this->Port = vtkKWLabeledEntry::New();
  this->Label = vtkKWLabel::New();
  this->MPINumberOfServers = vtkKWScale::New();
  this->MPIMode = vtkKWLabeledCheckButton::New();

  this->HostnameString = 0;
  this->PortInt = 0;
  this->MultiProcessMode = 0;
  this->NumberOfProcesses = 2;
  this->SSHUser = 0;

  this->ListOfServersString = 0;
  this->Servers = vtkStringList::New();
}

//----------------------------------------------------------------------------
vtkPVConnectDialog::~vtkPVConnectDialog()
{
  this->Username->Delete();
  this->Hostname->Delete();
  this->Port->Delete();
  this->Label->Delete();
  this->MPINumberOfServers->Delete();
  this->MPIMode->Delete();
  this->SetSSHUser(0);
  this->SetListOfServersString(0);
  this->Servers->Delete();
}

//----------------------------------------------------------------------------
void vtkPVConnectDialog::SetListOfServers(const char* list)
{
  vtkstd::string cserv;
  vtkstd::string::size_type cc;
  for ( cc = 0; list[cc]; cc ++ )
    {
    if ( list[cc] == ';' )
      {
      this->Servers->AddUniqueString(cserv.c_str());
      cserv = "";
      }
    else
      {
      cserv.append(1, list[cc]);
      }
    }
  if ( cserv.size() )
    {
    this->Servers->AddUniqueString(cserv.c_str());
    }
  int kk;
  for ( kk = 0; kk < this->Servers->GetLength(); kk ++ )
    {
    this->Hostname->GetEntry()->AddValue(this->Servers->GetString(kk));
    }
}

//----------------------------------------------------------------------------
const char* vtkPVConnectDialog::GetListOfServers()
{
  vtkstd::string servlist;
  int cc;

  this->Servers->AddUniqueString(this->Hostname->GetEntry()->GetValue());
  for ( cc = 0; cc < this->Hostname->GetEntry()->GetNumberOfValues(); cc ++ )
    {
    const char* server = this->Hostname->GetEntry()->GetValueFromIndex(cc);
    this->Servers->AddUniqueString(server);
    }

  for ( cc = 0; cc < this->Servers->GetLength(); cc ++ )
    {
    if ( cc )
      {
      servlist += ";";
      }
    servlist += this->Servers->GetString(cc);
    }
  this->SetListOfServersString(servlist.c_str());
  return this->ListOfServersString;
}

//----------------------------------------------------------------------------
void vtkPVConnectDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SSHUser: " << (this->SSHUser?this->SSHUser:"(none)") << endl;
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
  os << indent << "MultiProcessMode: " << this->MultiProcessMode << endl;
}

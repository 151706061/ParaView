/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLDataReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLDataReader.h"
#include "vtkXMLDataParser.h"
#include "vtkXMLDataElement.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"

vtkCxxRevisionMacro(vtkXMLDataReader, "1.2");

//----------------------------------------------------------------------------
vtkXMLDataReader::vtkXMLDataReader()
{
  this->NumberOfPieces = 0;
  this->PointDataElements = 0;
  this->CellDataElements = 0;
  this->Piece = 0;
}

//----------------------------------------------------------------------------
vtkXMLDataReader::~vtkXMLDataReader()
{
  if(this->NumberOfPieces) { this->DestroyPieces(); }
}

//----------------------------------------------------------------------------
void vtkXMLDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if(!this->Superclass::ReadPrimaryElement(ePrimary)) { return 0; }
  
  // Count the number of pieces in the file.
  int numNested = ePrimary->GetNumberOfNestedElements();
  int numPieces = 0;
  int i;
  for(i=0; i < numNested; ++i)
    {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if(strcmp(eNested->GetName(), "Piece") == 0) { ++numPieces; }
    }
  
  // Now read each piece.  If no "Piece" elements were found, assume
  // the primary element itself is a single piece.
  if(numPieces)
    {
    this->SetupPieces(numPieces);
    int piece = 0;
    for(i=0;i < numNested; ++i)
      {
      vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
      if(strcmp(eNested->GetName(), "Piece") == 0)
        {
        if(!this->ReadPiece(eNested, piece++)) { return 0; }
        }
      }
    }
  else
    {
    this->SetupPieces(1);
    if(!this->ReadPiece(ePrimary, 0)) { return 0; }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLDataReader::SetupPieces(int numPieces)
{
  if(this->NumberOfPieces) { this->DestroyPieces(); }
  this->NumberOfPieces = numPieces;
  this->PointDataElements = new vtkXMLDataElement*[numPieces];
  this->CellDataElements = new vtkXMLDataElement*[numPieces];
  int i;
  for(i=0;i < this->NumberOfPieces;++i)
    {
    this->PointDataElements[i] = 0;
    this->CellDataElements[i] = 0;
    }
}

//----------------------------------------------------------------------------
void vtkXMLDataReader::DestroyPieces()
{
  delete [] this->PointDataElements;
  delete [] this->CellDataElements;
  this->PointDataElements = 0;
  this->CellDataElements = 0;
  this->NumberOfPieces = 0;
}

//----------------------------------------------------------------------------
void vtkXMLDataReader::SetupOutputInformation()
{
  this->Superclass::SetupOutputInformation();
  if(!this->NumberOfPieces) { return; }
  
  int i;
  // Use the configuration of the first piece since all are the same.
  vtkXMLDataElement* ePointData = this->PointDataElements[0];
  vtkXMLDataElement* eCellData = this->CellDataElements[0];
  vtkPointData* pointData = this->GetOutputAsDataSet()->GetPointData();
  vtkCellData* cellData = this->GetOutputAsDataSet()->GetCellData();  
  
  // Setup the point and cell data arrays without allocation.
  if(ePointData)
    {
    for(i=0;i < ePointData->GetNumberOfNestedElements();++i)
      {
      vtkXMLDataElement* eNested = ePointData->GetNestedElement(i);
      vtkDataArray* array = this->CreateDataArray(eNested);
      pointData->AddArray(array);
      array->Delete();
      }
    }
  if(eCellData)
    {
    for(i=0;i < eCellData->GetNumberOfNestedElements();++i)
      {
      vtkXMLDataElement* eNested = eCellData->GetNestedElement(i);
      vtkDataArray* array = this->CreateDataArray(eNested);
      cellData->AddArray(array);
      array->Delete();
      }
    }  
  
  // Setup attribute indices for the point data and cell data.
  this->ReadAttributeIndices(ePointData, pointData);
  this->ReadAttributeIndices(eCellData, cellData);
}

//----------------------------------------------------------------------------
void vtkXMLDataReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
  
  vtkDataSet* output = this->GetOutputAsDataSet();
  vtkPointData* pointData = output->GetPointData();
  vtkCellData* cellData = output->GetCellData();
  
  // Get the size of the output arrays.
  unsigned long pointTuples = this->GetNumberOfPoints();
  unsigned long cellTuples = this->GetNumberOfCells();
  
  // Allocate the arrays in the output.  We only need the information
  // from one piece because all pieces have the same set of arrays.
  vtkXMLDataElement* ePointData = this->PointDataElements[0];
  vtkXMLDataElement* eCellData = this->CellDataElements[0];
  int i;
  if(ePointData)
    {
    for(i=0;i < ePointData->GetNumberOfNestedElements();++i)
      {
      pointData->GetArray(i)->SetNumberOfTuples(pointTuples);
      }
    }
  if(eCellData)
    {
    for(i=0;i < eCellData->GetNumberOfNestedElements();++i)
      {
      cellData->GetArray(i)->SetNumberOfTuples(cellTuples);
      }
    }  
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadPiece(vtkXMLDataElement* ePiece, int piece)
{
  this->Piece = piece;
  return this->ReadPiece(ePiece);
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  // Find the PointData and CellData in the piece.
  int i;
  for(i=0; i < ePiece->GetNumberOfNestedElements(); ++i)
    {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if(strcmp(eNested->GetName(), "PointData") == 0)
      {
      this->PointDataElements[this->Piece] = eNested;
      }
    else if(strcmp(eNested->GetName(), "CellData") == 0)
      {
      this->CellDataElements[this->Piece] = eNested;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadPieceData(int piece)
{
  this->Piece = piece;
  return this->ReadPieceData();
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadPieceData()
{
  vtkPointData* pointData = this->GetOutputAsDataSet()->GetPointData();
  vtkCellData* cellData = this->GetOutputAsDataSet()->GetCellData();
  vtkXMLDataElement* ePointData = this->PointDataElements[this->Piece];
  vtkXMLDataElement* eCellData = this->CellDataElements[this->Piece];
  
  // Read the data for this piece from each array.
  int i;
  if(ePointData)
    {
    for(i=0;i < ePointData->GetNumberOfNestedElements();++i)
      {
      if(!this->ReadArrayForPoints(ePointData->GetNestedElement(i),
                                   pointData->GetArray(i)))
        {
        return 0;
        }
      }
    }
  if(eCellData)
    {
    for(i=0;i < eCellData->GetNumberOfNestedElements();++i)
      {
      if(!this->ReadArrayForCells(eCellData->GetNestedElement(i),
                                  cellData->GetArray(i)))
        {
        return 0;
        }
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadArrayForPoints(vtkXMLDataElement* da,
                                         vtkDataArray* outArray)
{
  vtkIdType components = outArray->GetNumberOfComponents();
  vtkIdType numberOfTuples = this->GetNumberOfPoints();
  return this->ReadData(da, outArray->GetVoidPointer(0),
                        outArray->GetDataType(),
                        0, numberOfTuples*components);
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadArrayForCells(vtkXMLDataElement* da,
                                        vtkDataArray* outArray)
{
  vtkIdType components = outArray->GetNumberOfComponents();
  vtkIdType numberOfTuples = this->GetNumberOfCells();
  return this->ReadData(da, outArray->GetVoidPointer(0),
                        outArray->GetDataType(),
                        0, numberOfTuples*components);
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadData(vtkXMLDataElement* da, void* data, int wordType,
                               int startWord, int numWords)
{ 
  unsigned long num = numWords;
  if(da->GetAttribute("offset"))
    {
    int offset = 0;
    da->GetScalarAttribute("offset", offset);
    return (this->XMLParser->ReadAppendedData(offset, data, startWord,
                                              numWords, wordType) == num);
    }
  int isAscii = 1;
  const char* format = da->GetAttribute("format");
  if(format && (strcmp(format, "binary") == 0)) { isAscii = 0; }
  return (this->XMLParser->ReadInlineData(da, isAscii, data,
                                          startWord, numWords, wordType)
          == num);
}

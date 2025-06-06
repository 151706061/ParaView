// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSpreadSheetView.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCSVExporter.h"
#include "vtkCharArray.h"
#include "vtkClientServerMoveData.h"
#include "vtkCommunicator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMarkSelectedRows.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVLogger.h"
#include "vtkPVMergeTables.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkReductionFilter.h"
#include "vtkSmartPointer.h"
#include "vtkSortedTableStreamer.h"
#include "vtkSplitColumnComponents.h"
#include "vtkSpreadSheetRepresentation.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVariant.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace
{
vtkSmartPointer<vtkAbstractArray> MapBlockNames(vtkAbstractArray* aa_ids, vtkStringArray* names)
{
  const auto ids = vtkIdTypeArray::SafeDownCast(aa_ids);
  if (ids == nullptr || names == nullptr)
  {
    return aa_ids;
  }

  const auto maxNames = names->GetNumberOfTuples();

  vtkNew<vtkStringArray> mappedNames;
  mappedNames->SetName(ids->GetName());
  mappedNames->SetNumberOfTuples(ids->GetNumberOfTuples());
  const auto range = vtk::DataArrayValueRange<1>(ids);
  std::transform(range.begin(), range.end(), mappedNames->WritePointer(0, ids->GetNumberOfTuples()),
    [&names, &maxNames](vtkIdType idx)
    { return (idx >= 0 && idx < maxNames) ? names->GetValue(idx) : vtkStdString(); });
  return mappedNames;
}

/// internal function to convert any array's name to a user friendly name.
const char* get_userfriendly_name(
  const char* name, vtkSpreadSheetView* self, bool* converted = nullptr)
{
  if (converted)
  {
    *converted = true;
  }
  if (name == nullptr || name[0] == '\0')
  {
    if (converted)
    {
      *converted = false;
    }
    return name;
  }
  else if (strcmp("vtkBlockNameIndices", name) == 0)
  {
    return "Block Name";
  }
  else if (strcmp("vtkOriginalProcessIds", name) == 0)
  {
    return "Process ID";
  }
  else if (strcmp("vtkOriginalIndices", name) == 0)
  {
    switch (self->GetFieldAssociation())
    {
      case vtkDataObject::FIELD_ASSOCIATION_POINTS:
        return "Point ID";
      case vtkDataObject::FIELD_ASSOCIATION_CELLS:
        return "Cell ID";
      case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
        return "Vertex ID";
      case vtkDataObject::FIELD_ASSOCIATION_EDGES:
        return "Edge ID";
      case vtkDataObject::FIELD_ASSOCIATION_ROWS:
        return "Row ID";
      default:
        // LOG_S(INFO) << "Unknown field association encountered.";
        return name;
    }
  }
  else if (strcmp("vtkOriginalCellIds", name) == 0 && self->GetShowExtractedSelection())
  {
    return "Cell ID";
  }
  else if (strcmp("vtkOriginalPointIds", name) == 0 && self->GetShowExtractedSelection())
  {
    return "Point ID";
  }
  else if (strcmp("vtkOriginalRowIds", name) == 0 && self->GetShowExtractedSelection())
  {
    return "Row ID";
  }
  else if (strcmp("vtkCompositeIndexArray", name) == 0)
  {
    return "Block Number";
  }

  if (converted)
  {
    *converted = false;
  }
  return name;
}

/**
 * A subclass of vtkPVMergeTables to handle reduction for "vtkBlockNames" correctly.
 */
class SpreadSheetViewMergeTables : public vtkPVMergeTables
{
public:
  static SpreadSheetViewMergeTables* New();
  vtkTypeMacro(SpreadSheetViewMergeTables, vtkPVMergeTables);

protected:
  SpreadSheetViewMergeTables() = default;
  ~SpreadSheetViewMergeTables() override = default;

  int RequestData(vtkInformation* req, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override
  {
    auto output = vtkTable::GetData(outputVector, 0);
    auto inputs = vtkPVMergeTables::GetTables(inputVector[0]);

    const bool has_block_names =
      (!inputs.empty() && inputs[0]->GetFieldData()->GetAbstractArray("vtkBlockNames"));
    if (!has_block_names)
    {
      return this->Superclass::RequestData(req, inputVector, outputVector);
    }

    vtkPVMergeTables::MergeTables(output, inputs);

    output->GetFieldData()->RemoveArray("vtkBlockNames");
    output->GetFieldData()->AddArray(inputs[0]->GetFieldData()->GetAbstractArray("vtkBlockNames"));
    return 1;
  }

private:
  SpreadSheetViewMergeTables(const SpreadSheetViewMergeTables&) = delete;
  void operator=(const SpreadSheetViewMergeTables&) = delete;
};
vtkStandardNewMacro(SpreadSheetViewMergeTables);
}

class vtkSpreadSheetView::vtkInternals
{
  std::vector<std::tuple<std::string, std::string, int>> ColumnMetaData;
  std::map<std::string, size_t> ColumnIndexMap;

  void UpdateColumnMetaData(vtkTable* table)
  {
    this->ColumnMetaData.clear();
    this->ColumnIndexMap.clear();

    std::map<std::string, vtkIdType> index_map; // this is just to make the lookup faster.
    for (vtkIdType cc = 0, max = table->GetNumberOfColumns(); cc < max; ++cc)
    {
      // this build a tuple that indicates it's not an extracted component
      auto col = table->GetColumn(cc);
      auto colInfo = col->GetInformation();

      const std::string original_name =
        colInfo->Has(vtkSplitColumnComponents::ORIGINAL_ARRAY_NAME())
        ? std::string(colInfo->Get(vtkSplitColumnComponents::ORIGINAL_ARRAY_NAME()))
        : std::string();

      const int original_component =
        colInfo->Has(vtkSplitColumnComponents::ORIGINAL_COMPONENT_NUMBER())
        ? colInfo->Get(vtkSplitColumnComponents::ORIGINAL_COMPONENT_NUMBER())
        : -1;

      const std::string colName(col->GetName() ? col->GetName() : "<None>");
      auto tuple = std::make_tuple(
        colName, original_component >= 0 ? original_name : std::string(), original_component);
      this->ColumnIndexMap[std::get<0>(tuple)] = this->ColumnMetaData.size();
      this->ColumnMetaData.push_back(std::move(tuple));
    }

    assert(this->ColumnMetaData.size() == this->ColumnIndexMap.size() &&
      this->ColumnIndexMap.size() == static_cast<size_t>(table->GetNumberOfColumns()));
  }

  vtkIdType GetMostRecentlyAccessedBlock(vtkSpreadSheetView* self)
  {
    vtkIdType maxBlockId = self->GetNumberOfRows() / self->TableStreamer->GetBlockSize();
    if (this->MostRecentlyAccessedBlock >= 0 && this->MostRecentlyAccessedBlock <= maxBlockId)
    {
      return this->MostRecentlyAccessedBlock;
    }
    this->MostRecentlyAccessedBlock = 0;
    return 0;
  }

  class CacheInfo
  {
  public:
    vtkSmartPointer<vtkTable> Dataobject;
    vtkTimeStamp RecentUseTime;

    CacheInfo()
    {
      this->Dataobject = nullptr;
      this->RecentUseTime = vtkTimeStamp();
    }
  };

  typedef std::map<vtkIdType, CacheInfo> CacheType;
  CacheType CachedBlocks;
  std::pair<vtkIdType, CacheInfo> PreviousFirstCachedBlock;

public:
  void ClearCache()
  {
    if (!this->CachedBlocks.empty())
    {
      this->PreviousFirstCachedBlock = *this->CachedBlocks.begin();
    }
    this->CachedBlocks.clear();
    this->ColumnMetaData.clear();
    this->ColumnIndexMap.clear();
  }

  vtkIdType GetNumberOfColumns(vtkSpreadSheetView* self)
  {
    if (this->ActiveRepresentation != nullptr && this->ColumnMetaData.empty())
    {
      this->GetSomeBlock(self);
    }
    return static_cast<vtkIdType>(this->ColumnMetaData.size());
  }

  const char* GetColumnName(vtkIdType index, vtkSpreadSheetView* self)
  {
    if (this->GetNumberOfColumns(self) > index)
    {
      return std::get<0>(this->ColumnMetaData[index]).c_str();
    }
    return nullptr;
  }

  vtkIdType GetColumnByName(const std::string& aname) const
  {
    const auto& columns = this->ColumnMetaData;
    auto iter = std::find_if(columns.begin(), columns.end(),
      [&aname](const std::tuple<std::string, std::string, int>& tuple)
      { return (std::get<0>(tuple) == aname); });
    return iter != columns.end() ? static_cast<vtkIdType>(std::distance(columns.begin(), iter))
                                 : -1;
  }

  std::string GetOriginalArrayName(const std::string& aname) const
  {
    auto iter = this->ColumnIndexMap.find(aname);
    if (iter != this->ColumnIndexMap.end())
    {
      const auto& tuple = this->ColumnMetaData[iter->second];
      return !std::get<1>(tuple).empty() ? std::get<1>(tuple) : aname;
    }
    return aname;
  }

  vtkTable* GetDataObject(vtkIdType blockId)
  {
    CacheType::iterator iter = this->CachedBlocks.find(blockId);
    if (iter != this->CachedBlocks.end())
    {
      iter->second.RecentUseTime.Modified();
      this->MostRecentlyAccessedBlock = blockId;
      return iter->second.Dataobject.GetPointer();
    }
    return nullptr;
  }

  bool OrderByNames(vtkAbstractArray* a1, vtkAbstractArray* a2)
  {
    std::vector<std::string> firstOrder = { "vtkBlockNameIndices", "vtkOriginalProcessIds",
      "vtkCompositeIndexArray", "vtkOriginalIndices", "vtkOriginalCellIds", "vtkOriginalPointIds",
      "vtkOriginalRowIds", "Structured Coordinates" };
    std::vector<std::string> lastOrder = { "FieldData: " };

    if (this->OrderColumnsByList && !this->OrderedColumnList.empty())
    {
      firstOrder.insert(
        firstOrder.end(), this->OrderedColumnList.begin(), this->OrderedColumnList.end());
    }

    std::string a1Name = a1->GetName() ? a1->GetName() : "";
    std::string a2Name = a2->GetName() ? a2->GetName() : "";
    unsigned int a1Index = VTK_INT_MAX, a2Index = VTK_INT_MAX;

    auto findFirstOrderOccurance = [&firstOrder](const std::string& name, unsigned int& index)
    {
      for (unsigned int i = 0; i < firstOrder.size(); i++)
      {
        if (index == VTK_INT_MAX && name == firstOrder[i])
        {
          index = i;
          break;
        }
      }
    };
    findFirstOrderOccurance(a1Name, a1Index);
    findFirstOrderOccurance(a2Name, a2Index);
    if (a1Index < a2Index)
    {
      return true;
    }
    if (a2Index < a1Index)
    {
      return false;
    }

    // we can reach here only when both array names are not in the "priority"
    // set or they are the same (which does happen, see BUG #9808).
    assert((a1Index == VTK_INT_MAX && a2Index == VTK_INT_MAX) || (a1Name == a2Name));

    auto findLastOrderOccurance = [&lastOrder](const std::string& name, unsigned int& index)
    {
      for (unsigned int i = 0; i < lastOrder.size(); i++)
      {
        if (name.find(lastOrder[i], 0) != std::string::npos)
        {
          index = i;
          break;
        }
      }
    };
    findLastOrderOccurance(a1Name, a1Index);
    findLastOrderOccurance(a2Name, a2Index);

    if (a1Index == a2Index)
    {
      std::transform(a1Name.begin(), a1Name.end(), a1Name.begin(), ::tolower);
      std::transform(a2Name.begin(), a2Name.end(), a2Name.begin(), ::tolower);
      return (a1Name < a2Name);
    }
    return a1Index > a2Index;
  }

  vtkTable* AddToCache(vtkIdType blockId, vtkTable* data, vtkIdType max)
  {
    CacheType::iterator iter = this->CachedBlocks.find(blockId);
    if (iter != this->CachedBlocks.end())
    {
      this->CachedBlocks.erase(iter);
    }

    if (static_cast<vtkIdType>(this->CachedBlocks.size()) == max)
    {
      // remove least-recent-used block.
      iter = this->CachedBlocks.begin();
      CacheType::iterator iterToRemove = this->CachedBlocks.begin();
      for (; iter != this->CachedBlocks.end(); ++iter)
      {
        if (iterToRemove->second.RecentUseTime > iter->second.RecentUseTime)
        {
          iterToRemove = iter;
        }
      }
      this->CachedBlocks.erase(iterToRemove);
    }

    CacheInfo info;
    vtkTable* clone = vtkTable::New();

    // sort columns for better usability.
    std::vector<vtkSmartPointer<vtkAbstractArray>> arrays;
    for (vtkIdType cc = 0; cc < data->GetNumberOfColumns(); cc++)
    {
      if (auto column = data->GetColumn(cc))
      {
        if (column->GetName() && strcmp(column->GetName(), "vtkBlockNameIndices") == 0)
        {
          arrays.push_back(::MapBlockNames(column,
            vtkStringArray::SafeDownCast(data->GetFieldData()->GetAbstractArray("vtkBlockNames"))));
        }
        else
        {
          arrays.push_back(column);
        }
      }
    }
    // if block-names are present in field-data, create an array
    std::sort(arrays.begin(), arrays.end(),
      [this](vtkAbstractArray* a1, vtkAbstractArray* a2) { return this->OrderByNames(a1, a2); });
    for (const auto& column : arrays)
    {
      clone->AddColumn(column);
    }
    info.Dataobject = clone;
    clone->FastDelete();
    info.RecentUseTime.Modified();
    this->CachedBlocks[blockId] = info;
    this->MostRecentlyAccessedBlock = blockId;
    if (this->CachedBlocks.size() == 1)
    {
      this->UpdateColumnMetaData(clone);
    }
    return clone;
  }

  /**
   * Get Previous first cached block
   */
  std::pair<vtkIdType, CacheInfo> GetPreviousFirstCachedBlock()
  {
    return this->PreviousFirstCachedBlock;
  }

  /**
   * A convenient method to get an existing block. It returns either the most recently accessed
   * block or a cached block.
   */
  vtkTable* GetAnExistingBlock(vtkSpreadSheetView* self)
  {
    const auto mrbId = this->GetMostRecentlyAccessedBlock(self);
    if (auto table = this->GetDataObject(mrbId))
    {
      return table;
    }

    for (const auto& cinfo : this->CachedBlocks)
    {
      if (cinfo.second.Dataobject != nullptr)
      {
        return cinfo.second.Dataobject;
      }
    }
    return nullptr;
  }

  /**
   * A convenient method to get some block. It returns either the most recently accessed block
   * or a cached block, if possible. This method will avoid a fetch unless needed.
   */
  vtkTable* GetSomeBlock(vtkSpreadSheetView* self)
  {
    vtkTable* table = this->GetAnExistingBlock(self);
    return table ? table : self->FetchBlock(this->GetMostRecentlyAccessedBlock(self));
  }

  vtkIdType MostRecentlyAccessedBlock;
  vtkWeakPointer<vtkSpreadSheetRepresentation> ActiveRepresentation;
  vtkCommand* Observer;

  std::set<std::string> HiddenColumnsByName;
  std::set<std::string> HiddenColumnsByLabel;

  std::vector<std::string> OrderedColumnList;
  bool OrderColumnsByList = false;
};

namespace
{
void FetchRMI(void* localArg, void* remoteArg, int remoteArgLength, int)
{
  assert(remoteArgLength == sizeof(vtkTypeUInt64) * 2);
  (void)remoteArgLength;

  auto arg = reinterpret_cast<vtkTypeUInt64*>(remoteArg);
  vtkSpreadSheetView* self = reinterpret_cast<vtkSpreadSheetView*>(localArg);
  if (self->GetIdentifier() == arg[0])
  {
    self->FetchBlockCallback(static_cast<vtkIdType>(arg[1]));
  }
}

unsigned long vtkCountNumberOfRows(vtkDataObject* dobj)
{
  vtkTable* table = vtkTable::SafeDownCast(dobj);
  if (table)
  {
    return table->GetNumberOfRows();
  }
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(dobj);
  if (cd)
  {
    unsigned long count = 0;
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      count += vtkCountNumberOfRows(iter->GetCurrentDataObject());
    }
    iter->Delete();
    return count;
  }
  return 0;
}

vtkAlgorithmOutput* vtkGetDataProducer(vtkSpreadSheetView* self, vtkSpreadSheetRepresentation* repr)
{
  if (repr)
  {
    if (self->GetShowExtractedSelection())
    {
      return repr->GetExtractedDataProducer();
    }
    else
    {
      return repr->GetDataProducer();
    }
  }
  return nullptr;
}

#if 0 // Its usage is commented out below.
  vtkAlgorithmOutput* vtkGetSelectionProducer(
    vtkSpreadSheetView* self, vtkSpreadSheetRepresentation* repr)
    {
    if (repr)
      {
      if (self->GetShowExtractedSelection())
        {
        return nullptr;
        }
      return repr->GetSelectionProducer();
      }
    return nullptr;
    }
#endif
}

vtkStandardNewMacro(vtkSpreadSheetView);
//----------------------------------------------------------------------------
vtkSpreadSheetView::vtkSpreadSheetView()
  : Superclass(/*create_render_window=*/false)
  , TableStreamer(vtkSortedTableStreamer::New())
  , TableSelectionMarker(vtkMarkSelectedRows::New())
  , ReductionFilter(vtkReductionFilter::New())
  , DeliveryFilter(vtkClientServerMoveData::New())
  , NumberOfRows(0)
  , CRMICallbackTag(0)
  , PRMICallbackTag(0)
  , Identifier(0)
  , Internals(new vtkSpreadSheetView::vtkInternals())
  , SomethingUpdated(false)
  , FieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS)
{
  this->ReductionFilter->SetController(vtkMultiProcessController::GetGlobalController());
  this->ReductionFilter->SetPostGatherHelper(vtkNew<SpreadSheetViewMergeTables>().GetPointer());
  this->DeliveryFilter->SetOutputDataType(VTK_TABLE);
  this->ReductionFilter->SetInputConnection(this->TableStreamer->GetOutputPort());

  this->Internals->MostRecentlyAccessedBlock = -1;
  this->Internals->Observer =
    vtkMakeMemberFunctionCommand(*this, &vtkSpreadSheetView::OnRepresentationUpdated);

  auto session = this->GetSession();
  assert(session);
  if (auto cController = session->GetController(vtkPVSession::CLIENT))
  {
    this->CRMICallbackTag = cController->AddRMICallback(::FetchRMI, this, FETCH_BLOCK_TAG);
  }
  if (auto pController = vtkMultiProcessController::GetGlobalController())
  {
    this->PRMICallbackTag = pController->AddRMICallback(::FetchRMI, this, FETCH_BLOCK_TAG);
  }
}

//----------------------------------------------------------------------------
vtkSpreadSheetView::~vtkSpreadSheetView()
{
  auto session = this->GetSession();
  if (auto cController = session ? session->GetController(vtkPVSession::CLIENT) : nullptr)
  {
    cController->RemoveRMICallback(this->CRMICallbackTag);
    this->CRMICallbackTag = 0;
  }
  if (auto pController = session ? vtkMultiProcessController::GetGlobalController() : nullptr)
  {
    pController->RemoveRMICallback(this->PRMICallbackTag);
    this->PRMICallbackTag = 0;
  }

  this->TableStreamer->Delete();
  this->TableSelectionMarker->Delete();
  this->ReductionFilter->Delete();
  this->DeliveryFilter->Delete();

  this->Internals->Observer->Delete();
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::SetShowExtractedSelection(bool val)
{
  if (val != this->ShowExtractedSelection)
  {
    this->ShowExtractedSelection = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::HideColumnByName(const char* columnName)
{
  if (columnName)
  {
    auto& internals = *this->Internals;
    internals.HiddenColumnsByName.insert(columnName);
  }
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::ClearHiddenColumnsByName()
{
  auto& internals = *this->Internals;
  internals.HiddenColumnsByName.clear();
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::IsColumnHiddenByName(const char* columnName)
{
  const auto& internals = *this->Internals;
  return columnName
    ? internals.HiddenColumnsByName.find(columnName) != internals.HiddenColumnsByName.end()
    : true;
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::HideColumnByLabel(const char* columnLabel)
{
  if (columnLabel)
  {
    auto& internals = *this->Internals;
    internals.HiddenColumnsByLabel.insert(columnLabel);
  }
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::ClearHiddenColumnsByLabel()
{
  auto& internals = *this->Internals;
  internals.HiddenColumnsByLabel.clear();
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::IsColumnHiddenByLabel(const std::string& columnLabel)
{
  const auto& internals = *this->Internals;
  return columnLabel.empty()
    ? true
    : internals.HiddenColumnsByLabel.find(columnLabel) != internals.HiddenColumnsByLabel.end();
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::ClearCache()
{
  this->Internals->ClearCache();
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::GetColumnVisibility(vtkIdType index)
{
  auto name = this->GetColumnName(index);
  auto label = this->GetColumnLabel(name);
  return !(this->IsColumnInternal(name) || this->IsColumnHiddenByName(name) ||
    this->IsColumnHiddenByLabel(label));
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::IsColumnInternal(vtkIdType index)
{
  return this->IsColumnInternal(this->GetColumnName(index));
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::IsColumnInternal(const char* columnName)
{
  return (columnName == nullptr || strcmp(columnName, "__vtkIsSelected__") == 0) ||
      (std::strstr(columnName, "__vtkValidMask__") == columnName)
    ? true
    : false;
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::OrderColumnsByList(bool val)
{
  // Avoid unecessary called to ClearCache()
  if (this->Internals->OrderColumnsByList == val)
  {
    return;
  }

  this->Internals->OrderColumnsByList = val;
  this->ClearCache();
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::InitializeOrderedColumnList()
{
  // We only want visible column
  const char* unwantedColumnNames[2] = { "__vtkValidMask__", "vtkOriginalIndices" };

  for (vtkIdType index = 0; index < this->GetNumberOfColumns(); index++)
  {
    std::string colName = this->GetColumnName(index);
    if (colName.find(unwantedColumnNames[0]) == std::string::npos &&
      colName.find(unwantedColumnNames[1]) == std::string::npos)
    {
      this->Internals->OrderedColumnList.push_back(colName);
    }
  }

  std::sort(this->Internals->OrderedColumnList.begin(), this->Internals->OrderedColumnList.end());
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkSpreadSheetView::GetOrderedColumnList()
{
  return this->Internals->OrderedColumnList;
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::SetOrderedColumnList(std::vector<std::string> list)
{
  this->Internals->OrderedColumnList = list;
  this->ClearCache();
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::ClearOrderedColumnList()
{
  this->Internals->OrderedColumnList.clear();
  this->ClearCache();
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::Update()
{
  vtkSpreadSheetRepresentation* prev = this->Internals->ActiveRepresentation;
  vtkSpreadSheetRepresentation* cur = nullptr;
  for (int cc = 0; cc < this->GetNumberOfRepresentations(); cc++)
  {
    vtkSpreadSheetRepresentation* repr =
      vtkSpreadSheetRepresentation::SafeDownCast(this->GetRepresentation(cc));
    if (repr && repr->GetVisibility())
    {
      cur = repr;
      break;
    }
  }

  if (prev != cur)
  {
    if (prev)
    {
      prev->RemoveObserver(this->Internals->Observer);
    }
    if (cur)
    {
      cur->AddObserver(vtkCommand::UpdateDataEvent, this->Internals->Observer);
    }
    this->Internals->ActiveRepresentation = cur;
    this->ClearCache();
  }

  this->SomethingUpdated = false;

  // The code below is the same with vtkPVView::Update() except for the addition of a block to the
  // cache if none exists. This is needed to ensure that if a progress event is attempted to be
  // handled, because of a WrongTagEvent during another Receive call message from the client's
  // controller in any view that is present, the progress can query some block from the cache
  // instead of trying to fetch it from the server which will result in a deadlock. After finishing
  // the Update(), a StillRender/InteractiveRender will be called which will ensure that the cache
  // is cleared and the correct block is fetched from the server.

  vtkVLogScopeF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "%s: update view", this->GetLogName().c_str());

  // Propagate update time.
  const int num_reprs = this->GetNumberOfRepresentations();
  const auto view_time = this->GetViewTime();
  for (int cc = 0; cc < num_reprs; cc++)
  {
    if (auto pvrepr = vtkPVDataRepresentation::SafeDownCast(this->GetRepresentation(cc)))
    {
      // Pass the view time information to the representation
      if (this->GetViewTimeValid())
      {
        pvrepr->SetUpdateTime(view_time);
      }
      else
      {
        pvrepr->ResetUpdateTime();
      }
    }
  }

  vtkTimerLog::MarkStartEvent("vtkPVView::Update");
  const int count = this->CallProcessViewRequest(
    vtkPVView::REQUEST_UPDATE(), this->RequestInformation, this->ReplyInformationVector);
  vtkTimerLog::MarkEndEvent("vtkPVView::Update");

  // Add a block to the cache if it doesn't exist.
  if (!this->Internals->GetAnExistingBlock(this))
  {
    // Add the previous first cached block to the cache if it exists.
    auto previousFirstCachedBlock = this->Internals->GetPreviousFirstCachedBlock();
    vtkSmartPointer<vtkTable> table = previousFirstCachedBlock.second.Dataobject;
    if (table)
    {
      this->Internals->AddToCache(previousFirstCachedBlock.first, table, 10);
    }
    else // Add an empty block to the cache.
    {
      table = vtkSmartPointer<vtkTable>::New();
      this->Internals->AddToCache(0, table, 10);
    }
  }

  // exchange information about representations that are time-dependent.
  // this goes from data-server-root to client and render-server.
  if (count)
  {
    this->SynchronizeRepresentationTemporalPipelineStates();
  }

  this->UpdateTimeStamp.Modified();
}

//----------------------------------------------------------------------------
int vtkSpreadSheetView::StreamToClient()
{
  vtkSpreadSheetRepresentation* cur = this->Internals->ActiveRepresentation;
  if (cur == nullptr)
  {
    if (this->NumberOfRows > 0)
    {
      // BUG #13231: It implies that we had some data in the previous render and
      // we don't have it anymore. We need to trigger "UpdateDataEvent" to
      // ensure that the UI can update the rows and columns correctly.
      this->NumberOfRows = 0;
      this->InvokeEvent(vtkCommand::UpdateDataEvent);
    }
    return 0;
  }

  vtkTypeUInt64 num_rows = 0;

  // From the active representation obtain the data/selection producers that
  // need to be streamed to the client.
  vtkAlgorithmOutput* dataPort = vtkGetDataProducer(this, cur);
  //  vtkAlgorithmOutput* selectionPort = vtkGetSelectionProducer(this, cur);

  this->TableSelectionMarker->SetInputConnection(0, dataPort);
  this->TableSelectionMarker->SetInputConnection(1, cur->GetExtractedDataProducer());
  this->TableStreamer->SetInputConnection(this->TableSelectionMarker->GetOutputPort());
  if (dataPort)
  {
    dataPort->GetProducer()->Update();
    this->DeliveryFilter->SetInputConnection(this->ReductionFilter->GetOutputPort());
    num_rows =
      vtkCountNumberOfRows(dataPort->GetProducer()->GetOutputDataObject(dataPort->GetIndex()));
  }
  else
  {
    this->DeliveryFilter->RemoveAllInputs();
  }

  this->AllReduce(num_rows, num_rows, vtkCommunicator::SUM_OP);

  if (this->NumberOfRows != static_cast<vtkIdType>(num_rows))
  {
    this->SomethingUpdated = true;
  }
  this->NumberOfRows = num_rows;
  if (this->SomethingUpdated)
  {
    this->ClearCache();
    this->InvokeEvent(vtkCommand::UpdateDataEvent);
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::OnRepresentationUpdated()
{
  this->ClearCache();
  this->SomethingUpdated = true;
}

//----------------------------------------------------------------------------
vtkTable* vtkSpreadSheetView::FetchBlock(vtkIdType blockindex)
{
  vtkTable* block = this->Internals->GetDataObject(blockindex);
  if (!block)
  {
    block = this->FetchBlockCallback(blockindex);
    // use the block returned from the AddToCache since that is cleaned up
    // to have columns in correct order.
    block = this->Internals->AddToCache(blockindex, block, 10);
    this->InvokeEvent(vtkCommand::UpdateEvent, &blockindex);
  }
  return block;
}

//----------------------------------------------------------------------------
vtkTable* vtkSpreadSheetView::FetchBlockCallback(vtkIdType blockindex)
{
  // Sanity Check
  if (!this->Internals->ActiveRepresentation)
  {
    return nullptr;
  }

  // cout << "FetchBlockCallback" << endl;
  vtkTypeUInt64 data[2] = { this->Identifier, static_cast<vtkTypeUInt64>(blockindex) };
  if (auto dController = this->GetSession()->GetController(vtkPVSession::DATA_SERVER_ROOT))
  {
    dController->TriggerRMIOnAllChildren(data, sizeof(vtkTypeUInt64) * 2, FETCH_BLOCK_TAG);
  }
  auto pController = vtkMultiProcessController::GetGlobalController();
  if (pController && pController->GetLocalProcessId() == 0 &&
    pController->GetNumberOfProcesses() > 1)
  {
    pController->TriggerRMIOnAllChildren(data, sizeof(vtkTypeUInt64) * 2, FETCH_BLOCK_TAG);
  }

  this->TableStreamer->SetBlock(blockindex);
  this->TableStreamer->SetShowFieldData(this->ShowFieldData);
  this->TableStreamer->Modified();
  this->TableSelectionMarker->SetFieldAssociation(this->FieldAssociation);
  this->ReductionFilter->Modified();
  this->DeliveryFilter->Modified();
  this->DeliveryFilter->Update();
  return vtkTable::SafeDownCast(this->DeliveryFilter->GetOutput());
}

//----------------------------------------------------------------------------
vtkIdType vtkSpreadSheetView::GetNumberOfColumns()
{
  return this->Internals->GetNumberOfColumns(this);
}

//----------------------------------------------------------------------------
vtkIdType vtkSpreadSheetView::GetNumberOfRows()
{
  return this->NumberOfRows;
}

//----------------------------------------------------------------------------
const char* vtkSpreadSheetView::GetColumnName(vtkIdType index)
{
  return this->Internals->GetColumnName(index, this);
}

//----------------------------------------------------------------------------
std::string vtkSpreadSheetView::GetColumnLabel(vtkIdType index)
{
  return this->GetColumnLabel(this->GetColumnName(index));
}

//----------------------------------------------------------------------------
std::string vtkSpreadSheetView::GetColumnLabel(const char* name)
{
  if (name == nullptr || this->IsColumnInternal(name))
  {
    return std::string();
  }

  bool cleaned = false;
  auto cleanedname = ::get_userfriendly_name(name, this, &cleaned);
  return cleaned ? cleanedname : this->Internals->GetOriginalArrayName(name);
}

//----------------------------------------------------------------------------
vtkIdType vtkSpreadSheetView::GetColumnByName(const char* columnName)
{
  if (columnName == nullptr)
  {
    return -1;
  }

  return this->Internals->GetColumnByName(columnName);
}

//----------------------------------------------------------------------------
vtkVariant vtkSpreadSheetView::GetValue(vtkIdType row, vtkIdType col)
{
  vtkIdType blockSize = this->TableStreamer->GetBlockSize();
  vtkIdType blockIndex = row / blockSize;
  vtkTable* block = this->FetchBlock(blockIndex);
  vtkIdType blockOffset = row - (blockIndex * blockSize);
  return block->GetValue(blockOffset, col);
}

//----------------------------------------------------------------------------
vtkVariant vtkSpreadSheetView::GetValueByName(vtkIdType row, const char* columnName)
{
  vtkIdType blockSize = this->TableStreamer->GetBlockSize();
  vtkIdType blockIndex = row / blockSize;
  vtkTable* block = this->FetchBlock(blockIndex);
  vtkIdType blockOffset = row - (blockIndex * blockSize);
  return block->GetValueByName(blockOffset, columnName);
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::IsRowSelected(vtkIdType row)
{
  vtkIdType blockSize = this->TableStreamer->GetBlockSize();
  vtkIdType blockIndex = row / blockSize;
  vtkTable* block = this->FetchBlock(blockIndex);
  vtkIdType blockOffset = row - (blockIndex * blockSize);
  vtkCharArray* vtkIsSelected =
    vtkCharArray::SafeDownCast(block->GetColumnByName("__vtkIsSelected__"));
  if (vtkIsSelected)
  {
    return vtkIsSelected->GetValue(blockOffset) == 1;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::IsAvailable(vtkIdType row)
{
  vtkIdType blockSize = this->TableStreamer->GetBlockSize();
  vtkIdType blockIndex = row / blockSize;
  return this->Internals->GetDataObject(blockIndex) != nullptr;
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::IsDataValid(vtkIdType row, vtkIdType col)
{
  vtkIdType blockSize = this->TableStreamer->GetBlockSize();
  vtkIdType blockIndex = row / blockSize;
  vtkTable* block = this->FetchBlock(blockIndex);
  vtkIdType blockOffset = row - (blockIndex * blockSize);

  if (auto columnName = this->GetColumnName(col))
  {
    const std::string maskColumnName("__vtkValidMask__" + std::string(columnName));
    auto maskArray =
      vtkUnsignedCharArray::SafeDownCast(block->GetColumnByName(maskColumnName.c_str()));
    if (maskArray && maskArray->GetNumberOfTuples() > blockOffset &&
      maskArray->GetTypedComponent(blockOffset, 0) == static_cast<unsigned char>(0))
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSpreadSheetView::Export(vtkCSVExporter* exporter)
{
  if (!exporter->Open())
  {
    return false;
  }
  this->ClearCache();

  if (auto activeRepr = this->Internals->ActiveRepresentation)
  {
    vtkIdType blockSize = this->TableStreamer->GetBlockSize();
    vtkIdType numBlocks = (this->GetNumberOfRows() / blockSize) + 1;
    for (vtkIdType cc = 0; cc < numBlocks; cc++)
    {
      if (vtkTable* block = this->FetchBlock(cc))
      {
        auto* rowData = block->GetRowData();
        if (cc == 0)
        {
          // update column labels; this ensures that all the columns have same
          // names as the spreadsheet view.
          for (vtkIdType idx = 0; idx < rowData->GetNumberOfArrays(); ++idx)
          {
            if (auto array = rowData->GetAbstractArray(idx))
            {
              // note: internal columns get nullptr label which the exporter
              // skips.
              auto name = array->GetName();
              auto label = this->GetColumnLabel(name);
              if (this->IsColumnInternal(name) || this->IsColumnHiddenByName(name) ||
                this->IsColumnHiddenByLabel(label))
              {
                exporter->SetColumnLabel(array->GetName(), nullptr);
              }
              else
              {
                // we don't use the label since it is same for all components in
                // the array, instead we only use user friendly version of the
                // array name.
                exporter->SetColumnLabel(array->GetName(), ::get_userfriendly_name(name, this));
              }
            }
          }
          exporter->WriteHeader(rowData);
        }
        exporter->WriteData(rowData);
      }
    }
  }
  exporter->Close();
  return true;
}

//***************************************************************************
// Forwarded to vtkSortedTableStreamer.
//----------------------------------------------------------------------------
void vtkSpreadSheetView::SetColumnNameToSort(const char* name)
{
  this->TableStreamer->SetColumnNameToSort(name);
  this->ClearCache();
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::SetInvertSortOrder(bool val)
{
  this->TableStreamer->SetInvertOrder(val ? 1 : 0);
  this->ClearCache();
}

//----------------------------------------------------------------------------
void vtkSpreadSheetView::SetBlockSize(vtkIdType val)
{
  this->TableStreamer->SetBlockSize(val);
  this->ClearCache();
}

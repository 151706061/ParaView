/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataDeliveryManagerInternals.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkPVDataDeliveryManagerInternals_h
#define vtkPVDataDeliveryManagerInternals_h
#ifndef __WRAP__

#include "vtkDataObject.h"  // for vtkDataObject
#include "vtkInformation.h" // for vtkInformation
#include "vtkNew.h"         // for vtkNew
#include "vtkPVDataDeliveryManager.h"
#include "vtkPVDataRepresentation.h" // for vtkPVDataRepresentation
#include "vtkPVTrivialProducer.h"    // for vtkPVTrivialProducer
#include "vtkSmartPointer.h"         // for vtkSmartPointer
#include "vtkWeakPointer.h"          // for vtkWeakPointer

#include <cassert> // for assert
#include <map>     // for std::map
#include <numeric> // for std::accumulate
#include <utility> // for std::pair

class vtkPVDataDeliveryManager::vtkInternals
{
  friend class vtkItem;
  std::map<int, vtkSmartPointer<vtkDataObject> > EmptyDataObjectTypes;

  // This helps us avoid creating new instances of various data object types to use as
  // empty datasets. Instead, we build a map and keep reusing objects.
  vtkDataObject* GetEmptyDataObject(vtkDataObject* ref)
  {
    if (ref)
    {
      auto iter = this->EmptyDataObjectTypes.find(ref->GetDataObjectType());
      if (iter != this->EmptyDataObjectTypes.end())
      {
        return iter->second;
      }
      else
      {
        vtkSmartPointer<vtkDataObject> clone;
        clone.TakeReference(ref->NewInstance());
        this->EmptyDataObjectTypes[ref->GetDataObjectType()] = clone;
        return clone;
      }
    }
    return nullptr;
  }

public:
  struct vtkRepresentedData
  {
    // Data object produced by the representation.
    vtkSmartPointer<vtkDataObject> DataObject;

    // Data object available after delivery to the "rendering" node.
    std::map<int, vtkSmartPointer<vtkDataObject> > DeliveredDataObjects;

    // Some useful meta-data.
    vtkMTimeType TimeStamp{ 0 };
    vtkMTimeType ActualMemorySize{ 0 };

    // Arbitrary meta-data container.
    vtkSmartPointer<vtkInformation> Information;
  };

  class vtkItem
  {
    vtkNew<vtkPVTrivialProducer> Producer;

    // Store of data generated by the representation for rendering.
    // The store keeps data at various stages of the pipeline along with
    // relevant cache, as appropriate.
    std::map<double, vtkRepresentedData> Data;

    vtkMTimeType TimeStamp{ 0 };

  public:
    vtkItem() {}

    void ClearCache() { this->Data.clear(); }

    void SetDataObject(vtkDataObject* data, vtkInternals* helper, double cacheKey)
    {
      auto& store = this->Data[cacheKey];
      if (data)
      {
        store.DataObject.TakeReference(data->NewInstance());
        store.DataObject->ShallowCopy(data);
      }
      else
      {
        store.DataObject = nullptr;
      }

      store.DeliveredDataObjects.clear();
      store.ActualMemorySize = data ? data->GetActualMemorySize() : 0;
      // This method gets called when data is entirely changed. That means that any
      // data we may have delivered or redistributed would also be obsolete.
      // Hence we reset the `Producer` as well. This avoids #2160.

      // explanation for using a clone: typically, the Producer is connected by the
      // representation to a rendering pipeline e.g. the mapper. As that could be the
      // case, we need to ensure the producer's input is cleaned too. Setting simply nullptr
      // could confuse the mapper and hence we setup a data object of the same type as the data.
      // we could simply set the data too, but that can lead to other confusion as the mapper should
      // never directly see the representation's data.
      this->Producer->SetOutput(helper->GetEmptyDataObject(data));

      vtkTimeStamp ts;
      ts.Modified();
      store.TimeStamp = ts;
      this->TimeStamp = ts;
    }

    void SetActualMemorySize(unsigned long size, double cacheKey)
    {
      auto& store = this->Data[cacheKey];
      store.ActualMemorySize = size;
    }

    unsigned long GetActualMemorySize(double cacheKey) const
    {
      auto iter = this->Data.find(cacheKey);
      return iter != this->Data.end() ? iter->second.ActualMemorySize : 0;
    }

    vtkDataObject* GetDeliveredDataObject(int dataKey, double cacheKey) const
    {
      try
      {
        const auto& store = this->Data.at(cacheKey);
        return store.DeliveredDataObjects.at(dataKey);
      }
      catch (std::out_of_range&)
      {
        return nullptr;
      }
    }

    void SetDeliveredDataObject(int dataKey, double cacheKey, vtkDataObject* data)
    {
      auto& store = this->Data[cacheKey];
      store.DeliveredDataObjects[dataKey] = data;
    }

    vtkPVTrivialProducer* GetProducer(int dataKey, double cacheKey)
    {
      vtkDataObject* prev = this->Producer->GetOutputDataObject(0);
      vtkDataObject* cur = this->GetDeliveredDataObject(dataKey, cacheKey);
      this->Producer->SetOutput(cur);
      if (cur != prev && cur != nullptr)
      {
        // this is only needed to overcome a bug in the mapper where they are
        // using input's mtime incorrectly.
        cur->Modified();
      }
      return this->Producer.GetPointer();
    }

    vtkDataObject* GetDataObject(double cacheKey) const
    {
      auto iter = this->Data.find(cacheKey);
      return iter != this->Data.end() ? iter->second.DataObject.GetPointer() : nullptr;
    }

    vtkMTimeType GetTimeStamp(double cacheKey) const
    {
      auto iter = this->Data.find(cacheKey);
      return iter != this->Data.end() ? iter->second.TimeStamp : vtkMTimeType{ 0 };
    }

    vtkInformation* GetPieceInformation(double cacheKey)
    {
      auto& store = this->Data[cacheKey];
      if (store.Information == nullptr)
      {
        store.Information = vtkSmartPointer<vtkInformation>::New();
      }
      return store.Information;
    }

    vtkMTimeType GetTimeStamp() const { return this->TimeStamp; }
    vtkMTimeType GetDeliveryTimeStamp(int dataKey, double cacheKey) const
    {
      if (auto dobj = this->GetDeliveredDataObject(dataKey, cacheKey))
      {
        return dobj->GetMTime();
      }
      return vtkMTimeType{ 0 };
    }
  };

  // First is repr unique id, second is the input port.
  typedef std::pair<unsigned int, int> ReprPortType;
  typedef std::map<ReprPortType, std::pair<vtkItem, vtkItem> > ItemsMapType;

  // Keep track of representation and its uid.
  typedef std::map<unsigned int, vtkWeakPointer<vtkPVDataRepresentation> > RepresentationsMapType;

  vtkItem* GetItem(unsigned int index, bool use_second, int port, bool create_if_needed = false)
  {
    ReprPortType key(index, port);
    ItemsMapType::iterator items = this->ItemsMap.find(key);
    if (items != this->ItemsMap.end())
    {
      return use_second ? &(items->second.second) : &(items->second.first);
    }
    else if (create_if_needed)
    {
      std::pair<vtkItem, vtkItem>& itemsPair = this->ItemsMap[key];
      return use_second ? &(itemsPair.second) : &(itemsPair.first);
    }
    return nullptr;
  }

  vtkItem* GetItem(
    vtkPVDataRepresentation* repr, bool use_second, int port, bool create_if_needed = false)
  {
    return this->GetItem(repr->GetUniqueIdentifier(), use_second, port, create_if_needed);
  }

  int GetNumberOfPorts(vtkPVDataRepresentation* repr)
  {
    const auto id = repr->GetUniqueIdentifier();
    return std::accumulate(this->ItemsMap.begin(), this->ItemsMap.end(), 0,
      [&id](int sum, const ItemsMapType::value_type& item_pair) {
        return sum + (item_pair.first.first == id ? 1 : 0);
      });
  }

  unsigned long GetVisibleDataSize(bool use_second_if_available, vtkPVDataDeliveryManager* dmgr)
  {
    unsigned long size = 0;
    ItemsMapType::iterator iter;
    for (iter = this->ItemsMap.begin(); iter != this->ItemsMap.end(); ++iter)
    {
      const ReprPortType& key = iter->first;
      if (!this->IsRepresentationVisible(key.first))
      {
        // skip hidden representations.
        continue;
      }

      auto repr = this->RepresentationsMap[key.first];
      assert(repr != nullptr);
      const double cacheKey = dmgr->GetCacheKey(repr);

      if (use_second_if_available && iter->second.second.GetDataObject(cacheKey))
      {
        size += iter->second.second.GetActualMemorySize(cacheKey);
      }
      else
      {
        size += iter->second.first.GetActualMemorySize(cacheKey);
      }
    }
    return size;
  }

  bool IsRepresentationVisible(unsigned int id) const
  {
    RepresentationsMapType::const_iterator riter = this->RepresentationsMap.find(id);
    return (riter != this->RepresentationsMap.end() && riter->second.GetPointer() != nullptr &&
      riter->second->GetVisibility());
  }

  void ClearCache(vtkPVDataRepresentation* repr) { this->ClearCache(repr->GetUniqueIdentifier()); }
  void ClearCache(unsigned int id)
  {
    for (auto& ipair : this->ItemsMap)
    {
      if (ipair.first.first == id)
      {
        ipair.second.first.ClearCache();
        ipair.second.second.ClearCache();
      }
    }
  }

  ItemsMapType ItemsMap;
  RepresentationsMapType RepresentationsMap;
};

#endif // __WRAP__
#endif

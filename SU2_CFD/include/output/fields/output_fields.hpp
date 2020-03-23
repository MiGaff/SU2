#pragma once

#include "../../../Common/include/toolboxes/CIndexedMap.hpp"
#include "../../../Common/include/datatype_structure.hpp"
#include "../../../Common/include/mpi_structure.hpp"
#include "../../../Common/include/toolboxes/CExpressionParser.hpp"

/** \brief Enum to identify the screen output format. */
enum class ScreenOutputFormat {
  INTEGER,         /*!< \brief Integer format. Example: 34 */
  FIXED,           /*!< \brief Format with fixed precision for floating point values. Example: 344.54  */
  SCIENTIFIC,      /*!< \brief Scientific format for floating point values. Example: 3.4454E02 */
  PERCENT          /*!< \brief Format with fixed precision for floating point values with a % signs. Example: 99.52% */
};

/** \brief Enum to identify the screen/history field type. */
enum class FieldType {
  RESIDUAL,         /*!< \brief A user-defined residual field type*/
  AUTO_RESIDUAL,    /*!< \brief An automatically generated residual field type */
  COEFFICIENT,      /*!< \brief User defined coefficient field type  */
  AUTO_COEFFICIENT, /*!< \brief Automatically generated coefficient field type  */
  CUSTOM,           /*!< \brief Custom field */
  DEFAULT           /*!< \brief Default field type */
};


class COutputField {

public:
  /*! \brief The name of the field, i.e. the name that is printed in the screen or file header.*/
  std::string         fieldName = "";
  /*! \brief The group this field belongs to. */
  std::string         outputGroup  ="";
  /*! \brief String containing the description of the field */
  std::string         description = "";
  /*! \brief The field type*/
  FieldType    fieldType = FieldType::DEFAULT;

  CExpressionParser expParser;

  packToken* tokenRef = nullptr;

  COutputField() = default;

  COutputField(std::string fieldName_, FieldType type_, std::string OutputGroup_, std::string description_)
    : fieldName(std::move(fieldName_)), outputGroup(std::move(OutputGroup_)),  description(std::move(description_)),
      fieldType(type_)
  {}
};

/** \brief Structure to store information for a history output field.
 *
 *  The stored information is printed to the history file and to screen.
 * Each individual instance represents a single field (i.e. column) in the history file or on screen.
 */
class HistoryOutputField final : public COutputField {
public:

  /*! \brief The value of the field. */
  su2double           value = 0.0;
  /*! \brief The format that is used to print this value to screen. */
  ScreenOutputFormat  screenFormat = ScreenOutputFormat::FIXED;

  /*! \brief Default constructor. */
  HistoryOutputField() = default;
  /*! \brief Constructor to initialize all members. */
  HistoryOutputField(std::string fieldName_, ScreenOutputFormat screenFormat_, std::string OutputGroup_,
                     FieldType fieldType_, std::string description_): COutputField(fieldName_, fieldType_, OutputGroup_, description_),
     value(0.0), screenFormat(screenFormat_)
    {}

};

/** \brief Structure to store information for a volume output field.
 *
 *  The stored information is used to create the volume solution file.
 */
class VolumeOutputField : public COutputField {

public:
  /*! \brief This value identifies the position of the values of this field at each node in the ::Local_Data array. */
  short       offset;

  /*! \brief Default constructor. */
  VolumeOutputField () {}
  /*! \brief Constructor to initialize all members. */
  VolumeOutputField(std::string fieldName_, int offset_, std::string volumeOutputGroup_, std::string description_):
     COutputField(fieldName_, FieldType::DEFAULT, volumeOutputGroup_, description_),
    offset(std::move(offset_)){}
};

template <class C>
class COutFieldCollection : public CIndexedMap<std::string, C> {

public:
  using CurrentType     = CIndexedMap<std::string, C>;
  using InsertionVector = typename CurrentType::InsertionVector;
  using KeyVector       = typename CurrentType::KeyVector;
  using KeyType         = typename CurrentType::KeyType;
  using ItemType        = typename CurrentType::ItemType;
  using Map             = typename CurrentType::Map;

protected:
  GlobalScope outFieldScope;
  using CurrentType::insertionVector;

private:

  static bool findFieldWithType(const FieldType& type, const typename Map::iterator& mapIndex){
    return (mapIndex->second.fieldType == type);
  }


public:

  TokenMap& GetScope() { return outFieldScope; }

  /*!
   * \brief Get fields by using a list of keys
   * \param[in] fieldKey   - A vector of keys
   * \param[out] notFound  - Vector containing a list of keys that have not been found
   * \param[in] refVector  - Vector containing references to entries of an indexed map
   * \param[in] searchGroup - Boolean indicating whether also group names should be also included in the search
   * \return Vector containing references to the entries with the specified keys and/or group names
   */
  static InsertionVector GetFieldsByKey(const KeyVector& fieldKey, KeyVector& notFound,
                                        const InsertionVector& refVector, bool searchGroup = false) {
    auto findFieldWithName = [&searchGroup](const KeyType& key, const typename Map::iterator& mapIndex){
      return (key == mapIndex->first || (searchGroup && (key == mapIndex->second.outputGroup)));
    };
    return GetReferences(fieldKey, notFound, refVector, findFieldWithName);
  }

  /*!
   * \brief Get fields by using a list of keys
   * \param[in] fieldKey   - A vector of keys
   * \param[in] refVector  - Vector containing references to entries of an indexed map
   * \param[in] searchGroup - Boolean indicating whether also group names should be also included in the search
   * \return Vector containing references to the entries with the specified keys and/or group names
   */
  static InsertionVector GetFieldsByKey(const KeyVector& fieldKey, const InsertionVector& refVector,
                                        bool searchGroup = false) {
    KeyVector dummy;
    return GetFieldsByKey(fieldKey, dummy, refVector, searchGroup);
  }

  /*!
   * \brief Get fields by using a list of keys
   * \param[in] fieldKey   - A vector of keys
   * \param[out] notFound  - Vector containing a list of keys that have not been found
   * \param[in] searchGroup - Boolean indicating whether also group names should be also included in the search
   * \return Vector containing references to the entries with the specified keys and/or group names
   */
  InsertionVector GetFieldsByKey(const KeyVector& fieldKey, KeyVector& notFound, bool searchGroup = false) const {
    auto findFieldWithName = [&searchGroup](const KeyType& key, const typename Map::iterator& mapIndex){
      return (key == mapIndex->first || (searchGroup && (key == mapIndex->second.outputGroup)));
    };
    return CurrentType::GetReferences(fieldKey, notFound, insertionVector, findFieldWithName);
  }

  /*!
   * \brief Get fields by using a list of keys
   * \param[in] fieldKey   - A vector of keys
   * \param[in] searchGroup - Boolean indicating whether also group names should be also included in the search
   * \return Vector containing references to the entries with the specified keys and/or group names
   */
  InsertionVector GetFieldsByKey(const KeyVector& fieldKey, bool searchGroup = false) const {
    KeyVector dummy;
    return GetFieldsByKey(fieldKey, dummy, searchGroup);
  }

  /*!
   * \brief Get fields by using a list of group names
   * \param[in] groupList  - A vector of group names
   * \param[out] notFound  - Vector containing a list of group names that have not been found
   * \param[in] refVector  - Vector containing references to entries of an indexed map
   * \return Vector containing references to the entries with the specified group names
   */
  static InsertionVector GetFieldsByGroup(const std::vector<std::string>& groupList,
                                          std::vector<std::string>& notFound, const InsertionVector& refVector){
    auto findFieldWithGroup = [](const std::string& group, const typename Map::iterator& mapIndex){
      return (group == mapIndex->second.outputGroup);
    };
    return CurrentType::GetReferences(groupList, notFound, refVector, findFieldWithGroup);
  }

  /*!
   * \brief Get fields by using a list of group names
   * \param[in] groupList  - A vector of group names
   * \param[out] notFound  - Vector containing a list of group names that have not been found
   * \return Vector containing references to the entries with the specified group names
   */
  InsertionVector GetFieldsByGroup(const std::vector<std::string>& groupList, std::vector<std::string>& notFound){
    auto findFieldWithGroup = [](const std::string& group, const typename Map::iterator& mapIndex){
      return (group == mapIndex->second.outputGroup);
    };
    return CurrentType::GetReferences(groupList, notFound, insertionVector, findFieldWithGroup);
  }

  /*!
   * \brief Get fields by using a list of group names
   * \param[in] groupList  - A vector of group names
   * \return Vector containing references to the entries with the specified group names
   */
  InsertionVector GetFieldsByGroup(const std::vector<std::string>& groupList){
    std::vector<std::string> dummy;
    return GetFieldsByGroup(groupList, dummy);
  }

  /*!
   * \brief Get fields by using a list of field types
   * \param[in] groupList  - A vector of field types
   * \param[in] refVector  - Vector containing references to entries of an indexed map
   * \return Vector containing references to the entries with the specified field types
   */
  static InsertionVector GetFieldsByType(const std::vector<FieldType>& type, const InsertionVector& refVector){
    std::vector<FieldType> dummy;
    return CurrentType::GetReferences(type, dummy, refVector, findFieldWithType);
  }

  /*!
   * \brief Get fields by using a list of field types
   * \param[in] groupList  - A vector of field types
   * \return Vector containing references to the entries with the specified field types
   */
  InsertionVector GetFieldsByType(const std::vector<FieldType>& type) const {
    std::vector<FieldType> dummy;
    return CurrentType::GetReferences(type, dummy, insertionVector, findFieldWithType);
  }

};


class HistoryOutFieldCollection final : public COutFieldCollection<HistoryOutputField>{

public:
  void UpdateTokens(){

    auto customFields = GetFieldsByType({FieldType::CUSTOM});

    if (!customFields.empty()){
      for (const auto& field : insertionVector){
        if (field->second.fieldType != FieldType::CUSTOM){
          outFieldScope[field->first] = field->second.value;
        }
      }

      for (const auto& field : customFields){
        field->second.expParser.ExecCode();
        field->second.value = field->second.expParser.Eval(std::string("eval"));
      }
    }
  }

  /*!
   * \brief Set the value of specific field by using its key
   * \param[in] key   - The key of the field
   * \param[in] value - The new value for this field
   */
  void SetValueByKey(const KeyType& key, su2double value){
    map.find(key)->second.value = value;
  }

  /*!
   * \brief Set the value of specific field by using its index
   * \param[in] i     - The index of the field
   * \param[in] value - The new value for this field
   */
  void SetValueByIndex(const int i, su2double value){
    insertionVector[i]->second.value = value;
  }

};

class VolumeOutFieldCollection final : public COutFieldCollection<VolumeOutputField>{


public:
  void UpdateTokens(){

  }

};


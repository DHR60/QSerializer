[<img alt="logo" width="1280px" src="https://habrastorage.org/webt/t6/e1/vv/t6e1vvxggs9qkz_h5njg4xrzi0k.png" />]()
This project is designed to convert data from an object view to JSON or XML and opposite in the Qt/C++ ecosystem. C ++ classes by default do not have the required meta-object information for serializing class fields, but Qt is equipped with its own highly efficient meta-object system.
An important feature of the QSerializer is the ability to specify serializable fields of the class without having to serialize the entire class. QSerilaizer generate code and declare `Q_PROPERTY` for every declared member of class. This is convenient because you do not need to create separate structures or classes for serialization of write some code to serialize every class, it's just included to QSerializer.

## Installation
Download repository
```bash
$ git clone https://github.com/smurfomen/QSerializer.git
```
Just include qserializer.h in your project and enjoy simple serialization. qserializer.h located in src folder.
Set compiler define `QS_HAS_JSON` or `QS_HAS_XML` for enabling support for xml or json. Enable both at the same time
[is not supported](https://github.com/smurfomen/QSerializer/issues/7).

</br>A demo project for using QSerializer located in example folder.

## Workflow
To get started, include qserializer.h in your code.
## Create serialization class
For create serializable member of class and generate propertyes, use macro:
- __QS_FIELD__
- __QS_COLLECTION__
- __QS_OBJECT__
- __QS_COLLECTION_OBJECTS__
- __QS_QT_DICT__
- __QS_QT_DICT_OBJECTS__
- __QS_STL_DICT__
- __QS_STL_DICT_OBJECTS__

If you want only declare exists fields - use macro QS_JSON_FIELD, QS_XML_FIELD, QS_JSON_COLLECTION and other (look at qserializer.h)
### Inherit from QSerializer
Inherit from QSerializer, use macro QS_SERIALIZABLE or override metaObject method and declare some serializable fields.</br>
In this case you must use Q_GADGET in your class.
```C++
class User : public QSerializer
{
Q_GADGET
QS_SERIALIZABLE
// Create data members to be serialized - you can use this members in code
QS_FIELD(int, age)
QS_COLLECTION(QVector, QString, parents)
};
```
## **Serialize**
Now you can serialize object of this class to JSON or XML.
For example:
```C++
User u;
u.age = 20;
u.parents.append("Mary");
u.parents.append("Jeff");

/* case: json value */
QJsonObject jsonUser = u.toJson();

/* case: raw json data */
QByteArray djson = u.toRawJson();

/* case: xml-dom */
QDomNode xmlUser = u.toXml();

/* case: raw xml data */
QByteArray dxml = u.toRawXml();
```

## **Deserialize**
Opposite of the serialization procedure is the deserialization procedure.
You can deserialize object from JSON or XML, declared fields will be modified or resets to default for this type values.
For example:
```C++
...
User u;

/* case: json value */
QJsonObject json;
u.fromJson(json);

/* case: raw json data */
QByteArray rawJson;
u.fromJson(rawJson);

/* case: xml-dom */
QDomNode xml;
u.fromXml(xml);

/* case: raw xml data */
QByteArray rawXml;
u.fromXml(rawXml);
```
## Macro description
| Macro                 | Description                                                  |
| --------------------- | ------------------------------------------------------------ |
| QSERIALIZABLE         | Make class or struct is serializable to QSerializer (override QMetaObject method and define Q_GADGET macro)                             |
| QS_FIELD              | Create serializable simple field                             |
| QS_FIELD_OPT          | Create serializable std::optional<T> field                   |
| QS_COLLECTION         | Create serializable collection values of primitive types     |
| QS_OBJECT             | Create serializable inner custom type object                 |
| QS_COLLECTION_OBJECTS | Create serializable collection of custom type objects        |
| QS_QT_DICT            | Create serializable dictionary of primitive type values FOR QT DICTIONARY TYPES |
| QS_QT_DICT_OBJECTS    | Create serializable dictionary of custom type values FOR QT DICTIONARY TYPES |
| QS_STL_DICT           | Create serializable dictionary of primitive type values FOR STL DICTIONARY TYPES |
| QS_STL_DICT_OBJECTS   | Create serializable dictionary of custom type values FOR STL DICTIONARY TYPES |
| QS_SERIALIZABLE       | Override method metaObject and make class serializable       |

## Skipping Empty and Null Values

QSerializer supports skipping empty values and nulls during serialization. This can be configured at both the class and member level.

### Class-Level Options

| Macro                          | Description                                           |
| ------------------------------ | ----------------------------------------------------- |
| QS_SKIP_EMPTY                  | Skip empty values (empty strings, arrays, objects)    |
| QS_SKIP_NULL                   | Skip null values                                      |
| QS_SKIP_EMPTY_AND_NULL         | Skip both empty and null values                       |
| QS_SKIP_EMPTY_AND_NULL_LITERALS| Skip empty, null and "null" string literals           |

Example:
```C++
class User : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE
    QS_SKIP_EMPTY // Will skip all empty values in this class

    QS_FIELD(int, age)
    QS_FIELD(QString, name)
};
```

### Member-Level Options

| Macro                                     | Description                                     |
| ----------------------------------------- | ----------------------------------------------- |
| QS_MEMBER_SKIP_EMPTY                      | Skip if the specific member is empty            |
| QS_MEMBER_SKIP_NULL                       | Skip if the specific member is null             |
| QS_MEMBER_SKIP_EMPTY_AND_NULL             | Skip if the specific member is empty or null    |
| QS_MEMBER_SKIP_EMPTY_AND_NULL_LITERALS    | Skip empty, null and "null" string literals     |

Example:
```C++
class User : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE

    QS_FIELD(int, age)
    QS_FIELD(QString, name)
};

// Skip empty name fields in User class
QS_MEMBER_SKIP_EMPTY(User, name)
```

## std::optional Support

QSerializer now supports `std::optional<T>` fields via the `QS_FIELD_OPT` macro:

```C++
class User : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE
    
    QS_FIELD(int, id)
    QS_FIELD_OPT(QString, email) // Optional field
};
```

When serializing:
- If the optional has a value, it will be serialized normally
- If the optional is empty, it will be serialized as null (unless configured to be skipped)

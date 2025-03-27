/*
 MIT License

 Copyright (c) 2020-2021 Agadzhanov Vladimir
 Portions Copyright (c) 2021 Jerry Jacobs

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
                                                                                 */
/*
 * version modified by Massimo Sacchi (RCH S.p.A.) to set QJsonDocument mode
 * (setted as Compact mode)
 */

/*
 * version modified by DHR60 to
 * add support skip empty values and null
 * fix QJsonDocument mode
 * Support JSON/XML serialization for std::optional<T>
 * Optimize Code
 */

#ifndef QSERIALIZER_H
#define QSERIALIZER_H

/* JSON */
#ifdef QS_HAS_JSON
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#endif

/* XML */
#ifdef QS_HAS_XML
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#endif

/* META OBJECT SYSTEM */
#include <QDebug>
#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaType>
#include <QVariant>

#define QS_VERSION "1.2.3"

/* Generate metaObject method */
#define QS_META_OBJECT_METHOD                     \
  virtual const QMetaObject* metaObject() const { \
    return &this->staticMetaObject;               \
  }

#define QSERIALIZABLE \
  Q_GADGET            \
  QS_META_OBJECT_METHOD

/* Mark class as serializable */
#define QS_SERIALIZABLE QS_META_OBJECT_METHOD

#ifdef QS_HAS_XML
Q_DECLARE_METATYPE(QDomNode)
Q_DECLARE_METATYPE(QDomElement)
#endif

// enable QJsonDocument::Indented for readability, QJsonDocument::Compact for
// performance
#define QS_JSON_DOC_MODE QJsonDocument::Indented  // QJsonDocument::Compact

class QSerializer {
  Q_GADGET
  QS_SERIALIZABLE
 public:
  struct Options {
    bool skipEmpty = false;
    bool skipNull = false;
    bool skipNullLiterals = false;
  };

  struct MemberOptions {
    bool skipEmpty = false;
    bool skipNull = false;
    bool skipNullLiterals = false;
    std::string memberName;
  };

  typedef std::map<std::string, Options> OptionsMap;
  typedef std::map<std::string, std::vector<MemberOptions>> MemberOptionsMap;

  virtual ~QSerializer() = default;

  static OptionsMap s_classOptions;
  static MemberOptionsMap s_memberOptions;

  static void setClassOptions(const std::string& className,
                              const Options& options) {
    s_classOptions[className] = options;
  }

  static Options getClassOptions(const std::string& className) {
    auto it = s_classOptions.find(className);
    if (it != s_classOptions.end()) {
      return it->second;
    }
    return Options();
  }

  static void setMemberOptions(const std::string& className,
                               const std::string& memberName, bool skipEmpty,
                               bool skipNull, bool skipNullLiterals) {
    MemberOptions opts;
    opts.skipEmpty = skipEmpty;
    opts.skipNull = skipNull;
    opts.skipNullLiterals = skipNullLiterals;
    opts.memberName = memberName;
    s_memberOptions[className].push_back(opts);
  }

  bool shouldSkipMemberEmpty(const char* memberName) const {
    auto it = s_memberOptions.find(metaObject()->className());
    if (it != s_memberOptions.end()) {
      for (const auto& opt : it->second) {
        if (opt.memberName == memberName) {
          return opt.skipEmpty;
        }
      }
    }
    return shouldSkipEmpty();  // If there is no member-level setting, use the
                               // class-level setting
  }

  bool shouldSkipMemberNull(const char* memberName) const {
    auto it = s_memberOptions.find(metaObject()->className());
    if (it != s_memberOptions.end()) {
      for (const auto& opt : it->second) {
        if (opt.memberName == memberName) {
          return opt.skipNull;
        }
      }
    }
    return shouldSkipNull();
  }

  bool shouldSkipMemberNullLiterals(const char* memberName) const {
    auto it = s_memberOptions.find(metaObject()->className());
    if (it != s_memberOptions.end()) {
      for (const auto& opt : it->second) {
        if (opt.memberName == memberName) {
          return opt.skipNullLiterals;
        }
      }
    }
    return shouldSkipNullLiterals();
  }

  bool shouldSkipEmpty() const {
    return getClassOptions(metaObject()->className()).skipEmpty;
  }

  bool shouldSkipNull() const {
    return getClassOptions(metaObject()->className()).skipNull;
  }

  bool shouldSkipNullLiterals() const {
    return getClassOptions(metaObject()->className()).skipNullLiterals;
  }

#ifdef QS_HAS_JSON
  /*! \brief  Convert QJsonValue in QJsonDocument as QByteArray. */
  static QByteArray toByteArray(const QJsonValue& value) {
    return QJsonDocument(value.toObject()).toJson(QS_JSON_DOC_MODE);
  }
#endif

#ifdef QS_HAS_XML
  /*! \brief  Convert QDomNode in QDomDocument as QByteArray. */
  static QByteArray toByteArray(const QDomNode& value) {
    QDomDocument doc = value.toDocument();
    return doc.toByteArray();
  }

  /*! \brief  Make xml processing instruction (hat) and returns new XML
   * QDomDocument. On deserialization procedure all processing instructions will
   * be ignored. */
  static QDomDocument appendXmlHat(const QDomNode& node,
                                   const QString& encoding,
                                   const QString& version = "1.0") {
    QDomDocument doc = node.toDocument();
    QDomNode xmlNode = doc.createProcessingInstruction(
        "xml",
        QString("version=\"%1\" encoding=\"%2\"").arg(version).arg(encoding));
    doc.insertBefore(xmlNode, doc.firstChild());
    return doc;
  }
#endif

#ifdef QS_HAS_JSON
  /*! \brief  Serialize all accessed JSON propertyes for this object. */
  virtual QJsonObject toJson() const {
    QJsonObject json;

    for (int i = 0; i < metaObject()->propertyCount(); i++) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
      if (QString(metaObject()->property(i).typeName()) !=
          QMetaType::typeName(qMetaTypeId<QJsonValue>())) {
        continue;
      }
#else
      if (metaObject()->property(i).metaType().id() != QMetaType::QJsonValue) {
        continue;
      }
#endif

      const char* propName = metaObject()->property(i).name();
      QJsonValue value =
          metaObject()->property(i).readOnGadget(this).toJsonValue();

      // Use member-level options
      bool skipEmpty = shouldSkipMemberEmpty(propName);
      bool skipNull = shouldSkipMemberNull(propName);
      bool skipNullLiterals = shouldSkipMemberNullLiterals(propName);

      // skip empty values and nulls
      if ((skipEmpty && value.isString() && value.toString().isEmpty()) ||
          (skipEmpty && value.isArray() && value.toArray().isEmpty()) ||
          (skipEmpty && value.isObject() && value.toObject().isEmpty()) ||
          (skipNull && value.isNull()) ||
          (skipNullLiterals && value.isString() &&
           value.toString() == "null")) {
        continue;
      }

      json.insert(propName, value);
    }
    return json;
  }

  /*! \brief  Returns QByteArray representation this object using
   * json-serialization. */
  QByteArray toRawJson() const { return toByteArray(toJson()); }

  /*! \brief  Deserialize all accessed XML propertyes for this object. */
  virtual void fromJson(const QJsonValue& val) {
    if (val.isObject()) {
      QJsonObject json = val.toObject();
      QStringList keys = json.keys();
      int propCount = metaObject()->propertyCount();
      for (int i = 0; i < propCount; i++) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (QString(metaObject()->property(i).typeName()) !=
            QMetaType::typeName(qMetaTypeId<QJsonValue>())) {
          continue;
        }
#else
        if (metaObject()->property(i).metaType().id() !=
            QMetaType::QJsonValue) {
          continue;
        }
#endif

        for (auto key : json.keys()) {
          if (key == metaObject()->property(i).name()) {
            metaObject()->property(i).writeOnGadget(this, json.value(key));
            break;
          }
        }
      }
    }
  }

  /*! \brief  Deserialize all accessed JSON propertyes for this object. */
  void fromJson(const QByteArray& data) {
    fromJson(QJsonDocument::fromJson(data).object());
  }

  /*! \brief  Create and deserialize an object of type T from JSON. */
  template <typename T>
  static T fromJson(const QJsonValue& val) {
    T obj;
    obj.fromJson(val);
    return obj;
  }

  /*! \brief  Create and deserialize an object of type T from JSON byte array.
   */
  template <typename T>
  static T fromJson(const QByteArray& data) {
    T obj;
    obj.fromJson(data);
    return obj;
  }
#endif  // QS_HAS_JSON

#ifdef QS_HAS_XML
  /*! \brief  Serialize all accessed XML propertyes for this object. */
  virtual QDomNode toXml() const {
    QDomDocument doc;
    QDomElement el = doc.createElement(metaObject()->className());

    for (int i = 0; i < metaObject()->propertyCount(); i++) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
      if (QString(metaObject()->property(i).typeName()) !=
          QMetaType::typeName(qMetaTypeId<QDomNode>())) {
        continue;
      }
#else
      if (metaObject()->property(i).metaType().id() !=
          qMetaTypeId<QDomNode>()) {
        continue;
      }
#endif
      const char* propName = metaObject()->property(i).name();
      QDomNode nodeValue = QDomNode(
          metaObject()->property(i).readOnGadget(this).value<QDomNode>());

      // Use member-level options
      bool skipEmpty = shouldSkipMemberEmpty(propName);
      bool skipNull = shouldSkipMemberNull(propName);
      bool skipNullLiterals = shouldSkipMemberNullLiterals(propName);

      bool isNullLiteral = false;
      bool isEmpty = false;

      // First, check if the node is null
      if (nodeValue.isNull()) {
        isEmpty = true;
      }
      // Then, check if it is a document node (QS_XML_FIELD returns a
      // QDomDocument)
      else if (nodeValue.isDocument()) {
        QDomDocument doc = nodeValue.toDocument();  // Convert to QDomDocument
        QDomElement rootElem = doc.documentElement();
        if (rootElem.hasChildNodes() && rootElem.firstChild().isText()) {
          QString textValue = rootElem.firstChild().nodeValue();
          isNullLiteral = (textValue == "null");
          isEmpty = textValue.isEmpty();
        } else {
          isEmpty = !rootElem.hasChildNodes();
        }
      }
      // Finally, check if it is an element node
      else if (nodeValue.isElement()) {
        QDomElement elem = nodeValue.toElement();
        if (elem.hasChildNodes() && elem.firstChild().isText()) {
          QString textValue = elem.firstChild().nodeValue();
          isNullLiteral = (textValue == "null");
          isEmpty = textValue.isEmpty();
        } else {
          isEmpty = elem.childNodes().isEmpty();
        }
      }

      // More comprehensive skipping condition check
      if ((skipEmpty && isEmpty) || (skipNull && nodeValue.isNull()) ||
          (skipNullLiterals && isNullLiteral)) {
        continue;
      }

      el.appendChild(nodeValue);
    }
    doc.appendChild(el);
    return doc;
  }

  /*! \brief  Returns QByteArray representation this object using
   * xml-serialization. */
  QByteArray toRawXml() const { return toByteArray(toXml()); }

  /*! \brief  Deserialize all accessed XML propertyes for this object. */
  virtual void fromXml(const QDomNode& val) {
    QDomNode doc = val;

    auto n = doc.firstChildElement(metaObject()->className());

    if (!n.isNull()) {
      for (int i = 0; i < metaObject()->propertyCount(); i++) {
        QString name = metaObject()->property(i).name();
        QDomElement tmp = metaObject()
                              ->property(i)
                              .readOnGadget(this)
                              .value<QDomNode>()
                              .firstChildElement();

        auto f = n.firstChildElement(tmp.tagName());
        metaObject()->property(i).writeOnGadget(
            this, QVariant::fromValue<QDomNode>(f));
      }
    } else {
      for (int i = 0; i < metaObject()->propertyCount(); i++) {
        QString name = metaObject()->property(i).name();
        auto f = doc.firstChildElement(name);
        metaObject()->property(i).writeOnGadget(
            this, QVariant::fromValue<QDomNode>(f));
      }
    }
  }

  /*! \brief  Deserialize all accessed XML propertyes for this object. */
  void fromXml(const QByteArray& data) {
    QDomDocument d;
    d.setContent(data);
    fromXml(d);
  }

  /*! \brief  Create and deserialize an object of type T from XML. */
  template <typename T>
  static T fromXml(const QDomNode& val) {
    T obj;
    obj.fromXml(val);
    return obj;
  }

  /*! \brief  Create and deserialize an object of type T from XML byte array. */
  template <typename T>
  static T fromXml(const QByteArray& data) {
    T obj;
    obj.fromXml(data);
    return obj;
  }
#endif  // QS_HAS_XML
};

#define GET(prefix, name) get_##prefix##_##name
#define SET(prefix, name) set_##prefix##_##name

/* Create variable */
#define QS_DECLARE_MEMBER(type, name) \
 public:                              \
  type name = type();

#define QS_DECLARE_MEMBER_DEFAULT(type, name, default_value) \
 public:                                                     \
  type name = default_value;

/* Create JSON property and methods for primitive type field*/
#ifdef QS_HAS_JSON
#define QS_JSON_FIELD(type, name)                                        \
  Q_PROPERTY(QJsonValue name READ GET(json, name) WRITE SET(json, name)) \
 private:                                                                \
  QJsonValue GET(json, name)() const {                                   \
    QJsonValue val = QJsonValue::fromVariant(QVariant(name));            \
    return val;                                                          \
  }                                                                      \
  void SET(json, name)(const QJsonValue& varname) {                      \
    name = varname.toVariant().value<type>();                            \
  }
#define QS_JSON_FIELD_OPT(type, name)                                        \
  Q_PROPERTY(QJsonValue name READ GET(json, name) WRITE SET(json, name))     \
 private:                                                                    \
  QJsonValue GET(json, name)() const {                                       \
    if (name.has_value()) {                                                  \
      /* When optional has a value, use QVariant for conversion */           \
      return QJsonValue::fromVariant(QVariant(name.value()));                \
    } else {                                                                 \
      /* When optional is empty, return null; whether to write it depends on \
       * the skip null setting */                                            \
      return QJsonValue(QJsonValue::Null);                                   \
    }                                                                        \
  }                                                                          \
  void SET(json, name)(const QJsonValue& varname) {                          \
    if (varname.isNull()) {                                                  \
      name = std::nullopt;                                                   \
    } else {                                                                 \
      name = varname.toVariant().value<type>();                              \
    }                                                                        \
  }
#define QS_JSON_OBJECT_OPT(type, name)                                   \
  Q_PROPERTY(QJsonValue name READ GET(json, name) WRITE SET(json, name)) \
 private:                                                                \
  QJsonValue GET(json, name)() const {                                   \
    if (name.has_value()) {                                              \
      return name.value().toJson();                                      \
    } else {                                                             \
      return QJsonValue(QJsonValue::Null);                               \
    }                                                                    \
  }                                                                      \
  void SET(json, name)(const QJsonValue& varname) {                      \
    if (varname.isNull()) {                                              \
      name = std::nullopt;                                               \
    } else {                                                             \
      type temp;                                                         \
      temp.fromJson(varname);                                            \
      name = temp;                                                       \
    }                                                                    \
  }
#else
#define QS_JSON_FIELD(type, name)
#define QS_JSON_FIELD_OPT(type, name)
#define QS_JSON_OBJECT_OPT(type, name)
#endif

/* Create XML property and methods for primitive type field*/
#ifdef QS_HAS_XML
#define QS_XML_FIELD(type, name)                                          \
  Q_PROPERTY(QDomNode name READ GET(xml, name) WRITE SET(xml, name))      \
 private:                                                                 \
  QDomNode GET(xml, name)() const {                                       \
    QDomDocument doc;                                                     \
    QString strname = #name;                                              \
    QDomElement element = doc.createElement(strname);                     \
    QDomText valueOfProp = doc.createTextNode(QVariant(name).toString()); \
    element.appendChild(valueOfProp);                                     \
    doc.appendChild(element);                                             \
    return QDomNode(doc);                                                 \
  }                                                                       \
  void SET(xml, name)(const QDomNode& node) {                             \
    if (!node.isNull() && node.isElement()) {                             \
      QDomElement domElement = node.toElement();                          \
      if (domElement.tagName() == #name)                                  \
        name = QVariant(domElement.text()).value<type>();                 \
    }                                                                     \
  }
#define QS_XML_FIELD_OPT(type, name)                                      \
  Q_PROPERTY(QDomNode name READ GET(xml, name) WRITE SET(xml, name))      \
 private:                                                                 \
  QDomNode GET(xml, name)() const {                                       \
    QDomDocument doc;                                                     \
    QString strname = #name;                                              \
    QDomElement element = doc.createElement(strname);                     \
    if (name.has_value()) {                                               \
      QDomText valueOfProp =                                              \
          doc.createTextNode(QVariant(name.value()).toString());          \
      element.appendChild(valueOfProp);                                   \
    } else {                                                              \
      /* Generate “null” text when optional field is null, subsequent \
       * deserialization identifies as null */                            \
      QDomText valueOfProp = doc.createTextNode("null");                  \
      element.appendChild(valueOfProp);                                   \
    }                                                                     \
    doc.appendChild(element);                                             \
    return QDomNode(doc);                                                 \
  }                                                                       \
  void SET(xml, name)(const QDomNode& node) {                             \
    if (!node.isNull() && node.isElement()) {                             \
      QDomElement domElement = node.toElement();                          \
      if (domElement.tagName() == #name) {                                \
        QString text = domElement.text();                                 \
        if (text == "null") {                                             \
          name = std::nullopt;                                            \
        } else {                                                          \
          name = QVariant(text).value<type>();                            \
        }                                                                 \
      }                                                                   \
    }                                                                     \
  }
#define QS_XML_OBJECT_OPT(type, name)                                \
  Q_PROPERTY(QDomNode name READ GET(xml, name) WRITE SET(xml, name)) \
 private:                                                            \
  QDomNode GET(xml, name)() const {                                  \
    if (name.has_value()) {                                          \
      return name.value().toXml();                                   \
    } else {                                                         \
      QDomDocument doc;                                              \
      QDomElement element = doc.createElement(#name);                \
      element.appendChild(doc.createTextNode("null"));               \
      doc.appendChild(element);                                      \
      return QDomNode(doc);                                          \
    }                                                                \
  }                                                                  \
  void SET(xml, name)(const QDomNode& node) {                        \
    if (!node.isNull() && node.isElement()) {                        \
      QDomElement domElement = node.toElement();                     \
      if (domElement.text() == "null") {                             \
        name = std::nullopt;                                         \
      } else {                                                       \
        type temp;                                                   \
        temp.fromXml(node);                                          \
        name = temp;                                                 \
      }                                                              \
    }                                                                \
  }
#else
#define QS_XML_FIELD(type, name)
#define QS_XML_FIELD_OPT(type, name)
#define QS_XML_OBJECT_OPT(type, name)
#endif

/* Generate JSON-property and methods for primitive type objects */
/* This collection must be provide method append(T) (it's can be QList, QVector)
 */
#ifdef QS_HAS_JSON
#define QS_JSON_ARRAY(itemType, name)                                    \
  Q_PROPERTY(QJsonValue name READ GET(json, name) WRITE SET(json, name)) \
 private:                                                                \
  QJsonValue GET(json, name)() const {                                   \
    QJsonArray val;                                                      \
    for (int i = 0; i < name.size(); i++) val.push_back(name.at(i));     \
    return QJsonValue::fromVariant(val);                                 \
  }                                                                      \
  void SET(json, name)(const QJsonValue& varname) {                      \
    if (!varname.isArray()) return;                                      \
    name.clear();                                                        \
    QJsonArray val = varname.toArray();                                  \
    for (const auto& item : val) {                                       \
      itemType tmp;                                                      \
      tmp = item.toVariant().value<itemType>();                          \
      name.append(tmp);                                                  \
    }                                                                    \
  }
#else
#define QS_JSON_ARRAY(itemType, name)
#endif

/* Generate XML-property and methods for primitive type objects */
/* This collection must be provide method append(T) (it's can be QList, QVector)
 */
#ifdef QS_HAS_XML
#define QS_XML_ARRAY(itemType, name)                                      \
  Q_PROPERTY(QDomNode name READ GET(xml, name) WRITE SET(xml, name))      \
 private:                                                                 \
  QDomNode GET(xml, name)() const {                                       \
    QDomDocument doc;                                                     \
    QString strname = #name;                                              \
    QDomElement arrayXml = doc.createElement(QString(strname));           \
    arrayXml.setAttribute("type", "array");                               \
                                                                          \
    for (int i = 0; i < name.size(); i++) {                               \
      itemType item = name.at(i);                                         \
      QDomElement itemXml = doc.createElement("item");                    \
      itemXml.setAttribute("type", #itemType);                            \
      itemXml.setAttribute("index", i);                                   \
      itemXml.appendChild(doc.createTextNode(QVariant(item).toString())); \
      arrayXml.appendChild(itemXml);                                      \
    }                                                                     \
                                                                          \
    doc.appendChild(arrayXml);                                            \
    return QDomNode(doc);                                                 \
  }                                                                       \
  void SET(xml, name)(const QDomNode& node) {                             \
    QDomNode domNode = node.firstChild();                                 \
    name.clear();                                                         \
    while (!domNode.isNull()) {                                           \
      if (domNode.isElement()) {                                          \
        QDomElement domElement = domNode.toElement();                     \
        name.append(QVariant(domElement.text()).value<itemType>());       \
      }                                                                   \
      domNode = domNode.nextSibling();                                    \
    }                                                                     \
  }
#else
#define QS_XML_ARRAY(itemType, name)
#endif

/* Generate JSON-property and methods for some custom class */
/* Custom type must be provide methods fromJson and toJson or inherit from
 * QSerializer */
#ifdef QS_HAS_JSON
#define QS_JSON_OBJECT(type, name)                                       \
  Q_PROPERTY(QJsonValue name READ GET(json, name) WRITE SET(json, name)) \
 private:                                                                \
  QJsonValue GET(json, name)() const {                                   \
    QJsonObject val = name.toJson();                                     \
    return QJsonValue(val);                                              \
  }                                                                      \
  void SET(json, name)(const QJsonValue& varname) {                      \
    if (!varname.isObject()) return;                                     \
    name.fromJson(varname);                                              \
  }
#else
#define QS_JSON_OBJECT(type, name)
#endif

/* Generate XML-property and methods for some custom class */
/* Custom type must be provide methods fromJson and toJson or inherit from
 * QSerializer */
#ifdef QS_HAS_XML
#define QS_XML_OBJECT(type, name)                                    \
  Q_PROPERTY(QDomNode name READ GET(xml, name) WRITE SET(xml, name)) \
 private:                                                            \
  QDomNode GET(xml, name)() const { return name.toXml(); }           \
  void SET(xml, name)(const QDomNode& node) { name.fromXml(node); }
#else
#define QS_XML_OBJECT(type, name)
#endif

/* Generate JSON-property and methods for collection of custom type objects */
/* Custom item type must be provide methods fromJson and toJson or inherit from
 * QSerializer */
/* This collection must be provide method append(T) (it's can be QList, QVector)
 */
#ifdef QS_HAS_JSON
#define QS_JSON_ARRAY_OBJECTS(itemType, name)                                 \
  Q_PROPERTY(QJsonValue name READ GET(json, name) WRITE SET(json, name))      \
 private:                                                                     \
  QJsonValue GET(json, name)() const {                                        \
    QJsonArray val;                                                           \
    for (int i = 0; i < name.size(); i++) val.push_back(name.at(i).toJson()); \
    return QJsonValue::fromVariant(val);                                      \
  }                                                                           \
  void SET(json, name)(const QJsonValue& varname) {                           \
    if (!varname.isArray()) return;                                           \
    name.clear();                                                             \
    QJsonArray val = varname.toArray();                                       \
    for (int i = 0; i < val.size(); i++) {                                    \
      itemType tmp;                                                           \
      tmp.fromJson(val.at(i));                                                \
      name.append(tmp);                                                       \
    }                                                                         \
  }
#else
#define QS_JSON_ARRAY_OBJECTS(itemType, name)
#endif

/* Generate XML-property and methods for collection of custom type objects  */
/* Custom type must be provide methods fromXml and toXml or inherit from
 * QSerializer */
/* This collection must be provide method append(T) (it's can be QList, QVector)
 */
#ifdef QS_HAS_XML
#define QS_XML_ARRAY_OBJECTS(itemType, name)                         \
  Q_PROPERTY(QDomNode name READ GET(xml, name) WRITE SET(xml, name)) \
 private:                                                            \
  QDomNode GET(xml, name)() const {                                  \
    QDomDocument doc;                                                \
    QDomElement element = doc.createElement(#name);                  \
    element.setAttribute("type", "array");                           \
    for (int i = 0; i < name.size(); i++)                            \
      element.appendChild(name.at(i).toXml());                       \
    doc.appendChild(element);                                        \
    return QDomNode(doc);                                            \
  }                                                                  \
  void SET(xml, name)(const QDomNode& node) {                        \
    name.clear();                                                    \
    QDomNodeList nodesList = node.childNodes();                      \
    for (int i = 0; i < nodesList.size(); i++) {                     \
      itemType tmp;                                                  \
      tmp.fromXml(nodesList.at(i));                                  \
      name.append(tmp);                                              \
    }                                                                \
  }
#else
#define QS_XML_ARRAY_OBJECTS(itemType, name)
#endif

/* Generate JSON-property and methods for dictionary of simple fields (int,
 * bool, QString, ...)  */
/* Custom type must be inherit from QSerializer */
/* This collection must be provide method insert(KeyT, ValueT) (it's can be
 * QMap, QHash)    */
/* THIS IS FOR QT DICTIONARY TYPES, for example QMap<int, QString>,
 * QMap<int,int>, ...*/
#ifdef QS_HAS_JSON
#define QS_JSON_QT_DICT(map, name)                                       \
  Q_PROPERTY(QJsonValue name READ GET(json, name) WRITE SET(json, name)) \
 private:                                                                \
  QJsonValue GET(json, name)() const {                                   \
    QJsonObject val;                                                     \
    for (auto p = name.constBegin(); p != name.constEnd(); ++p) {        \
      val.insert(QVariant(p.key()).toString(),                           \
                 QJsonValue::fromVariant(QVariant(p.value())));          \
    }                                                                    \
    return val;                                                          \
  }                                                                      \
  void SET(json, name)(const QJsonValue& varname) {                      \
    QJsonObject val = varname.toObject();                                \
    name.clear();                                                        \
    for (auto p = val.constBegin(); p != val.constEnd(); ++p) {          \
      name.insert(QVariant(p.key()).value<map::key_type>(),              \
                  QVariant(p.value()).value<map::mapped_type>());        \
    }                                                                    \
  }
#else
#define QS_JSON_QT_DICT(map, name)
#endif

/* Generate XML-property and methods for dictionary of simple fields (int, bool,
 * QString, ...)  */
/* Custom type must be inherit from QSerializer */
/* This collection must be provide method insert(KeyT, ValueT) (it's can be
 * QMap, QHash)    */
/* THIS IS FOR QT DICTIONARY TYPES, for example QMap<int, QString>,
 * QMap<int,int>, ...*/
#ifdef QS_HAS_XML
#define QS_XML_QT_DICT(map, name)                                    \
  Q_PROPERTY(QDomNode name READ GET(xml, name) WRITE SET(xml, name)) \
 private:                                                            \
  QDomNode GET(xml, name)() const {                                  \
    QDomDocument doc;                                                \
    QDomElement element = doc.createElement(#name);                  \
    element.setAttribute("type", "map");                             \
    for (auto p = name.begin(); p != name.end(); ++p) {              \
      QDomElement e = doc.createElement("item");                     \
      e.setAttribute("key", QVariant(p.key()).toString());           \
      e.setAttribute("value", QVariant(p.value()).toString());       \
      element.appendChild(e);                                        \
    }                                                                \
    doc.appendChild(element);                                        \
    return QDomNode(doc);                                            \
  }                                                                  \
  void SET(xml, name)(const QDomNode& node) {                        \
    if (!node.isNull() && node.isElement()) {                        \
      QDomElement root = node.toElement();                           \
      if (root.tagName() == #name) {                                 \
        QDomNodeList childs = root.childNodes();                     \
                                                                     \
        for (int i = 0; i < childs.size(); ++i) {                    \
          QDomElement item = childs.at(i).toElement();               \
          name.insert(QVariant(item.attributeNode("key").value())    \
                          .value<map::key_type>(),                   \
                      QVariant(item.attributeNode("value").value())  \
                          .value<map::mapped_type>());               \
        }                                                            \
      }                                                              \
    }                                                                \
  }
#else
#define QS_XML_QT_DICT(map, name)
#endif

/* Generate JSON-property and methods for dictionary of custom type objects  */
/* Custom type must be inherit from QSerializer */
/* This collection must be provide method inserv(KeyT, ValueT) (it's can be
 * QMap, QHash)    */
/* THIS IS FOR QT DICTIONARY TYPES, for example QMap<int,
 * CustomSerializableType> */
#ifdef QS_HAS_JSON
#define QS_JSON_QT_DICT_OBJECTS(map, name)                                     \
  Q_PROPERTY(QJsonValue name READ GET(json, name) WRITE SET(json, name))       \
 private:                                                                      \
  QJsonValue GET(json, name)() const {                                         \
    QJsonObject val;                                                           \
    for (auto p = name.begin(); p != name.end(); ++p) {                        \
      val.insert(QVariant::fromValue(p.key()).toString(), p.value().toJson()); \
    }                                                                          \
    return val;                                                                \
  }                                                                            \
  void SET(json, name)(const QJsonValue& varname) {                            \
    QJsonObject val = varname.toObject();                                      \
    name.clear();                                                              \
    for (auto p = val.constBegin(); p != val.constEnd(); ++p) {                \
      map::mapped_type tmp;                                                    \
      tmp.fromJson(p.value());                                                 \
      name.insert(QVariant(p.key()).value<map::key_type>(), tmp);              \
    }                                                                          \
  }
#else
#define QS_JSON_QT_DICT_OBJECTS(map, name)
#endif

/* Generate XML-property and methods for dictionary of custom type objects  */
/* Custom type must be inherit from QSerializer */
/* This collection must be provide method insert(KeyT, ValueT) (it's can be
 * QMap, QHash)    */
/* THIS IS FOR QT DICTIONARY TYPES, for example QMap<int,
 * CustomSerializableType> */
#ifdef QS_HAS_XML
#define QS_XML_QT_DICT_OBJECTS(map, name)                            \
  Q_PROPERTY(QDomNode name READ GET(xml, name) WRITE SET(xml, name)) \
 private:                                                            \
  QDomNode GET(xml, name)() const {                                  \
    QDomDocument doc;                                                \
    QDomElement element = doc.createElement(#name);                  \
    element.setAttribute("type", "map");                             \
    for (auto p = name.begin(); p != name.end(); ++p) {              \
      QDomElement e = doc.createElement("item");                     \
      e.setAttribute("key", QVariant(p.key()).toString());           \
      e.appendChild(p.value().toXml());                              \
      element.appendChild(e);                                        \
    }                                                                \
    doc.appendChild(element);                                        \
    return QDomNode(doc);                                            \
  }                                                                  \
  void SET(xml, name)(const QDomNode& node) {                        \
    if (!node.isNull() && node.isElement()) {                        \
      QDomElement root = node.toElement();                           \
      if (root.tagName() == #name) {                                 \
        QDomNodeList childs = root.childNodes();                     \
                                                                     \
        for (int i = 0; i < childs.size(); ++i) {                    \
          QDomElement item = childs.at(i).toElement();               \
          map::mapped_type tmp;                                      \
          tmp.fromXml(item.firstChild());                            \
          name.insert(QVariant(item.attributeNode("key").value())    \
                          .value<map::key_type>(),                   \
                      tmp);                                          \
        }                                                            \
      }                                                              \
    }                                                                \
  }
#else
#define QS_XML_QT_DICT_OBJECTS(map, name)
#endif

/* Generate JSON-property and methods for dictionary of simple fields (int,
 * bool, QString, ...)  */
/* Custom type must be inherit from QSerializer */
/* This collection must be provide method insert(KeyT, ValueT) (it's can be
 * std::map)    */
/* THIS IS FOR STL DICTIONARY TYPES, for example std::map<int, QString>,
 * std::map<int,int>, ...*/
#ifdef QS_HAS_JSON
#define QS_JSON_STL_DICT(map, name)                                      \
  Q_PROPERTY(QJsonValue name READ GET(json, name) WRITE SET(json, name)) \
 private:                                                                \
  QJsonValue GET(json, name)() const {                                   \
    QJsonObject val;                                                     \
    for (const auto& p : name) {                                         \
      val.insert(QVariant::fromValue(p.first).toString(),                \
                 QJsonValue::fromVariant(QVariant(p.second)));           \
    }                                                                    \
    return val;                                                          \
  }                                                                      \
  void SET(json, name)(const QJsonValue& varname) {                      \
    QJsonObject val = varname.toObject();                                \
    name.clear();                                                        \
    for (auto p = val.constBegin(); p != val.constEnd(); ++p) {          \
      name.insert(std::pair<map::key_type, map::mapped_type>(            \
          QVariant(p.key()).value<map::key_type>(),                      \
          QVariant(p.value()).value<map::mapped_type>()));               \
    }                                                                    \
  }
#else
#define QS_JSON_STL_DICT(map, name)
#endif

#ifdef QS_HAS_XML
#define QS_XML_STL_DICT(map, name)                                   \
  Q_PROPERTY(QDomNode name READ GET(xml, name) WRITE SET(xml, name)) \
 private:                                                            \
  QDomNode GET(xml, name)() const {                                  \
    QDomDocument doc;                                                \
    QDomElement element = doc.createElement(#name);                  \
    element.setAttribute("type", "map");                             \
    for (auto p : name) {                                            \
      QDomElement e = doc.createElement("item");                     \
      e.setAttribute("key", QVariant(p.first).toString());           \
      e.setAttribute("value", QVariant(p.second).toString());        \
      element.appendChild(e);                                        \
    }                                                                \
    doc.appendChild(element);                                        \
    return QDomNode(doc);                                            \
  }                                                                  \
  void SET(xml, name)(const QDomNode& node) {                        \
    if (!node.isNull() && node.isElement()) {                        \
      QDomElement root = node.toElement();                           \
      if (root.tagName() == #name) {                                 \
        QDomNodeList childs = root.childNodes();                     \
                                                                     \
        for (int i = 0; i < childs.size(); ++i) {                    \
          QDomElement item = childs.at(i).toElement();               \
          name.insert(std::pair<map::key_type, map::mapped_type>(    \
              QVariant(item.attributeNode("key").value())            \
                  .value<map::key_type>(),                           \
              QVariant(item.attributeNode("value").value())          \
                  .value<map::mapped_type>()));                      \
        }                                                            \
      }                                                              \
    }                                                                \
  }
#else
#define QS_XML_STL_DICT(map, name)
#endif

/* Generate JSON-property and methods for dictionary of custom type objects */
/* Custom type must be inherit from QSerializer */
/* This collection must be provide method insert(KeyT, ValueT) (it's can be
 * std::map)    */
/* THIS IS FOR STL DICTIONARY TYPES, for example std::map<int,
 * CustomSerializableType> */
#ifdef QS_HAS_JSON
#define QS_JSON_STL_DICT_OBJECTS(map, name)                                   \
  Q_PROPERTY(QJsonValue name READ GET(json, name) WRITE SET(json, name))      \
 private:                                                                     \
  QJsonValue GET(json, name)() const {                                        \
    QJsonObject val;                                                          \
    for (auto p : name) {                                                     \
      val.insert(QVariant::fromValue(p.first).toString(), p.second.toJson()); \
    }                                                                         \
    return val;                                                               \
  }                                                                           \
  void SET(json, name)(const QJsonValue& varname) {                           \
    QJsonObject val = varname.toObject();                                     \
    name.clear();                                                             \
    for (auto p = val.constBegin(); p != val.constEnd(); ++p) {               \
      map::mapped_type tmp;                                                   \
      tmp.fromJson(p.value());                                                \
      name.insert(std::pair<map::key_type, map::mapped_type>(                 \
          QVariant(p.key()).value<map::key_type>(), tmp));                    \
    }                                                                         \
  }
#else
#define QS_JSON_STL_DICT_OBJECTS(map, name)
#endif

/* Generate XML-property and methods for dictionary of custom type objects */
/* Custom type must be inherit from QSerializer */
/* This collection must be provide method insert(KeyT, ValueT) (it's can be
 * std::map)    */
/* THIS IS FOR STL DICTIONARY TYPES, for example std::map<int,
 * CustomSerializableType> */
#ifdef QS_HAS_XML
#define QS_XML_STL_DICT_OBJECTS(map, name)                           \
  Q_PROPERTY(QDomNode name READ GET(xml, name) WRITE SET(xml, name)) \
 private:                                                            \
  QDomNode GET(xml, name)() const {                                  \
    QDomDocument doc;                                                \
    QDomElement element = doc.createElement(#name);                  \
    element.setAttribute("type", "map");                             \
    for (auto p : name) {                                            \
      QDomElement e = doc.createElement("item");                     \
      e.setAttribute("key", QVariant(p.first).toString());           \
      e.appendChild(p.second.toXml());                               \
      element.appendChild(e);                                        \
    }                                                                \
    doc.appendChild(element);                                        \
    return QDomNode(doc);                                            \
  }                                                                  \
  void SET(xml, name)(const QDomNode& node) {                        \
    if (!node.isNull() && node.isElement()) {                        \
      QDomElement root = node.toElement();                           \
      if (root.tagName() == #name) {                                 \
        QDomNodeList childs = root.childNodes();                     \
                                                                     \
        for (int i = 0; i < childs.size(); ++i) {                    \
          QDomElement item = childs.at(i).toElement();               \
          map::mapped_type tmp;                                      \
          tmp.fromXml(item.firstChild());                            \
          name.insert(std::pair<map::key_type, map::mapped_type>(    \
              QVariant(item.attributeNode("key").value())            \
                  .value<map::key_type>(),                           \
              tmp));                                                 \
        }                                                            \
      }                                                              \
    }                                                                \
  }
#else
#define QS_XML_STL_DICT_OBJECTS(map, name)
#endif

/* BIND: */
/* generate serializable propertyes JSON and XML for primitive type field */
#define QS_BIND_FIELD(type, name) \
  QS_JSON_FIELD(type, name)       \
  QS_XML_FIELD(type, name)

#define QS_BIND_FIELD_OPT(type, name) \
  QS_JSON_FIELD_OPT(type, name)       \
  QS_XML_FIELD_OPT(type, name)

/* BIND: */
/* generate serializable propertyes JSON and XML for collection of primitive
 * type fields */
#define QS_BIND_COLLECTION(itemType, name) \
  QS_JSON_ARRAY(itemType, name)            \
  QS_XML_ARRAY(itemType, name)

/* BIND: */
/* generate serializable propertyes JSON and XML for custom type object */
#define QS_BIND_OBJECT(type, name) \
  QS_JSON_OBJECT(type, name)       \
  QS_XML_OBJECT(type, name)

#define QS_BIND_OBJECT_OPT(type, name) \
  QS_JSON_OBJECT_OPT(type, name)       \
  QS_XML_OBJECT_OPT(type, name)

/* BIND: */
/* generate serializable propertyes JSON and XML for collection of custom type
 * objects */
#define QS_BIND_COLLECTION_OBJECTS(itemType, name) \
  QS_JSON_ARRAY_OBJECTS(itemType, name)            \
  QS_XML_ARRAY_OBJECTS(itemType, name)

/* BIND: */
/* generate serializable propertyes JSON and XML for dictionary with primitive
 * value type for QT DICTIONARY TYPES */
#define QS_BIND_QT_DICT(map, name) \
  QS_JSON_QT_DICT(map, name)       \
  QS_XML_QT_DICT(map, name)

/* BIND: */
/* generate serializable propertyes JSON and XML for dictionary of custom type
 * objects for QT DICTIONARY TYPES */
#define QS_BIND_QT_DICT_OBJECTS(map, name) \
  QS_JSON_QT_DICT_OBJECTS(map, name)       \
  QS_XML_QT_DICT_OBJECTS(map, name)

/* BIND: */
/* generate serializable propertyes JSON and XML for dictionary with primitive
 * value type for STL DICTIONARY TYPES */
#define QS_BIND_STL_DICT(map, name) \
  QS_JSON_STL_DICT(map, name)       \
  QS_XML_STL_DICT(map, name)

/* BIND: */
/* generate serializable propertyes JSON and XML for dictionary of custom type
 * objects for STL DICTIONARY TYPES */
#define QS_BIND_STL_DICT_OBJECTS(map, name) \
  QS_JSON_STL_DICT_OBJECTS(map, name)       \
  QS_XML_STL_DICT_OBJECTS(map, name)

/* CREATE AND BIND: */
/* Make primitive field and generate serializable propertyes */
/* For example: QS_FIELD(int, digit), QS_FIELD(bool, flag) */
#define QS_FIELD(type, name)    \
  QS_DECLARE_MEMBER(type, name) \
  QS_BIND_FIELD(type, name)

#define QS_FIELD_OPT(type, name)               \
  QS_DECLARE_MEMBER(std::optional<type>, name) \
  QS_BIND_FIELD_OPT(type, name)

#define QS_FIELD_DEFAULT(type, name, default_value)    \
  QS_DECLARE_MEMBER_DEFAULT(type, name, default_value) \
  QS_BIND_FIELD(type, name)

/* CREATE AND BIND: */
/* Make collection of primitive type objects [collectionType<itemType> name] and
 * generate serializable propertyes for this collection */
/* This collection must be provide method append(T) (it's can be QList, QVector)
 */
#define QS_COLLECTION(collectionType, itemType, name) \
  QS_DECLARE_MEMBER(collectionType<itemType>, name)   \
  QS_BIND_COLLECTION(itemType, name)

/* CREATE AND BIND: */
/* Make custom class object and bind serializable propertyes */
/* This class must be inherited from QSerializer */
#define QS_OBJECT(type, name)   \
  QS_DECLARE_MEMBER(type, name) \
  QS_BIND_OBJECT(type, name)

#define QS_OBJECT_OPT(type, name)              \
  QS_DECLARE_MEMBER(std::optional<type>, name) \
  QS_BIND_OBJECT_OPT(type, name)

#define QS_OBJECT_DEFAULT(type, name, default_value)   \
  QS_DECLARE_MEMBER_DEFAULT(type, name, default_value) \
  QS_BIND_OBJECT(type, name)

/* CREATE AND BIND: */
/* Make collection of custom class objects [collectionType<itemType> name] and
 * bind serializable propertyes */
/* This collection must be provide method append(T) (it's can be QList, QVector)
 */
#define QS_COLLECTION_OBJECTS(collectionType, itemType, name) \
  QS_DECLARE_MEMBER(collectionType<itemType>, name)           \
  QS_BIND_COLLECTION_OBJECTS(itemType, name)

/* CREATE AND BIND: */
/* Make dictionary collection of simple types [dictionary<key, itemType> name]
 * and bind serializable propertyes */
/* This collection must be QT DICTIONARY TYPE */
#define QS_QT_DICT(map, first, second, name)  \
 public:                                      \
  typedef map<first, second> dict_##name##_t; \
  dict_##name##_t name = dict_##name##_t();   \
  QS_BIND_QT_DICT(dict_##name##_t, name)

/* CREATE AND BIND: */
/* Make dictionary collection of custom class objects [dictionary<key, itemType>
 * name] and bind serializable propertyes */
/* This collection must be QT DICTIONARY TYPE */
#define QS_QT_DICT_OBJECTS(map, first, second, name) \
 public:                                             \
  typedef map<first, second> dict_##name##_t;        \
  dict_##name##_t name = dict_##name##_t();          \
  QS_BIND_QT_DICT_OBJECTS(dict_##name##_t, name)

/* CREATE AND BIND: */
/* Make dictionary collection of simple types [dictionary<key, itemType> name]
 * and bind serializable propertyes */
/* This collection must be STL DICTIONARY TYPE */
#define QS_STL_DICT(map, first, second, name) \
 public:                                      \
  typedef map<first, second> dict_##name##_t; \
  dict_##name##_t name = dict_##name##_t();   \
  QS_BIND_STL_DICT(dict_##name##_t, name)

/* CREATE AND BIND: */
/* Make dictionary collection of custom class objects [dictionary<key, itemType>
 * name] and bind serializable propertyes */
/* This collection must be STL DICTIONARY TYPE */
#define QS_STL_DICT_OBJECTS(map, first, second, name) \
 public:                                              \
  typedef map<first, second> dict_##name##_t;         \
  dict_##name##_t name = dict_##name##_t();           \
  QS_BIND_STL_DICT_OBJECTS(dict_##name##_t, name)

#define QS_SERIALIZE_OPTIONS(className, skipEmpty, skipNull, skipNullLiterals) \
  namespace {                                                                  \
  struct className##_options_initializer {                                     \
    className##_options_initializer() {                                        \
      QSerializer::Options opts;                                               \
      opts.skipEmpty = skipEmpty;                                              \
      opts.skipNull = skipNull;                                                \
      opts.skipNullLiterals = skipNullLiterals;                                \
      QSerializer::setClassOptions(#className, opts);                          \
    }                                                                          \
  };                                                                           \
  static className##_options_initializer className##_options_init;             \
  }

#define QS_SKIP_EMPTY(className) \
  QS_SERIALIZE_OPTIONS(className, true, false, false)

#define QS_SKIP_NULL(className) \
  QS_SERIALIZE_OPTIONS(className, false, true, false)

#define QS_SKIP_EMPTY_AND_NULL(className) \
  QS_SERIALIZE_OPTIONS(className, true, true, false)

#define QS_SKIP_EMPTY_AND_NULL_LITERALS(className) \
  QS_SERIALIZE_OPTIONS(className, true, true, true)

#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define QS_INTERNAL_SERIALIZE_OPTIONS(skipEmpty, skipNull, skipNullLiterals)  \
 private:                                                                     \
  static constexpr QSerializer::Options _classOptions = {skipEmpty, skipNull, \
                                                         skipNullLiterals};   \
  class OptionsInitializer {                                                  \
   public:                                                                    \
    OptionsInitializer() {                                                    \
      QSerializer::setClassOptions(staticMetaObject.className(),              \
                                   _classOptions);                            \
    }                                                                         \
  };                                                                          \
  inline static OptionsInitializer _optionsInitializer;
#else
#define QS_INTERNAL_SERIALIZE_OPTIONS(skipEmpty, skipNull, skipNullLiterals) \
 private:                                                                    \
  static void _initializeOptions() {                                         \
    static bool initialized = false;                                         \
    if (!initialized) {                                                      \
      QSerializer::Options opts;                                             \
      opts.skipEmpty = skipEmpty;                                            \
      opts.skipNull = skipNull;                                              \
      opts.skipNullLiterals = skipNullLiterals;                              \
      QSerializer::setClassOptions(staticMetaObject.className(), opts);      \
      initialized = true;                                                    \
    }                                                                        \
  }                                                                          \
  class OptionsInitializer {                                                 \
   public:                                                                   \
    OptionsInitializer() { _initializeOptions(); }                           \
  };                                                                         \
  OptionsInitializer _optionsInitializer;
#endif

#define QS_INTERNAL_SKIP_EMPTY QS_INTERNAL_SERIALIZE_OPTIONS(true, false, false)

#define QS_INTERNAL_SKIP_NULL QS_INTERNAL_SERIALIZE_OPTIONS(false, true, false)

#define QS_INTERNAL_SKIP_EMPTY_AND_NULL \
  QS_INTERNAL_SERIALIZE_OPTIONS(true, true, false)

#define QS_INTERNAL_SKIP_EMPTY_AND_NULL_LITERALS \
  QS_INTERNAL_SERIALIZE_OPTIONS(true, true, true)

/* Complete the definition of serialization properties in the implementation
 * file */
#define QS_IMPLEMENT_BEGIN(className)    \
  class className : public QSerializer { \
    Q_GADGET                             \
    QS_SERIALIZABLE

#define QS_IMPLEMENT_END \
  }                      \
  ;

/* Simplifying the Definition of Complete Serialized Classes */
#define QS_CLASS_BEGIN(className)        \
  class className : public QSerializer { \
    Q_GADGET                             \
    QS_SERIALIZABLE

#define QS_CLASS_END \
  }                  \
  ;

#define QS_MEMBER_SERIALIZE_OPTIONS(className, memberName, skipEmpty,   \
                                    skipNull, skipNullLiterals)         \
  namespace {                                                           \
  struct className##_##memberName##_options_initializer {               \
    className##_##memberName##_options_initializer() {                  \
      QSerializer::setMemberOptions(#className, #memberName, skipEmpty, \
                                    skipNull, skipNullLiterals);        \
    }                                                                   \
  };                                                                    \
  static className##_##memberName##_options_initializer                 \
      className##_##memberName##_options_init;                          \
  }

#define QS_MEMBER_SKIP_EMPTY(className, memberName) \
  QS_MEMBER_SERIALIZE_OPTIONS(className, memberName, true, false, false)

#define QS_MEMBER_SKIP_NULL(className, memberName) \
  QS_MEMBER_SERIALIZE_OPTIONS(className, memberName, false, true, false)

#define QS_MEMBER_SKIP_EMPTY_AND_NULL(className, memberName) \
  QS_MEMBER_SERIALIZE_OPTIONS(className, memberName, true, true, false)

#define QS_MEMBER_SKIP_EMPTY_AND_NULL_LITERALS(className, memberName) \
  QS_MEMBER_SERIALIZE_OPTIONS(className, memberName, true, true, true)

#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
// C++17 or later - use inline static
#define QS_INTERNAL_MEMBER_SERIALIZE_OPTIONS(memberName, skipEmpty, skipNull,  \
                                             skipNullLiterals)                 \
 private:                                                                      \
  class MemberOptionsInitializer_##memberName {                                \
   public:                                                                     \
    MemberOptionsInitializer_##memberName() {                                  \
      QSerializer::setMemberOptions(staticMetaObject.className(), #memberName, \
                                    skipEmpty, skipNull, skipNullLiterals);    \
    }                                                                          \
  };                                                                           \
  inline static MemberOptionsInitializer_##memberName                          \
      _memberOptionsInit_##memberName =                                        \
          MemberOptionsInitializer_##memberName();
#else
// C++14 or earlier - use static member
#define QS_INTERNAL_MEMBER_SERIALIZE_OPTIONS(memberName, skipEmpty, skipNull,  \
                                             skipNullLiterals)                 \
 private:                                                                      \
  static void _initializeMemberOptions_##memberName() {                        \
    static bool initialized = false;                                           \
    if (!initialized) {                                                        \
      QSerializer::setMemberOptions(staticMetaObject.className(), #memberName, \
                                    skipEmpty, skipNull, skipNullLiterals);    \
      initialized = true;                                                      \
    }                                                                          \
  }                                                                            \
  class MemberOptionsInitializer_##memberName {                                \
   public:                                                                     \
    MemberOptionsInitializer_##memberName() {                                  \
      _initializeMemberOptions_##memberName();                                 \
    }                                                                          \
  };                                                                           \
  MemberOptionsInitializer_##memberName _memberOptionsInit_##memberName;
#endif

#define QS_INTERNAL_MEMBER_SKIP_EMPTY(memberName) \
  QS_INTERNAL_MEMBER_SERIALIZE_OPTIONS(memberName, true, false, false)

#define QS_INTERNAL_MEMBER_SKIP_NULL(memberName) \
  QS_INTERNAL_MEMBER_SERIALIZE_OPTIONS(memberName, false, true, false)

#define QS_INTERNAL_MEMBER_SKIP_EMPTY_AND_NULL(memberName) \
  QS_INTERNAL_MEMBER_SERIALIZE_OPTIONS(memberName, true, true, false)

#define QS_INTERNAL_MEMBER_SKIP_EMPTY_AND_NULL_LITERALS(memberName) \
  QS_INTERNAL_MEMBER_SERIALIZE_OPTIONS(memberName, true, true, true)
#endif  // QSERIALIZER_H

#ifndef QSERIALIZER_IMPLEMENTATION
#define QSERIALIZER_IMPLEMENTATION

// Use conditional compilation to ensure that it is defined only once
#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
// C++17 or later - use inline static
inline QSerializer::OptionsMap QSerializer::s_classOptions;
inline QSerializer::MemberOptionsMap QSerializer::s_memberOptions;
#else
// C++14 or earlier - use static member
QSerializer::OptionsMap QSerializer::s_classOptions;
QSerializer::MemberOptionsMap QSerializer::s_memberOptions;
#endif

#endif  // QSERIALIZER_IMPLEMENTATION

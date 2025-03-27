#ifndef CLASSES_H
#define CLASSES_H
#include <QQueue>
#include <QSerializer>
#include <QStack>
#include <map>

class Parent : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE

public:
    Parent() {}
    Parent(int age, const QString &name, bool isMale)
        : age(age),
          name(name),
          male(isMale) {}
    QS_FIELD(int, age)
    QS_FIELD(QString, name)
    QS_FIELD(bool, male)
};

class Student : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE

public:
    Student() {}
    Student(int age, const QString &name, QStringList links, Parent mom, Parent dad)
        : age(age),
          name(name),
          links(links)
    {
        parents.append(mom);
        parents.append(dad);
    }
    QS_FIELD(int, age)
    QS_FIELD(QString, name)
    QS_COLLECTION(QList, QString, links)
    QS_COLLECTION_OBJECTS(QList, Parent, parents)
};

class Dictionaries : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE
    QS_QT_DICT(QHash, QString, QString, qt_hash)
    QS_QT_DICT(QMap, QString, QString, qt_map)
    QS_QT_DICT_OBJECTS(QMap, QString, Student, qt_map_objects);
    QS_STL_DICT(std::map, int, QString, std_map)
    QS_STL_DICT_OBJECTS(std::map, QString, Student, std_map_objects);
};

class Field : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE
    QS_FIELD(int, digit)
    QS_FIELD(QString, string)
    QS_FIELD(bool, flag)
    QS_FIELD(double, d_digit)
};

class Collection : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE
    QS_COLLECTION(QVector, int, vector)
    QS_COLLECTION(QList, QString, list)
    QS_COLLECTION(QStack, double, stack)
};

class CustomObject : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE
    QS_FIELD(int, digit)
    QS_COLLECTION(QVector, QString, string)
};

class CollectionOfObjects : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE
    QS_COLLECTION_OBJECTS(QVector, CustomObject, objects)
};

class EmptyClass : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE
    QS_FIELD(QString, str1)
    QS_FIELD(QString, str2)
    QS_FIELD(QString, str3)
    QS_FIELD_DEFAULT(QString, str4, "test")
    QS_FIELD(QString, str5)
    QS_FIELD_OPT(QString, str6)
    QS_COLLECTION(QVector, QString, strings)
    QS_OBJECT_OPT(CustomObject, object)
    QS_INTERNAL_MEMBER_SKIP_EMPTY_AND_NULL_LITERALS(str1)
    QS_INTERNAL_MEMBER_SKIP_EMPTY_AND_NULL_LITERALS(str3)
    QS_INTERNAL_MEMBER_SKIP_EMPTY_AND_NULL_LITERALS(str4)
    QS_INTERNAL_MEMBER_SKIP_EMPTY_AND_NULL_LITERALS(strings)
    QS_INTERNAL_MEMBER_SKIP_EMPTY_AND_NULL_LITERALS(object)
};
QS_MEMBER_SKIP_EMPTY_AND_NULL_LITERALS(EmptyClass, str2)

// {
//   "hosts": {
//     "baidu.com": "127.0.0.1",
//     "dns.google": [
//       "8.8.8.8",
//       "8.8.4.4"
//     ]
//   }
// }

class Host : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE

public:
    QMap<QString, QVariant> host;
    QJsonObject toJson() const override
    {
        auto json = this->QSerializer::toJson();
        for (auto it = host.begin(); it != host.end(); ++it)
        {
            if (it.value().metaType().id() == QMetaType::QStringList)
            {
                QJsonArray arr;
                for (auto &item : it.value().toStringList())
                {
                    arr.append(item);
                }
                json[it.key()] = arr;
            }
            else
            {
                json[it.key()] = it.value().toString();
            }
        }
        return json;
    }
    void fromJson(const QJsonValue &val) override
    {
        this->QSerializer::fromJson(val);
        host.clear();
        auto obj = val.toObject();
        for (auto it = obj.begin(); it != obj.end(); ++it)
        {
            if (it.value().isArray())
            {
                const auto &array = it.value().toArray();
                QStringList arr;
                for (const auto &item : array)
                {
                    arr.append(item.toString());
                }
                host.insert(it.key(), arr);
            }
            else
            {
                host.insert(it.key(), it.value().toString());
            }
        }
    }
};

class General : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE
    QS_OBJECT(Field, field)
    QS_OBJECT(Collection, collection)
    QS_OBJECT(CustomObject, object)
    QS_OBJECT(CollectionOfObjects, collectionObjects)
    QS_OBJECT(Dictionaries, dictionaries)
    QS_OBJECT(EmptyClass, emptyClass)
    QS_OBJECT(Host, host)
};

class TestXmlObject : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE
    QS_FIELD(int, digit)
    QS_COLLECTION(QVector, QString, string)
};

class TestXml : public QSerializer
{
    Q_GADGET
    QS_SERIALIZABLE
    QS_FIELD(int, field)
    QS_COLLECTION(QVector, int, collection)
    QS_OBJECT(TestXmlObject, object)
};

#endif // CLASSES_H
